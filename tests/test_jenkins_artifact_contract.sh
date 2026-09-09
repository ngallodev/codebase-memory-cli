#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
pipeline="$root/Jenkinsfile"
job_script="$root/scripts/jenkins-local-job.sh"

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

for required in "TRIGGER_SOURCE" "post-commit"; do
  grep -Fq "$required" "$pipeline" || {
    echo "missing Jenkins trigger contract: $required" >&2
    exit 1
  }
  grep -Fq "$required" "$job_script" || {
    echo "missing post-commit hook contract: $required" >&2
    exit 1
  }
done
