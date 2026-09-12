#!/usr/bin/env bash
# Publish the successful Jenkins gate as durable, SHA-bound evidence.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

: "${GH_TOKEN:?GH_TOKEN is required to publish Jenkins evidence}"
REPOSITORY="${GITHUB_REPOSITORY:-ngallodev/codebase-memory-cli}"
CONTEXT="${CBM_JENKINS_STATUS_CONTEXT:-jenkins/codebase-memory-cli-main/full-gate}"
REVISION="${GIT_COMMIT:-$(git rev-parse HEAD)}"
EVIDENCE_DIR="${CBM_JENKINS_EVIDENCE_DIR:-jenkins-evidence}"

# Jenkins builds this checkout from the local working tree, so publish the
# tested revision before asking GitHub to attach a SHA-bound status to it.
PUSH_ASKPASS="$(mktemp)"
trap 'rm -f "$PUSH_ASKPASS"' EXIT
# shellcheck disable=SC2016 # The helper must expand its own prompt and token.
printf '%s\n' '#!/bin/sh' 'case "$1" in' \
    '*Username*) printf "%s\\n" x-access-token ;;' \
    '*Password*) printf "%s\\n" "$GH_TOKEN" ;;' \
    '*) exit 1 ;;' 'esac' > "$PUSH_ASKPASS"
chmod 700 "$PUSH_ASKPASS"
GIT_ASKPASS="$PUSH_ASKPASS" GIT_TERMINAL_PROMPT=0 \
    git push "https://github.com/$REPOSITORY.git" 'HEAD:refs/heads/main'

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

GH_TOKEN="$GH_TOKEN" gh api --method POST \
    "repos/$REPOSITORY/statuses/$REVISION" \
    -f state=success \
    -f context="$CONTEXT" \
    -f description="Jenkins full gate passed; SHA-bound evidence archived in build ${BUILD_NUMBER:-unknown}" \
    -f target_url="${BUILD_URL:-}" \
    >/dev/null

echo "Published $CONTEXT for $REVISION"
