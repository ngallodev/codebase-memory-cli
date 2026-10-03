#!/usr/bin/env bash
# Archive the successful Jenkins gate as durable, SHA-bound evidence.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

CONTEXT="${CBM_JENKINS_STATUS_CONTEXT:-jenkins/codebase-memory-cli-main/full-gate}"
REVISION="${GIT_COMMIT:-$(git rev-parse HEAD)}"
EVIDENCE_DIR="${CBM_JENKINS_EVIDENCE_DIR:-jenkins-evidence}"

# Validation archives evidence locally. The orchestrator pushes release-tooling
# and publishes the SHA-bound GitHub status only after this job succeeds.
rm -rf "$EVIDENCE_DIR"
mkdir -p "$EVIDENCE_DIR"

printf '%s\n' "$REVISION" > "$EVIDENCE_DIR/source-revision"
if [ -d build/c/test-logs ]; then
    cp -a build/c/test-logs "$EVIDENCE_DIR/"
fi

jq -n \
    --arg commit "$REVISION" \
    --arg job "${JOB_NAME:-codebase-memory-cli-main}" \
    --arg build "${BUILD_NUMBER:-unknown}" \
    --arg url "${BUILD_URL:-}" \
    --arg context "$CONTEXT" \
    '{schema_version: 1, status: "success", source_commit: $commit,
      job: $job, build: $build, build_url: $url, status_context: $context}' \
    > "$EVIDENCE_DIR/qualification-evidence.json"

if [ -x build/c/codebase-memory-cli ]; then
    sha256sum build/c/codebase-memory-cli > "$EVIDENCE_DIR/production-binary.sha256"
fi
find "$EVIDENCE_DIR" -type f ! -name SHA256SUMS -print0 \
    | sort -z | xargs -0 sha256sum > "$EVIDENCE_DIR/SHA256SUMS"

echo "Archived $CONTEXT for $REVISION"
