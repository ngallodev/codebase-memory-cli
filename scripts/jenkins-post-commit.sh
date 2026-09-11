#!/usr/bin/env bash
# Local-only Jenkins dispatcher. GitHub Actions handle GitHub branches.
set -u
[[ "${CBM_JENKINS_POST_COMMIT:-1}" == 1 ]] || exit 0
root="$(git rev-parse --show-toplevel 2>/dev/null)" || exit 0
branch="$(git -C "$root" branch --show-current 2>/dev/null)"
case "$branch" in
  release-tooling)
    job="${JENKINS_RELEASE_JOB_NAME:-codebase-memory-cli-release-tooling}"
    params=(-p "CBM_TEST_SUITES=${CBM_JENKINS_RELEASE_SUITES:-cli daemon daemon_ipc pipeline}") ;;
  main)
    job="${JENKINS_MAIN_JOB_NAME:-codebase-memory-cli-main}"
    params=() ;;
  *) exit 0 ;;
esac
message="$(git -C "$root" log -1 --format=%B 2>/dev/null)"
case "$message" in *'[skip jenkins]'*|*'[skip ci]'*) exit 0 ;; esac
url="${JENKINS_TARGET_URL:-http://192.168.88.146:8080}"
cli_jar="${JENKINS_CLI:-/tmp/codebase-memory-cli-jenkins-cli.jar}"
[[ -r /home/nate/.config/osint-suite/jenkins.env ]] && source /home/nate/.config/osint-suite/jenkins.env
user="${JENKINS_USER:-${OSINT_JENKINS_USER:-}}"
token="${JENKINS_TOKEN:-${JENKINS_API_TOKEN:-${OSINT_JENKINS_TOKEN:-}}}"
if [[ "${JENKINS_POST_COMMIT_DRY_RUN:-0}" == 1 ]]; then
  printf 'would trigger job=%s branch=%s params=%s\n' "$job" "$branch" "${params[*]:-none}"
  exit 0
fi
[[ -s "$cli_jar" ]] || exit 0
if [[ -n "$user" && -n "$token" ]]; then
  java -jar "$cli_jar" -s "$url" -auth "$user:$token" build "$job" "${params[@]}"
else
  java -jar "$cli_jar" -s "$url" build "$job" "${params[@]}"
fi
