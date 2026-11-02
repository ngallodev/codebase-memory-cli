#!/usr/bin/env bash
# Prove foreign tags do not change fork metadata truth, while drift still fails.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
python3 - "$ROOT" "$BASH" <<'PYTEST'
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

root, bash = Path(sys.argv[1]), sys.argv[2]
with tempfile.TemporaryDirectory(prefix="cbm-version-contract-") as tmp:
    fixture = Path(tmp)
    paths = subprocess.check_output(
        ["git", "ls-files", "pkg", ".github/workflows/release.yml",
         ".github/workflows/promote-qualified-release.yml"], cwd=root, text=True,
    ).splitlines()
    for relative in paths:
        target = fixture / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(root / relative, target)
    git = ["git", "-C", str(fixture), "-c", "user.name=Contract test",
           "-c", "user.email=contract@example.invalid"]
    subprocess.run([*git, "init", "-q"], check=True)
    subprocess.run([*git, "add", "."], check=True)
    subprocess.run([*git, "commit", "-qm", "fixture"], check=True)
    subprocess.run([*git, "tag", "v99.0.0"], check=True)
    command = [bash, str(root / "tests/test_version_metadata_contract.sh"), str(fixture)]
    good = subprocess.run(command, text=True, capture_output=True)
    assert good.returncode == 0, good.stdout + good.stderr
    manifest = fixture / "pkg/npm/package.json"
    data = json.loads(manifest.read_text())
    data["version"] = "88.0.0"
    manifest.write_text(json.dumps(data))
    bad = subprocess.run(command, text=True, capture_output=True)
    assert bad.returncode != 0 and "disagree with each other" in bad.stderr, bad.stdout + bad.stderr
    shutil.copyfile(root / "pkg/npm/package.json", manifest)
    formula = fixture / "pkg/homebrew/Formula/codebase-memory-cli.rb"
    formula.write_text(formula.read_text().replace('version "0.10.3"', 'version "88.0.0"'))
    bad_pin = subprocess.run(command, text=True, capture_output=True)
    assert bad_pin.returncode != 0 and "pinned at 0.10.3" in bad_pin.stderr, bad_pin.stdout + bad_pin.stderr
print("PASS: foreign tags ignored; registry drift and frozen artifact-pin drift rejected")
PYTEST
