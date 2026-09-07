#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
pipeline="$root/Jenkinsfile"

for required in \
  "Archive Linux CLI artifact" \
  "codebase-memory-cli-linux-amd64" \
  "sha256sum" \
  "source-revision" \
  "archiveArtifacts"; do
  grep -Fq "$required" "$pipeline" || {
    echo "missing Jenkins artifact contract: $required" >&2
    exit 1
  }
done
