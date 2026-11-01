#!/usr/bin/env python3
"""Verify a real upstream checkpoint and merge-parent ancestry using Git."""
import argparse
import os
import re
import subprocess
import sys


def git(*args: str) -> str:
    env = os.environ | {"GIT_NO_REPLACE_OBJECTS": "1"}
    return subprocess.check_output(["git", *args], text=True, stderr=subprocess.PIPE, env=env).strip()


def exact_commit(value: str) -> str:
    if not re.fullmatch(r"(?:[0-9a-f]{40}|[0-9a-f]{64})", value):
        raise ValueError(f"expected a full commit SHA, got {value!r}")
    resolved = git("rev-parse", "--verify", f"{value}^{{commit}}")
    if resolved != value:
        raise ValueError(f"commit did not resolve to the exact supplied SHA: {value}")
    return resolved


def verify(checkpoint: str, target: str, merge: str) -> None:
    checkpoint = exact_commit(checkpoint)
    target = exact_commit(target)
    merge = exact_commit(merge)
    if subprocess.run(["git", "merge-base", "--is-ancestor", checkpoint, target],
                      env=os.environ | {"GIT_NO_REPLACE_OBJECTS": "1"}, check=False).returncode:
        raise ValueError("checkpoint is not an ancestor of pinned target")
    parents = git("rev-list", "--parents", "-n", "1", merge).split()[1:]
    if len(parents) < 2:
        raise ValueError("result is not a merge commit")
    if not any(subprocess.run(["git", "merge-base", "--is-ancestor", target, parent],
                              env=os.environ | {"GIT_NO_REPLACE_OBJECTS": "1"},
                              check=False).returncode == 0 for parent in parents[1:]):
        raise ValueError("pinned target is not in a non-first merge-parent history")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--checkpoint", required=True, help="full upstream checkpoint commit SHA")
    parser.add_argument("--target", required=True, help="full pinned upstream target commit SHA")
    parser.add_argument("--merge", required=True, help="full integration merge commit SHA")
    args = parser.parse_args()
    try:
        verify(args.checkpoint, args.target, args.merge)
    except (ValueError, subprocess.CalledProcessError) as exc:
        print(f"sync ancestry check failed: {exc}", file=sys.stderr)
        return 1
    print("sync ancestry: exact checkpoint, pinned target, and merge parent verified")
    return 0


if __name__ == "__main__":
    sys.exit(main())
