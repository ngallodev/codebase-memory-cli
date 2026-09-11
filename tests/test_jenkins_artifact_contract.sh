#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
pipeline="$root/Jenkinsfile"

for required in \
  "stage('Lint')" \
  "stage('Memory lint')" \
  "stage('Security static')" \
  "stage('Install Python CI tools')" \
  "stage('License gate')" \
  "stage('Package wrappers')" \
  "stage('Thread sanitizer')" \
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

grep -Fq 'requirements-ci.txt' "$root/scripts/ci/install-python-tools.sh" || {
  echo "missing CI Python requirements contract" >&2
  exit 1
}
