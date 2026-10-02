#!/usr/bin/env bash
set -euo pipefail
case "${1:-}" in
-h|--help) echo 'Usage: VERSION=... PLATFORMS=... HOLD=... REGISTRIES=... PRERELEASE_PUBLISH=... bash scripts/ci/validate-release-inputs.sh'; exit 0 ;;
'') ;;
*) echo 'Please consult --help.' >&2; exit 2 ;;
esac
fail() { echo "::error::$1"; exit 1; }
if [ "$PRERELEASE_PUBLISH" = "true" ]; then
  [[ "$VERSION" == *-* ]] || fail "publish_github_prerelease requires a prerelease (hyphenated) version, got '$VERSION'"
  [ "$REGISTRIES" != "true" ] || fail "publish_github_prerelease cannot be combined with publish_registries"
  [ "$HOLD" != "true" ] || fail "publish_github_prerelease cannot be combined with hold_for_external_qualification"
fi
if [ "$PLATFORMS" = "linux" ]; then
  [ "$HOLD" != "true" ] || fail "platforms=linux has no Windows archive to qualify; set hold_for_external_qualification=false"
  [ "$REGISTRIES" != "true" ] || fail "platforms=linux cannot publish registry packages (they need every platform)"
fi
echo "input combination OK"
