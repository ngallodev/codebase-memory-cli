#!/usr/bin/env bash
# Keep fork registry metadata mutually consistent and preserve explicit
# distribution version/checksum pins. Shared local tags can belong to upstream;
# they do not prove what this fork published.
# Usage: tests/test_version_metadata_contract.sh [repo-root]

set -euo pipefail

case "${1:-}" in
-h | --help)
    sed -n '2,5p' "$0" | sed 's/^# \{0,1\}//'
    exit 0
    ;;
esac

ROOT="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

failures=0

fail() {
    echo "FAIL: $*" >&2
    failures=$((failures + 1))
}

# Local Git tags have no remote provenance in a checkout containing both fork
# and upstream refs. Publication freshness belongs to release qualification;
# this offline repository contract must not silently adopt an upstream version.
# ponytail: offline consistency only; published fork freshness needs release qualification.
echo "NOTE: local tags are not fork release authority; checking consistency and explicit pins."

# ── 2. Registered surfaces ──────────────────────────────────────────
# "release" = all registry-facing versions must agree. A new published fork
# version and its artifacts still require separate release qualification.
#
# "pin:<v>" = frozen at a version we have NOT re-pinned. These embed a
# per-release sha256 of a published asset, so bumping them means re-pinning
# checksums — a distribution chore, not a metadata edit. They are still gated:
# the version must match its explicit distribution pin. Every pin needs a
# reason (PIN_REASONS), so
# a frozen surface can never become an unexplained exemption.
SURFACES=(
    "pkg/npm/package.json|release"
    "pkg/pypi/pyproject.toml|release"
    "pkg/pypi/src/codebase_memory_cli/_cli.py|release"
    "pkg/go/cmd/codebase-memory-cli/main.go|release"
    "pkg/chocolatey/codebase-memory-cli.nuspec|release"
    "pkg/chocolatey/tools/chocolateyInstall.ps1|release"
    "pkg/homebrew/Formula/codebase-memory-cli.rb|pin:0.10.3"
    "pkg/scoop/codebase-memory-cli.json|pin:0.8.1"
    "pkg/aur/PKGBUILD|pin:0.8.1"
    "pkg/aur/.SRCINFO|pin:0.8.1"
)

# Why each pinned surface is frozen, and what clearing it costs. To bring one
# up to date: download the release's checksums.txt
# (gh release download v<X.Y.Z> --pattern checksums.txt), replace the version
# AND every sha256 it pins, then move the entry to "release" above.
PIN_REASONS=(
    "pkg/homebrew/Formula/codebase-memory-cli.rb|pins 4 per-asset sha256 (darwin/linux x arm/intel); last re-pinned for v0.10.3"
    "pkg/scoop/codebase-memory-cli.json|pins the windows-amd64.zip sha256; last re-pinned for v0.8.1"
    "pkg/aur/PKGBUILD|pins sha256sums_x86_64 + sha256sums_aarch64; last re-pinned for v0.8.1"
    "pkg/aur/.SRCINFO|generated from PKGBUILD, so it must move with it, not before it"
)

# Paths that legitimately contain a semver but make no claim about the
# product's current version. Each entry states what its number really is, so an
# unexplained exemption cannot hide here.
EXEMPT=(
    'pkg/winget/manifests/'          # published version-pinned manifests: the directory
                                     # name IS the version, and rewriting a shipped
                                     # manifest would falsify what 0.8.1 actually was
    'pkg/pypi/tests/'                # unit-test fixtures; the literal is arbitrary
                                     # cache-path input, not a published claim
    'pkg/npm/test/'                  # same: publication tests assert shape, not value
    'pkg/pypi/requirements-publish.txt'  # publish-toolchain pins (twine/hatchling)
    'tests/test_version_metadata_contract.sh'  # this file, which names the numbers above
)

# ── 3. Extraction ───────────────────────────────────────────────────
# Two independent claim forms, because a surface can lie in either one and the
# language-count contract learned the hard way that a gate sharing its regex
# with the enumeration can only confirm what the enumeration already saw:
#   declaration  version "X.Y.Z" / "version": "X.Y.Z" / pkgver=X.Y.Z / <version>
#   release URL  releases/download/vX.Y.Z/... , releases/tag/vX.Y.Z , -X.Y.Z-linux
# The Homebrew formula interpolates v#{version} into its URLs and so has no URL
# form at all; that is fine, the anti-vacuity check below is per FILE, not per
# form.
versions_in() {
    local path="$1"
    {
        # shellcheck disable=SC2016  # the literal text $version is what the
        # PowerShell surface writes, so this regex must NOT expand it
        grep -oiE '(pkgver|packageversion|\$version|"version"|<version>|^[[:space:]]*version)[^0-9]{0,16}[0-9]+\.[0-9]+\.[0-9]+' "$path" |
            grep -oE '[0-9]+\.[0-9]+\.[0-9]+$'
        grep -oE 'releases/(download|tag)/v[0-9]+\.[0-9]+\.[0-9]+' "$path" |
            grep -oE '[0-9]+\.[0-9]+\.[0-9]+$'
        grep -oE -- '-[0-9]+\.[0-9]+\.[0-9]+-(linux|darwin|windows)' "$path" |
            grep -oE '[0-9]+\.[0-9]+\.[0-9]+'
        # Bare literal form: pkg/pypi returns a hardcoded fallback from
        # _version() when importlib.metadata cannot resolve the installed
        # distribution, and that string is what `--version` then prints. The
        # anti-vacuity check below caught this one: the declaration form above
        # cannot see it, because the version keyword is in the function NAME.
        grep -oE 'return "[0-9]+\.[0-9]+\.[0-9]+"' "$path" |
            grep -oE '[0-9]+\.[0-9]+\.[0-9]+'
    } | sort -u
}

pin_reason_for() {
    local want="$1" entry
    for entry in "${PIN_REASONS[@]}"; do
        if [[ "${entry%%|*}" == "$want" ]]; then
            printf '%s' "${entry#*|}"
            return 0
        fi
    done
    printf ''
}

# ── 4. Every registered surface ─────────────────────────────────────
release_surface_count=0
checked_values=0

for entry in "${SURFACES[@]}"; do
    path="${entry%%|*}"
    rule="${entry#*|}"

    if [[ ! -f "$path" ]]; then
        fail "registered surface $path does not exist — update SURFACES"
        continue
    fi

    hits=$(versions_in "$path" || true)

    # Anti-vacuity. A surface that states no version is either no longer a
    # surface or has changed wording in a way this contract can no longer read.
    # Either way the silence is a defect, not a pass — the whole point of this
    # gate is that nothing states a version unchecked.
    if [[ -z "$hits" ]]; then
        fail "$path is registered as a version surface but states no version —" \
            "either it lost the claim (drop it from SURFACES) or its wording" \
            "changed and this check went vacuous"
        continue
    fi

    if [[ "$rule" == "release" ]]; then
        release_surface_count=$((release_surface_count + 1))
    else
        pinned="${rule#pin:}"
        reason="$(pin_reason_for "$path")"
        if [[ -z "$reason" ]]; then
            fail "$path is pinned at $pinned with no entry in PIN_REASONS —" \
                "a frozen surface must say WHY it is frozen"
        fi
    fi

    while IFS= read -r found; do
        [[ -z "$found" ]] && continue
        checked_values=$((checked_values + 1))

        if [[ "$rule" != "release" ]]; then
            pinned="${rule#pin:}"
            if [[ "$found" != "$pinned" ]]; then
                fail "$path declares $found but is pinned at $pinned —" \
                    "it embeds a per-release sha256, so the version and the" \
                    "checksums must move together (update PIN_REASONS or re-pin both)"
            fi
        fi
    done <<<"$hits"
done

# ── 5. Mutual consistency, independent of Git tags ────────────────
release_values=""
for entry in "${SURFACES[@]}"; do
    path="${entry%%|*}"
    [[ "${entry#*|}" == "release" ]] || continue
    [[ -f "$path" ]] || continue
    # `|| true` is load-bearing: versions_in ends in a pipeline and runs under
    # pipefail, so a claim form absent from THIS file
    # return form) makes it exit non-zero and set -e would kill the contract
    # mid-run — exiting 1 with no message, which reads exactly like a failure
    # that forgot to explain itself.
    release_values="$release_values$(versions_in "$path" || true)
"
done
distinct=$(printf '%s' "$release_values" | grep -vE '^$' | sort -u || true)
distinct_count=$(printf '%s\n' "$distinct" | grep -cE '^[0-9]' || true)

if ((distinct_count > 1)); then
    fail "the release-tracking surfaces disagree with each other: $(
        printf '%s' "$distinct" | tr '\n' ' '
    )"
fi
if ((distinct_count == 0)); then
    fail "no version could be read from ANY release-tracking surface —" \
        "this contract cannot vouch for anything and must not report success"
fi

# ── 6. No UNREGISTERED packaging surface may state a version ────────
# Without this, adding an eighth packaging target reintroduces exactly the drift
# this contract exists to prevent, and every check above would still pass. The
# sweep pattern is deliberately BROADER than the extractors in section 3: if the
# two shared a regex, a claim form the extractors cannot see would be invisible
# to the sweep as well.
while IFS= read -r path; do
    [[ -z "$path" ]] && continue
    for entry in "${SURFACES[@]}"; do
        [[ "$path" == "${entry%%|*}" ]] && continue 2
    done
    for skip in "${EXEMPT[@]}"; do
        [[ "$path" == "$skip"* ]] && continue 2
    done
    fail "$path states a product version but is not registered. Add it to" \
        "SURFACES so it stays in step with the release, or to EXEMPT with a" \
        "note saying what its number actually is."
done < <(
    git grep -IlE '(version|pkgver|releases/(download|tag)/v)[^0-9]{0,16}[0-9]+\.[0-9]+\.[0-9]+|return "[0-9]+\.[0-9]+\.[0-9]+"' \
        -- pkg tests/test_version_metadata_contract.sh 2>/dev/null || true
)

# Dispatch-version grammar. Both release entry points must accept a hyphenated
# semver prerelease such as the fork's v0.12.0-cli-rc.1 and keep refusing
# unprefixed, partial and trailing-garbage versions. A bare version burns an
# immutable release name (2026-08-18), so this is pinned from the workflow text
# itself rather than a copy of the regex.
for wf in release.yml promote-qualified-release.yml; do
    wf_path=".github/workflows/$wf"
    [[ -f "$wf_path" ]] || fail "$wf_path is missing"
    pattern="$(grep -oE '\^v\[0-9\]\+[^"]*\$' "$wf_path" | head -n 1)"
    [[ -n "$pattern" ]] || { fail "$wf_path has no version-grammar regex"; continue; }
    [[ "$pattern" == '^v[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z-]+(\.[0-9A-Za-z-]+)*)?$' ]] ||
        fail "$wf_path version grammar changed unexpectedly: $pattern"
    for good in v0.12.0 v0.12.0-rc.1 v0.12.0-cli-rc.1; do
        [[ "$good" =~ $pattern ]] || fail "$wf_path rejects valid version $good"
    done
    for bad in 0.12.0 v0.12 v0.12.0- v0.12.0.4 "v0.12.0-rc 1" v0.12.0-rc.1/x v0.12.0-rc..1 v0.12.0-.rc v0.12.0-rc.; do
        [[ "$bad" =~ $pattern ]] && fail "$wf_path accepts invalid version $bad"
    done
done
# A hyphen is what marks a version as a prerelease in release.yml; the prerelease
# publication job and the GitHub prerelease flag must both key off that.
grep -q "prerelease: \${{ contains(inputs.version, '-') }}" .github/workflows/release.yml ||
    fail "release.yml must mark hyphenated versions as GitHub prereleases"

if ((failures > 0)); then
    echo "FAIL: $failures version-metadata contract violation(s)" >&2
    exit 1
fi

echo "Version-metadata consistency passed (${#SURFACES[@]} surfaces," \
    "$release_surface_count registry surfaces, $checked_values value(s) checked)"
