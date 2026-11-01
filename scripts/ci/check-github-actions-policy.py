#!/usr/bin/env python3
"""Jenkins guard: forbid billable, automatically triggered GitHub Actions.

Only manually dispatched or reusable workflows are permitted in this fork.
When importing upstream changes, keep this policy even if upstream adds CI.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
WORKFLOWS = ROOT / ".github" / "workflows"
ALLOWED = {"workflow_dispatch", "workflow_call"}
ON = re.compile(r"""^(?:on|"on"|'on'):\s*(?:#.*)?$""")
EVENT = re.compile(r"^  ([A-Za-z_][A-Za-z0-9_-]*):")

def inspect(path: Path) -> list[str]:
    lines = path.read_text(encoding="utf-8").splitlines()
    triggers = []
    found = False
    for idx, line in enumerate(lines):
        if not ON.fullmatch(line):
            continue
        if found:
            return [f"{path.name}: multiple on sections"]
        found = True
        for sub in lines[idx + 1:]:
            if sub and not sub.startswith((" ", "#")):
                break
            match = EVENT.match(sub)
            if match:
                triggers.append(match.group(1))
    if not found or not triggers:
        return [f"{path.name}: missing or non-block YAML on declaration"]
    return [f"{path.name}: forbidden GitHub Actions trigger {event!r}"
            for event in triggers if event not in ALLOWED]

def main() -> int:
    errors = []
    if not WORKFLOWS.is_dir():
        errors.append("missing GitHub workflows directory")
    else:
        for path in sorted((*WORKFLOWS.glob("*.yml"), *WORKFLOWS.glob("*.yaml"))):
            errors.extend(inspect(path))
    if errors:
        for error in errors:
            print("CI policy violation:", error, file=sys.stderr)
        return 1
    print("GitHub Actions policy: manual/reusable only; Jenkins owns automatic CI")
    return 0

if __name__ == "__main__":
    sys.exit(main())
