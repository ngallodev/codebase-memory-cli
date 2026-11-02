#!/usr/bin/env bash
# Manual-workflow wiring contract: release/dry-run callers run all lanes,
# reusable inputs remain fail-open, and Jenkins owns automatic verification.
# The retired pr.yml matrix is deliberately not a required product surface.
# Text-level checks (no YAML library on every leg), like the other workflow
# contracts.
#
# Usage: tests/test_lane_wiring_contract.sh [repo-root]

set -euo pipefail

ROOT="${1:-$(cd "$(dirname "$0")/.." && pwd)}"

python3 - "$ROOT" <<'PY'
import pathlib
import re
import sys

root = pathlib.Path(sys.argv[1])
wf = root / ".github" / "workflows"
failures = []


def text(name):
    return (wf / name).read_text(encoding="utf-8")


def jobs(source):
    """job id -> body (the lines indented under `  <id>:`)."""
    out = {}
    for m in re.finditer(r"(?m)^  ([A-Za-z0-9_-]+):\s*$", source):
        rest = source[m.end():]
        end = re.search(r"(?m)^  [A-Za-z0-9_#-]", rest)
        out[m.group(1)] = rest[:end.start()] if end else rest
    return out


def require(cond, message):
    if not cond:
        failures.append(message)


# 1. dry run and release: no selection, so every job runs.
REUSABLE = ("_test.yml", "_lint.yml", "_security.yml")
for caller in ("dry-run.yml", "release.yml"):
    for job, body in jobs(text(caller)).items():
        called = re.search(r"uses: \./\.github/workflows/(_\w+\.yml)", body)
        if called and called.group(1) in REUSABLE:
            require(not re.search(r"(?m)^\s+(lanes|shard_profile|test_selection):", body),
                    f"{caller} job {job} passes a lane or test selection to {called.group(1)}; "
                    f"dry runs and releases must run every lane and every test")
m = re.search(r"(?ms)^      shard_profile:\n(.*?)(?=^      \S|^\S|\Z)", text("_test.yml"))
require(m is not None and re.search(r"(?m)^        default: standard$", m.group(1)) is not None,
        "_test.yml must declare a shard_profile input defaulting to standard")
for callee in REUSABLE:
    src = text(callee)
    m = re.search(r"(?ms)^      lanes:\n(.*?)(?=^      \S|^\S|\Z)", src)
    require(m is not None, f"{callee} must declare a `lanes` input")
    if m:
        require(re.search(r"(?m)^        type: string$", m.group(1)) is not None,
                f"{callee} lanes input must be a string")
        require(re.search(r"(?m)^        default: all$", m.group(1)) is not None,
                f"{callee} lanes input must default to all")

# Jenkins retains the full test preparation/shard path and workflow policy guard.
pipeline = (root / "Jenkinsfile").read_text(encoding="utf-8")
for needle in ("CBM_TEST_PHASE=prepare", "scripts/ci/check-github-actions-policy.py",
               "scripts/ci/verify-shard-union.sh"):
    require(needle in pipeline, f"Jenkins must run {needle}")
test_wf = jobs(text("_test.yml"))
require('--lanes "$LANES"' in test_wf.get("shard-completeness", ""),
        "manual shard-completeness must verify the selected shard union")

# 4. the test selection: PR-only, fails open.
m = re.search(r"(?ms)^      test_selection:\n(.*?)(?=^      \S|^\S|\Z)", text("_test.yml"))
require(m is not None and re.search(r"(?m)^        default: ''$", m.group(1)) is not None,
        "_test.yml must declare a test_selection input defaulting to '' (every test)")
readers = sorted(job for job, body in test_wf.items() if "inputs.test_selection" in body)
# Only the Linux x86_64 leg narrows (user decision 2026-10-16): the coverage comes from
# the Linux x86_64 producer, so arm64, macOS and Windows legs run every test.
require(readers == ["test-unix"], f"only test-unix may read test_selection, found {readers}")
unix = test_wf.get("test-unix", "")
linux_x64 = "inputs.test_selection != '' && runner.os == 'Linux' && runner.arch == 'X64'"
require(f"if: ${{{{ {linux_x64} }}}}" in unix and
        f"CBM_TEST_SELECTION_DIR: ${{{{ {linux_x64} && " in unix,
        "_test.yml test-unix must fetch the selection and hand it to scripts/test.sh as "
        "CBM_TEST_SELECTION_DIR only on Linux x86_64 and only when one was passed")
require("CBM_TEST_SELECTION_DIR: ''" in test_wf.get("test-windows", ""),
        "_test.yml test-windows must run every test (CBM_TEST_SELECTION_DIR: '')")

if failures:
    print("LANE WIRING CONTRACT VIOLATED:")
    for failure in failures:
        print(f"  {failure}")
    sys.exit(1)
print("lane-wiring contract OK (Jenkins owns automatic CI; manual release/dry-run run all lanes)")
PY
