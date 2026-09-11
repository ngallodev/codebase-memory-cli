#!/usr/bin/env python3
"""Byte-identity audit: compare every vendored license file against upstream.

Verdicts:
  IDENTICAL          byte-equal to the upstream default-branch license
  IDENTICAL@PINNED   byte-equal to the license at the manifest's pinned commit
  FIRST-PARTY-OK     byte-equal to the project root LICENSE
  FIRST-PARTY-VAR    first-party but text differs from root LICENSE (inspect)
  DIFFERS            no byte-equal upstream candidate found (inspect)
  ERROR              upstream fetch failed
"""
import base64
from concurrent.futures import ThreadPoolExecutor, as_completed
from functools import lru_cache
import hashlib
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GRAMMARS = os.path.join(ROOT, "internal/cbm/vendored/grammars")

# Grammars authored in this repository. This is a REGISTRATION, not an
# exemption: membership only routes the check -- the audit still requires the
# vendored LICENSE to be byte-equal to the project root LICENSE, and reports
# FIRST-PARTY-VAR (a failing verdict) when it is not.
FIRST_PARTY = {"chialisp", "cobol", "form", "janet", "magma", "protobuf", "wolfram"}
FORKS = {  # self-maintained forks: vendored LICENSE must match the original upstream
    # arkts: our own ArkTS fork of tree-sitter-typescript. The vendored LICENSE is
    # upstream's MIT verbatim, which is what MIT requires of a derivative; our own
    # copyright and the fork's provenance live where our SOURCE is (grammar.js
    # header, MANIFEST.md, THIRD_PARTY.md) rather than inside upstream's notice.
    # This is a routing entry, NOT an exemption -- check_upstream still byte-verifies
    # against the real upstream repository.
    "arkts": "tree-sitter/tree-sitter-typescript",
    "cfml": "cfmleditor/tree-sitter-cfml",
    "cfscript": "cfmleditor/tree-sitter-cfml",
    "dotenv": "pnx/tree-sitter-dotenv",
    "qml": "yuja/tree-sitter-qmljs",
}
SPECIAL_NOTICE = {
    "assembly": "RETAINED-MIT (upstream RubixDev/tree-sitter-assembly deleted from GitHub)",
    "pine": "PROVENANCE-NOTICE (upstream kvarenzn/tree-sitter-pine declares ISC, ships no license file)",
}
DISAGREEMENT = {
    "jinja2": "dbt-labs/tree-sitter-jinja2",
    "just": "casey/tree-sitter-just",
    "move": "tzakian/tree-sitter-move",
    "sshconfig": "ObserverOfTime/tree-sitter-ssh-config",
    "zsh": "georgeharker/tree-sitter-zsh",
}
LIBS = {
    "vendored/mimalloc": ("microsoft/mimalloc", None),
    "vendored/tre": ("laurikari/tre", None),
    "vendored/xxhash": ("Cyan4973/xxHash", None),
    "vendored/yyjson": ("ibireme/yyjson", None),
    "internal/cbm/vendored/lz4": ("lz4/lz4", "lib/LICENSE"),
    "internal/cbm/vendored/zstd": ("facebook/zstd", None),
    "internal/cbm/vendored/simplecpp": ("danmar/simplecpp", None),
    "internal/cbm/vendored/verstable": ("JacksonAllan/Verstable", None),
    "internal/cbm/vendored/wyhash": ("wangyi-fudan/wyhash", None),
    "internal/cbm/vendored/ts_runtime": ("tree-sitter/tree-sitter", None),
    "internal/cbm/vendored/common": ("tree-sitter/tree-sitter-html", None),
    "internal/cbm/vendored/common/tree_sitter": ("tree-sitter/tree-sitter", None),
}
CANDIDATE_NAMES = ["LICENSE", "LICENSE.md", "LICENSE.txt", "COPYING",
                   "COPYING.txt", "LICENSE-MIT", "UNLICENSE", "LICENCE",
                   "license", "License.txt", "NOTICE"]


@lru_cache(maxsize=None)
def gh_api(path):
    """Fetch one GitHub API path once per audit process."""
    r = subprocess.run(["gh", "api", path], capture_output=True, text=True)
    if r.returncode != 0:
        detail = r.stderr.strip().replace("\n", " ")
        print(f"GitHub API request failed ({path}): {detail}", file=sys.stderr)
        return None
    return r.stdout


def gh_json(path):
    out = gh_api(path)
    if not out:
        return None
    try:
        return json.loads(out)
    except json.JSONDecodeError:
        return None


@lru_cache(maxsize=None)
def default_branch(repo):
    data = gh_json(f"repos/{repo}")
    return data.get("default_branch") if isinstance(data, dict) else None


@lru_cache(maxsize=None)
def upstream_tree(repo, ref=None):
    """Return one repository/ref tree, or None when GitHub cannot provide it."""
    tree_ref = ref or default_branch(repo)
    if not tree_ref:
        return None
    data = gh_json(f"repos/{repo}/git/trees/{tree_ref}?recursive=1")
    if not isinstance(data, dict) or data.get("truncated"):
        return None
    return {
        entry["path"]: entry["sha"]
        for entry in data.get("tree", [])
        if entry.get("type") == "blob" and entry.get("path") and entry.get("sha")
    }


def upstream_blob(repo, sha):
    data = gh_json(f"repos/{repo}/git/blobs/{sha}")
    if not isinstance(data, dict) or data.get("encoding") != "base64":
        return None
    try:
        return base64.b64decode(data.get("content", "")).decode("utf-8", "replace")
    except (ValueError, UnicodeDecodeError):
        return None


def upstream_default_license(repo):
    out = gh_api(f"repos/{repo}/license")
    if not out:
        return None
    try:
        d = json.loads(out)
        return base64.b64decode(d.get("content", "")).decode("utf-8", "replace")
    except Exception:
        return None


def upstream_file(repo, path, ref=None):
    tree = upstream_tree(repo, ref)
    if tree is not None and path in tree:
        return upstream_blob(repo, tree[path])

    url = f"repos/{repo}/contents/{path}"
    if ref:
        url += f"?ref={ref}"
    out = gh_api(url)
    if not out:
        return None
    try:
        d = json.loads(out)
        if isinstance(d, dict) and d.get("content"):
            return base64.b64decode(d["content"]).decode("utf-8", "replace")
    except Exception:
        pass
    return None


def upstream_named_file(repo, name, ref=None):
    """Find a named license in a cached tree, falling back to contents API."""
    tree = upstream_tree(repo, ref)
    if tree is not None:
        wanted = name.lower()
        for path, sha in tree.items():
            if os.path.basename(path).lower() == wanted:
                content = upstream_blob(repo, sha)
                if content is not None:
                    return content
        return None
    return upstream_file(repo, name, ref)


def local_license(dirpath):
    if not os.path.isdir(dirpath):
        return None, None
    for f in sorted(os.listdir(dirpath)):
        if re.match(r"^(LICENSE|LICENCE|COPYING|UNLICENSE)", f, re.I):
            p = os.path.join(dirpath, f)
            with open(p, encoding="utf-8", errors="replace") as fh:
                return f, fh.read()
    return None, None


def parse_manifest():
    """grammar -> (repo, pinned_commit) from the verified-upstream table."""
    out = {}
    with open(os.path.join(GRAMMARS, "MANIFEST.md"), encoding="utf-8") as fh:
        for line in fh:
            m = re.match(
                r"^\| (\w[\w.+-]*) \| \d+ \| ([\w.-]+/[\w.-]+) \| `([^`]+)` \|", line)
            if m:
                out[m.group(1)] = (m.group(2), m.group(3))
    return out


def main():
    with open(os.path.join(ROOT, "LICENSE"), encoding="utf-8") as fh:
        root_license = fh.read()

    manifest = parse_manifest()
    results = {}

    def check_upstream(dirpath, repo, pinned=None, exact_path=None):
        fname, ours = local_license(dirpath)
        if ours is None:
            return "NO-LOCAL-LICENSE", ""
        # 1) exact upstream path (e.g. lz4 lib/LICENSE), at HEAD then pinned
        if exact_path:
            for ref in (None, pinned):
                up = upstream_file(repo, exact_path, ref)
                if up is not None and up == ours:
                    return ("IDENTICAL" if ref is None else "IDENTICAL@PINNED",
                            f"{repo}:{exact_path}")
        # 2) default-branch detected license
        up = upstream_default_license(repo)
        if up is not None and up == ours:
            return "IDENTICAL", f"{repo} (default branch)"
        # 3) pinned-commit candidates by filename
        if pinned:
            tried = [fname] + [c for c in CANDIDATE_NAMES if c != fname]
            for cand in tried:
                up2 = upstream_named_file(repo, cand, pinned)
                if up2 is not None and up2 == ours:
                    return "IDENTICAL@PINNED", f"{repo}:{cand}@{pinned}"
        # 4) HEAD candidates by filename
        for cand in CANDIDATE_NAMES:
            up3 = upstream_named_file(repo, cand)
            if up3 is not None and up3 == ours:
                return "IDENTICAL", f"{repo}:{cand} (default branch)"
        return ("DIFFERS" if up is not None else "ERROR", f"{repo}")

    checks = []

    # Libraries
    for rel, (repo, exact) in LIBS.items():
        checks.append((rel, os.path.join(ROOT, rel), repo, None, exact))

    # Special: nomic = canonical Apache-2.0 text; sqlite3 = first-party notice
    #
    # This used to `curl` https://www.apache.org/licenses/LICENSE-2.0.txt at
    # gate time and byte-compare. A gating verdict must not depend on a live
    # HTTP request: any fetch failure yielded an empty string, compared unequal,
    # and reported DIFFERS -- indistinguishable from a real licence change. That
    # is what reddened `security / license-gate` on PR #1337 for over two weeks,
    # on a branch touching three CLI files and no licence at all.
    #
    # The canonical Apache-2.0 text is immutable and versioned, so it is pinned
    # by digest instead. Verified byte-identical to the upstream text at the time
    # of pinning (11358 bytes). A mismatch now means our vendored copy changed --
    # which is exactly, and only, what this audit is meant to detect.
    APACHE_2_0_SHA256 = "cfc7749b96f63bd31c3c42b5c471bf756814053e847c10f3eb003417bc523d30"
    fname, ours = local_license(os.path.join(ROOT, "vendored/nomic"))
    ours_digest = hashlib.sha256(ours.encode("utf-8")).hexdigest() if ours is not None else None
    results["vendored/nomic"] = (
        "IDENTICAL" if ours_digest == APACHE_2_0_SHA256 else "DIFFERS",
        "canonical Apache-2.0 text, pinned by sha256")
    results["vendored/sqlite3"] = ("FIRST-PARTY-NOTICE",
                                   "own public-domain notice (sqlite has no upstream LICENSE)")

    # Grammars
    for g in sorted(os.listdir(GRAMMARS)):
        d = os.path.join(GRAMMARS, g)
        if not os.path.isdir(d):
            continue
        key = f"grammars/{g}"
        if g in FIRST_PARTY:
            fname, ours = local_license(d)
            if ours == root_license:
                results[key] = ("FIRST-PARTY-OK", "== project root LICENSE")
            else:
                results[key] = ("FIRST-PARTY-VAR", f"{fname}: differs from root LICENSE")
            continue
        if g in FORKS:
            checks.append((key, d, FORKS[g], None, None))
            continue
        if g in SPECIAL_NOTICE:
            results[key] = ("MANUAL-VERIFIED", SPECIAL_NOTICE[g])
            continue
        if g in DISAGREEMENT:
            checks.append((key, d, DISAGREEMENT[g], None, None))
            continue
        if g in manifest:
            repo, pinned = manifest[g]
            checks.append((key, d, repo, pinned, None))
            continue
        results[key] = ("NO-MANIFEST-ENTRY", "")

    workers = max(1, int(os.environ.get("CBM_PROVENANCE_WORKERS", "16")))
    with ThreadPoolExecutor(max_workers=workers) as pool:
        futures = {
            pool.submit(check_upstream, directory, repo, pinned, exact): key
            for key, directory, repo, pinned, exact in checks
        }
        for future in as_completed(futures):
            results[futures[future]] = future.result()

    # Report
    from collections import Counter
    counts = Counter(v[0] for v in results.values())
    print("=== verdict histogram ===")
    for k, v in counts.most_common():
        print(f"  {k}: {v}")
    print()
    print("=== everything that is NOT byte-identical ===")
    for key in sorted(results):
        verdict, detail = results[key]
        if verdict not in ("IDENTICAL", "IDENTICAL@PINNED", "FIRST-PARTY-OK"):
            print(f"  {key}: {verdict} [{detail}]")
    results_path = os.environ.get(
        "CBM_PROVENANCE_RESULTS",
        os.path.join(ROOT, "build", "audit_licenses_results.json"),
    )
    os.makedirs(os.path.dirname(os.path.abspath(results_path)), exist_ok=True)
    with open(results_path, "w", encoding="utf-8") as fh:
        json.dump({k: list(v) for k, v in results.items()}, fh, indent=1)
    print(f"\nfull results: {results_path}")

    accepted = {"IDENTICAL", "IDENTICAL@PINNED", "FIRST-PARTY-OK",
                "FIRST-PARTY-NOTICE", "MANUAL-VERIFIED"}
    bad = {k: v for k, v in results.items() if v[0] not in accepted}
    if bad:
        print(f"\nPROVENANCE AUDIT FAILED: {len(bad)} unexplained verdict(s)")
        sys.exit(1)
    print("\nPROVENANCE AUDIT PASSED: every vendored license accounted for")


if __name__ == "__main__":
    main()
