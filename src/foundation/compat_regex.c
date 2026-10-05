/*
 * compat_regex.c — Portable regular expression implementation.
 *
 * POSIX: direct wrappers around <regex.h>.
 * Windows: vendored TRE regex library (BSD-licensed).
 *
 * Both backends expand bounded repetition at compile time, so cbm_regcomp
 * sizes a pattern first (the compile-size guard below) and refuses one whose
 * compiled form would be too large, before the platform compiler allocates
 * anything.
 */
#include "foundation/constants.h"
#include "foundation/compat_regex.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

/* ── Compile-size guard (shared by both backends) ─────────────────
 *
 * The platform compilers expand `X{m,n}` into n copies of X, so nested
 * intervals multiply: `((((a){20}){20}){20}){20}` is 160,000 expanded atoms
 * from 25 bytes of pattern. A K-way alternation additionally costs about K^2
 * position-set entries on the TRE-derived backends (macOS libc and the vendored
 * TRE), because every union node copies its branches' sets.
 *
 * Measured on macOS libc with REG_EXTENDED|REG_NOSUB: the nested shape costs
 * about 0.6 KB of heap per expanded atom, 1.1 KB with submatch tracking on,
 * 1.7 KB with REG_ICASE and 2.3 KB with both; a K-way alternation adds about
 * 70 B x K^2 on top of its branches (twice that under REG_ICASE), and an
 * alternation under an interval pays that for every copy, about 1 KB per unit
 * with REG_NOSUB and 2.8 KB with REG_ICASE plus submatches. glibc costs about
 * 0.45 KB per atom and expands intervals the same way.
 *
 * cbm_regcomp_estimate_units() counts expanded atoms in one linear pass with
 * saturating arithmetic and charges a K-way alternation K^2/8 units, the
 * measured ratio between the two costs. With CBM_REGEX_COMPILE_BUDGET_UNITS =
 * 32768 the heaviest accepted compile is a repeated alternation at about 30 MB
 * with REG_NOSUB and about 90 MB under the ICASE-plus-submatch flag set no
 * call site uses; the nested shape tops out at 13^4 = 28,561 units, about
 * 16 MB. `(\w{1,100}){1,20}` (2,000 units) and an alternation of 500 short
 * names (31,250 units plus its letters) remain accepted. */

enum { REGEX_GROUP_DEPTH_MAX = 64 };

typedef struct {
    uint64_t total;    /* units of this group so far; includes `last` */
    uint64_t last;     /* units of the element a following quantifier applies to */
    uint64_t branches; /* alternatives in this group: 1 + the number of '|' */
} regex_frame_t;

static uint64_t regex_sat_add(uint64_t a, uint64_t b) {
    return a > UINT64_MAX - b ? UINT64_MAX : a + b;
}

static uint64_t regex_sat_mul(uint64_t a, uint64_t b) {
    if (a == 0 || b == 0) {
        return 0;
    }
    return a > UINT64_MAX / b ? UINT64_MAX : a * b;
}

static void regex_frame_atom(regex_frame_t *f, uint64_t units) {
    f->total = regex_sat_add(f->total, units);
    f->last = units;
}

/* An interval applies to the last element: n copies of it replace the one. */
static void regex_frame_repeat_last(regex_frame_t *f, uint64_t copies) {
    uint64_t grown = regex_sat_mul(f->last, copies);
    f->total = regex_sat_add(f->total - f->last, grown);
    f->last = grown;
}

/* Units a closed group contributes to its parent: its content plus the K^2/8
 * alternation share, never less than one atom. */
static uint64_t regex_frame_units(const regex_frame_t *f) {
    uint64_t alternation = regex_sat_mul(f->branches, f->branches) / 8;
    uint64_t units = regex_sat_add(f->total, alternation);
    return units ? units : 1;
}

/* Byte-locale TRE expands a named class into disjoint ranges. Count those
 * ranges even in multibyte locales, where the backend may use one class item. */
static uint64_t regex_class_items(const char *name, size_t len, bool icase) {
    char buf[64];
    if (len >= sizeof(buf)) {
        return 1; /* invalid class: the backend rejects it */
    }
    memcpy(buf, name, len);
    buf[len] = '\0';
    wctype_t cls = wctype(buf);
    uint64_t items = 0;
    bool previous = false;
    for (wint_t c = 0; c < 256; c++) {
        bool member =
            cls && (iswctype(c, cls) ||
                    (icase && (iswctype(towlower(c), cls) || iswctype(towupper(c), cls))));
        if (member && !previous) {
            items++;
        }
        previous = member;
    }
    return items ? items : 1;
}

static size_t regex_bracket_char(const char *p, size_t i, wchar_t *out) {
    mbstate_t state = {0};
    size_t len = mbrtowc(out, p + i, MB_LEN_MAX, &state);
    if (len == (size_t)-1 || len == (size_t)-2 || len == 0) {
        *out = (unsigned char)p[i];
        len = 1; /* malformed input is left for the backend to reject */
    }
    return i + len;
}

/* Bracket members form a union in TRE; repetition multiplies that union too.
 * Count ranges, leading ']', named classes and case-folded range items, then
 * apply the same conservative union charge used for ordinary alternation. */
static size_t regex_skip_bracket(const char *p, size_t i, bool icase, uint64_t *units) {
    size_t j = i + 1;
    bool negate = p[j] == '^';
    if (negate) {
        j++;
    }
    uint64_t items = 0;
    if (p[j] == ']') {
        items++;
        j++;
    }
    while (p[j] && p[j] != ']') {
        if (p[j] == '[' && (p[j + 1] == ':' || p[j + 1] == '=' || p[j + 1] == '.')) {
            char close = p[j + 1];
            size_t k = j + 2;
            while (p[k] && !(p[k] == close && p[k + 1] == ']')) {
                k++;
            }
            items = regex_sat_add(
                items, close == ':' ? regex_class_items(p + j + 2, k - j - 2, icase) : 1);
            j = p[k] ? k + 2 : k;
        } else {
            wchar_t first;
            j = regex_bracket_char(p, j, &first);
            wchar_t last = first;
            if (p[j] == '-' && p[j + 1] && p[j + 1] != ']') {
                j = regex_bracket_char(p, j + 1, &last);
            }
            items = regex_sat_add(items, 1);
            if (icase && last >= first) {
                if (last < 256) {
                    wint_t previous = WEOF;
                    for (wint_t c = (wint_t)first; c <= (wint_t)last; c++) {
                        wint_t folded =
                            iswlower(c) ? towupper(c) : (iswupper(c) ? towlower(c) : WEOF);
                        if (folded != WEOF && (previous == WEOF || folded != previous + 1)) {
                            items = regex_sat_add(items, 1);
                        }
                        previous = folded;
                    }
                } else {
                    /* ponytail: bound Unicode folding by one item per codepoint;
                     * count locale-specific runs if this ceiling rejects useful patterns. */
                    items = regex_sat_add(items, (uint64_t)(last - first) + 1);
                }
            }
        }
    }
    if (negate) {
        items = regex_sat_add(items, 1); /* complement's final range */
    }
    *units = regex_sat_add(items, regex_sat_mul(items, items) / 8);
    return p[j] ? j + 1 : j;
}

static bool regex_parse_digits(const char *p, size_t *i, uint64_t *value) {
    bool any = false;
    *value = 0;
    while (p[*i] >= '0' && p[*i] <= '9') {
        *value = regex_sat_add(regex_sat_mul(*value, 10), (uint64_t)(p[*i] - '0'));
        (*i)++;
        any = true;
    }
    return any;
}

/* Parse the interval body that starts at p[i], just past the opening brace
 * (`{` in extended syntax, `\{` in basic). On success stores the number of
 * copies the compiler will make (the upper bound, or the lower bound for
 * `{m,}`, at least 1) and the index just past the closing brace. Anything that
 * is not an interval leaves the brace to be counted as a literal, which is what
 * the backends that accept it do; the others reject the pattern. */
static bool regex_parse_interval(const char *p, size_t i, bool extended, uint64_t *copies,
                                 size_t *end) {
    uint64_t lower = 0;
    uint64_t upper = 0;
    bool has_lower = regex_parse_digits(p, &i, &lower);
    bool has_upper = false;
    if (p[i] == ',') {
        i++;
        has_upper = regex_parse_digits(p, &i, &upper);
    } else {
        if (!has_lower) {
            return false;
        }
        upper = lower;
        has_upper = true;
    }
    if (extended) {
        if (p[i] != '}') {
            return false;
        }
        i++;
    } else {
        if (p[i] != '\\' || p[i + 1] != '}') {
            return false;
        }
        i += 2;
    }
    uint64_t n = has_upper ? upper : lower;
    if (has_upper && lower > upper) {
        n = lower; /* `{5,3}` is rejected by the backends; count the larger bound */
    }
    *copies = n ? n : 1;
    *end = i;
    return true;
}

uint64_t cbm_regcomp_estimate_units(const char *pattern, int flags) {
    const bool extended = (flags & CBM_REG_EXTENDED) != 0;
    regex_frame_t frames[REGEX_GROUP_DEPTH_MAX];
    int depth = 0;
    frames[0] = (regex_frame_t){0, 0, 1};

    size_t i = 0;
    while (pattern[i]) {
        char c = pattern[i];
        if (c == '[') {
            uint64_t units = 0;
            i = regex_skip_bracket(pattern, i, (flags & CBM_REG_ICASE) != 0, &units);
            regex_frame_atom(&frames[depth], units);
            continue;
        }
        /* In extended syntax the bare characters operate and `\x` is one
         * literal; in basic syntax only `\(`, `\)`, `\{`, `\|`, `\+`, `\?` and
         * a bare `*` operate. */
        char op = 0;
        size_t len = 1;
        if (c == '\\') {
            len = pattern[i + 1] ? 2 : 1;
            if (!extended && len == 2 && strchr("(){|+?", pattern[i + 1])) {
                op = pattern[i + 1];
            }
        } else if (extended ? strchr("(){|*+?", c) != NULL : c == '*') {
            op = c;
        }
        regex_frame_t *f = &frames[depth];
        switch (op) {
        case '(':
            if (depth + 1 >= REGEX_GROUP_DEPTH_MAX) {
                return UINT64_MAX;
            }
            frames[++depth] = (regex_frame_t){0, 0, 1};
            break;
        case ')':
            if (depth == 0) {
                regex_frame_atom(f, 1); /* unmatched: a literal where accepted */
                break;
            }
            depth--;
            regex_frame_atom(&frames[depth], regex_frame_units(f));
            break;
        case '|':
            f->branches = regex_sat_add(f->branches, 1);
            f->last = 0;
            break;
        case '{': {
            uint64_t copies = 0;
            size_t end = 0;
            if (regex_parse_interval(pattern, i + len, extended, &copies, &end)) {
                regex_frame_repeat_last(f, copies);
                i = end;
                continue;
            }
            regex_frame_atom(f, 1);
            break;
        }
        case '*':
        case '+':
        case '?':
            break; /* unbounded or optional repetition is not expanded */
        default:
            regex_frame_atom(f, 1);
            break;
        }
        i += len;
    }
    while (depth > 0) {
        regex_frame_t *f = &frames[depth];
        depth--;
        regex_frame_atom(&frames[depth], regex_frame_units(f));
    }
    return regex_frame_units(&frames[0]);
}

/* Refuse a pattern before the platform compiler sees it. */
static int regex_compile_guard(const char *pattern, int flags) {
    size_t len = 0;
    while (len <= (size_t)CBM_REGEX_PATTERN_MAX_BYTES && pattern[len]) {
        len++;
    }
    if (len > (size_t)CBM_REGEX_PATTERN_MAX_BYTES) {
        return CBM_REG_ETOOBIG;
    }
    if (cbm_regcomp_estimate_units(pattern, flags) > (uint64_t)CBM_REGEX_COMPILE_BUDGET_UNITS) {
        return CBM_REG_ETOOBIG;
    }
    return CBM_REG_OK;
}

#ifdef _WIN32

/* ── Windows: use vendored TRE regex ─────────────────────────── */
#include "../../vendored/tre/regex.h"

_Static_assert(sizeof(regex_t) <= CBM_SZ_256,
               "cbm_regex_t opaque buffer too small for TRE regex_t");

static int translate_flags_tre(int flags) {
    int tre_flags = 0;
    if (flags & CBM_REG_EXTENDED)
        tre_flags |= REG_EXTENDED;
    if (flags & CBM_REG_ICASE)
        tre_flags |= REG_ICASE;
    if (flags & CBM_REG_NOSUB)
        tre_flags |= REG_NOSUB;
    if (flags & CBM_REG_NEWLINE)
        tre_flags |= REG_NEWLINE;
    return tre_flags;
}

int cbm_regcomp(cbm_regex_t *r, const char *pattern, int flags) {
    int refused = regex_compile_guard(pattern, flags);
    if (refused != CBM_REG_OK) {
        return refused;
    }
    regex_t *re = (regex_t *)r->opaque;
    int rc = tre_regcomp(re, pattern, translate_flags_tre(flags));
    return rc == 0 ? CBM_REG_OK : rc;
}

int cbm_regexec(const cbm_regex_t *r, const char *str, int nmatch, cbm_regmatch_t *matches,
                int eflags) {
    const regex_t *re = (const regex_t *)r->opaque;
    if (nmatch <= 0 || !matches) {
        int rc = tre_regexec(re, str, 0, NULL, eflags);
        return rc == 0 ? CBM_REG_OK : CBM_REG_NOMATCH;
    }
    regmatch_t pmatch[CBM_SZ_32];
    int n = nmatch > CBM_SZ_32 ? CBM_SZ_32 : nmatch;
    int rc = tre_regexec(re, str, (size_t)n, pmatch, eflags);
    if (rc != 0)
        return CBM_REG_NOMATCH;
    for (int i = 0; i < n; i++) {
        matches[i].rm_so = (int)pmatch[i].rm_so;
        matches[i].rm_eo = (int)pmatch[i].rm_eo;
    }
    return CBM_REG_OK;
}

void cbm_regfree(cbm_regex_t *r) {
    regex_t *re = (regex_t *)r->opaque;
    tre_regfree(re);
}

#else /* POSIX */

/* ── POSIX implementation ─────────────────────────────────────── */

#include <regex.h>

/* Static assert: our opaque buffer is large enough for regex_t.
 * If this fires, increase cbm_regex_t.opaque size. */
_Static_assert(sizeof(regex_t) <= CBM_SZ_256, "cbm_regex_t opaque buffer too small for regex_t");

static int translate_flags(int flags) {
    int posix_flags = 0;
    if (flags & CBM_REG_EXTENDED) {
        posix_flags |= REG_EXTENDED;
    }
    if (flags & CBM_REG_ICASE) {
        posix_flags |= REG_ICASE;
    }
    if (flags & CBM_REG_NOSUB) {
        posix_flags |= REG_NOSUB;
    }
    if (flags & CBM_REG_NEWLINE) {
        posix_flags |= REG_NEWLINE;
    }
    return posix_flags;
}

int cbm_regcomp(cbm_regex_t *r, const char *pattern, int flags) {
    int refused = regex_compile_guard(pattern, flags);
    if (refused != CBM_REG_OK) {
        return refused;
    }
    regex_t *re = (regex_t *)r->opaque;
    int rc = regcomp(re, pattern, translate_flags(flags));
    return rc == 0 ? CBM_REG_OK : rc;
}

int cbm_regexec(const cbm_regex_t *r, const char *str, int nmatch, cbm_regmatch_t *matches,
                int eflags) {
    const regex_t *re = (const regex_t *)r->opaque;

    if (nmatch <= 0 || !matches) {
        int rc = regexec(re, str, 0, NULL, eflags);
        return rc == 0 ? CBM_REG_OK : CBM_REG_NOMATCH;
    }

    /* Map through POSIX regmatch_t */
    regmatch_t pmatch[CBM_SZ_32];
    int n = nmatch > CBM_SZ_32 ? CBM_SZ_32 : nmatch;
    int rc = regexec(re, str, (size_t)n, pmatch, eflags);
    if (rc != 0) {
        return CBM_REG_NOMATCH;
    }

    for (int i = 0; i < n; i++) {
        matches[i].rm_so = (int)pmatch[i].rm_so;
        matches[i].rm_eo = (int)pmatch[i].rm_eo;
    }
    return CBM_REG_OK;
}

void cbm_regfree(cbm_regex_t *r) {
    regex_t *re = (regex_t *)r->opaque;
    regfree(re);
}

#endif /* _WIN32 */
