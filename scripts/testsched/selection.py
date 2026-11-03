"""Resolve selectors to a registration-ordered set of suite:test names."""
from __future__ import annotations

import json
import pathlib
import subprocess
import sys


def select_tests(args, all_tests: list[str], data_dir: pathlib.Path, root: pathlib.Path) -> list[str]:
    if args.tests is not None:
        requested = args.tests.split(",")
        if len(requested) != len(set(requested)):
            raise ValueError("duplicate test in --tests")
        unknown = sorted(set(requested) - set(all_tests))
        if unknown:
            raise ValueError("unknown tests: " + ", ".join(unknown))
        wanted = set(requested)
        return [name for name in all_tests if name in wanted]

    if args.suites:
        wanted = set(args.suites)
        known = {name.split(":", 1)[0] for name in all_tests}
        unknown = sorted(wanted - known)
        if unknown:
            raise ValueError("unknown suites: " + ", ".join(unknown))
        return [name for name in all_tests if name.split(":", 1)[0] in wanted]

    if args.changed is not None:
        return changed_tests(args.changed, all_tests, data_dir, root)

    if args.failed:
        try:
            previous = json.loads((data_dir / "last-results.json").read_text())
        except (OSError, ValueError):
            previous = {}
        failed = {
            name for name, result in previous.items()
            if result.get("status") not in ("PASS", "SKIP")
        }
        return [name for name in all_tests if name in failed]

    return all_tests[:]


def changed_tests(base: str, all_tests: list[str], data_dir: pathlib.Path, root: pathlib.Path) -> list[str]:
    changed = data_dir / "changed.txt"
    prediction_path = data_dir / "prediction.json"
    selection_path = data_dir / "selector.json"
    data_dir.mkdir(parents=True, exist_ok=True)
    selection_path.write_text('{"full": false, "tier": "smart"}\n')
    diff = subprocess.run(
        ["git", "diff", "--name-only", base, "--"],
        cwd=root,
        text=True,
        capture_output=True,
    )
    if diff.returncode:
        print("changed diff failed; falling back to all tests", file=sys.stderr)
        return all_tests[:]
    changed.write_text(diff.stdout)
    command = [
        sys.executable,
        str(root / "scripts/test-impact/predict.py"),
        "--root", str(root),
        "--changed", str(changed),
        "--selection", str(selection_path),
        "--out", str(prediction_path),
    ]
    prediction = subprocess.run(command, cwd=root)
    if prediction.returncode:
        print("changed selection failed; falling back to all tests", file=sys.stderr)
        return all_tests[:]
    try:
        selected = json.loads(prediction_path.read_text()).get("suites", {})
    except (OSError, ValueError):
        print("changed prediction unreadable; falling back to all tests", file=sys.stderr)
        return all_tests[:]
    result = []
    for name in all_tests:
        suite, test = name.split(":", 1)
        selection = selected.get(suite)
        if selection == "ALL" or isinstance(selection, list) and test in selection:
            result.append(name)
    return result
