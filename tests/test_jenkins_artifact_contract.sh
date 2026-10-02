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

# The validator must never publish source or require GitHub write credentials.
evidence="$root/scripts/ci/publish-jenkins-evidence.sh"
if grep -Eq 'git[[:space:]]+push|gh[[:space:]]+api|GIT_ASKPASS|GH_TOKEN|github-https-token' "$evidence"; then
  echo "Jenkins validation must archive evidence without remote writes" >&2
  exit 1
fi
