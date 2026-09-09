#!/usr/bin/env python3
"""Validate benchmark-corpus checkout identity before freeze or qualification.

This is intentionally stdlib-only. It validates repository identity, cleanliness,
version evidence, initialization refs, and pinned commits from
``docs/qualification/BENCHMARK_CORPUS.json``.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tomllib
from pathlib import Path
from urllib.parse import urlparse

UNSET_COMMIT = "TO_BE_PINNED_FROM_FROZEN_CHECKOUT"
UNSET_PATH = "TO_BE_SET_ON_LUIGI"


def fail(message: str) -> None:
    raise ValueError(message)


def run_git(repo: Path, *args: str) -> str:
    proc = subprocess.run(
        ["git", "-C", str(repo), *args],
        check=False,
        capture_output=True,
        text=True,
    )
    if proc.returncode != 0:
        detail = proc.stderr.strip() or proc.stdout.strip() or f"exit {proc.returncode}"
        fail(f"{repo}: git {' '.join(args)} failed: {detail}")
    return proc.stdout.strip()


def normalize_remote(value: str) -> str:
    value = value.strip().rstrip("/")
    if value.endswith(".git"):
        value = value[:-4]
    if value.startswith("git@github.com:"):
        value = "https://github.com/" + value.removeprefix("git@github.com:")
    parsed = urlparse(value)
    if parsed.scheme and parsed.hostname:
        host = parsed.hostname.lower()
        port = f":{parsed.port}" if parsed.port else ""
        return f"{host}{port}{parsed.path}".rstrip("/")
    return value.lower()


def pyproject_version(repo: Path, relative_path: str) -> str:
    path = repo / relative_path
    if not path.is_file():
        fail(f"{repo}: missing version evidence file {relative_path}")
    with path.open("rb") as handle:
        doc = tomllib.load(handle)
    try:
        version = doc["project"]["version"]
    except (KeyError, TypeError):
        fail(f"{repo}: {relative_path} has no [project].version")
    if not isinstance(version, str) or not version:
        fail(f"{repo}: {relative_path} [project].version is not a non-empty string")
    return version


def validate_version(repo: Path, entry: dict[str, object]) -> None:
    expected = entry.get("expected_release_version")
    check = entry.get("version_check")
    if expected is None and check is None:
        return
    if not isinstance(expected, str) or not expected:
        fail(f"{entry.get('id')}: expected_release_version must be a non-empty string")
    if not isinstance(check, dict):
        fail(f"{entry.get('id')}: version_check is required with expected_release_version")

    kind = check.get("kind")
    if kind == "pyproject_project_version":
        path = check.get("path", "pyproject.toml")
        if not isinstance(path, str) or not path:
            fail(f"{entry.get('id')}: version_check.path must be a non-empty string")
        actual = pyproject_version(repo, path)
        if actual != expected:
            fail(f"{entry.get('id')}: expected version {expected}, found {actual} in {path}")
        return

    if kind == "exact_git_tag":
        tag = check.get("tag")
        if not isinstance(tag, str) or not tag:
            fail(f"{entry.get('id')}: exact_git_tag requires tag")
        expected_tag = f"v{expected}"
        if tag != expected_tag:
            fail(f"{entry.get('id')}: version_check.tag {tag} does not match {expected_tag}")
        tag_commit = run_git(repo, "rev-list", "-n", "1", tag)
        head = run_git(repo, "rev-parse", "HEAD")
        if tag_commit != head:
            fail(f"{entry.get('id')}: HEAD {head} is not exact tag {tag} ({tag_commit})")
        return

    fail(f"{entry.get('id')}: unsupported version_check.kind {kind!r}")




def validate_workload(repo: Path, entry: dict[str, object]) -> None:
    operations = entry.get("operations")
    if not isinstance(operations, list) or not operations or not all(isinstance(v, str) and v for v in operations):
        fail(f"{entry.get('id')}: operations must be a non-empty string list")
    required = {"index", "search", "architecture", "snippet", "outline", "changes", "status"}
    missing = sorted(required.difference(operations))
    if missing:
        fail(f"{entry.get('id')}: required benchmark operations missing: {', '.join(missing)}")
    workload = entry.get("workload")
    if not isinstance(workload, dict):
        fail(f"{entry.get('id')}: workload object is required")
    for key in ("query", "symbol", "file_path", "base_branch"):
        value = workload.get(key)
        if not isinstance(value, str) or not value:
            fail(f"{entry.get('id')}: workload.{key} must be a non-empty string")
    file_path = workload["file_path"]
    if Path(file_path).is_absolute() or ".." in Path(file_path).parts:
        fail(f"{entry.get('id')}: workload.file_path must be repository-relative")
    if not (repo / file_path).is_file():
        fail(f"{entry.get('id')}: workload file does not exist at frozen checkout: {file_path}")

def validate_entry(entry: dict[str, object], allow_unpinned: bool) -> list[str]:
    repo_id = entry.get("id")
    if not isinstance(repo_id, str) or not repo_id:
        fail("repository entry has invalid id")

    local_path = entry.get("local_path")
    if not isinstance(local_path, str) or not local_path or local_path == UNSET_PATH:
        fail(f"{repo_id}: local_path is not initialized")
    repo = Path(local_path).expanduser().resolve()
    if not repo.is_dir():
        fail(f"{repo_id}: local_path is not a directory: {repo}")
    if run_git(repo, "rev-parse", "--is-inside-work-tree") != "true":
        fail(f"{repo_id}: local_path is not a git work tree: {repo}")

    expected_remote = entry.get("remote")
    if not isinstance(expected_remote, str) or not expected_remote:
        fail(f"{repo_id}: remote is missing")
    actual_remote = run_git(repo, "remote", "get-url", "origin")
    if normalize_remote(actual_remote) != normalize_remote(expected_remote):
        fail(f"{repo_id}: origin mismatch: expected {expected_remote}, found {actual_remote}")

    if entry.get("clean_required") is True:
        status = run_git(repo, "status", "--porcelain=v1", "--untracked-files=normal")
        if status:
            fail(f"{repo_id}: checkout is not clean")

    head = run_git(repo, "rev-parse", "HEAD")
    pinned = entry.get("commit")
    initialization_ref = entry.get("initialization_ref")
    if initialization_ref is not None and pinned == UNSET_COMMIT:
        if not isinstance(initialization_ref, str) or not initialization_ref:
            fail(f"{repo_id}: initialization_ref must be a non-empty string")
        ref_commit = run_git(repo, "rev-parse", f"{initialization_ref}^{{commit}}")
        if ref_commit != head:
            fail(f"{repo_id}: HEAD {head} does not match initialization_ref {initialization_ref} ({ref_commit})")

    validate_version(repo, entry)
    validate_workload(repo, entry)

    if not isinstance(pinned, str) or not pinned:
        fail(f"{repo_id}: commit is missing")
    if pinned == UNSET_COMMIT:
        if not allow_unpinned:
            fail(f"{repo_id}: commit is not frozen")
    elif pinned != head:
        fail(f"{repo_id}: pinned commit {pinned} does not match HEAD {head}")

    return [f"{repo_id}: {head}"]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "manifest",
        nargs="?",
        default="docs/qualification/BENCHMARK_CORPUS.json",
        help="benchmark corpus manifest",
    )
    parser.add_argument(
        "--allow-unpinned",
        action="store_true",
        help="allow TO_BE_PINNED_FROM_FROZEN_CHECKOUT while initializing a corpus",
    )
    args = parser.parse_args()

    manifest_path = Path(args.manifest)
    with manifest_path.open("r", encoding="utf-8") as handle:
        manifest = json.load(handle)

    if manifest.get("schema") != "codebase-memory-cli/benchmark-corpus/v1":
        fail(f"unsupported benchmark corpus schema: {manifest.get('schema')!r}")
    repositories = manifest.get("repositories")
    if not isinstance(repositories, list) or not repositories:
        fail("manifest repositories must be a non-empty list")

    summaries: list[str] = []
    for raw in repositories:
        if not isinstance(raw, dict):
            fail("repository entries must be objects")
        summaries.extend(validate_entry(raw, args.allow_unpinned))

    for summary in summaries:
        print(f"PASS {summary}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, json.JSONDecodeError, tomllib.TOMLDecodeError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(2)
