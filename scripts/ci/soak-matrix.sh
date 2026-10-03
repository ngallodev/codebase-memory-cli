#!/usr/bin/env bash
set -euo pipefail
case "${1:-}" in
-h|--help) echo 'Usage: PLATFORMS=all|linux GITHUB_OUTPUT=... bash scripts/ci/soak-matrix.sh'; exit 0 ;;
'') ;;
*) echo 'Please consult --help.' >&2; exit 2 ;;
esac
UNIX='[
  {"os":"ubuntu-latest","goos":"linux","goarch":"amd64","cc":"gcc","cxx":"g++"},
  {"os":"ubuntu-24.04-arm","goos":"linux","goarch":"arm64","cc":"gcc","cxx":"g++"},
  {"os":"macos-14","goos":"darwin","goarch":"arm64","cc":"cc","cxx":"c++"},
  {"os":"macos-15-intel","goos":"darwin","goarch":"amd64","cc":"cc","cxx":"c++"}
]'
case "$PLATFORMS" in
  all) ;;
  linux) UNIX=$(jq -c 'map(select(.goos == "linux"))' <<<"$UNIX") ;;
  *) echo "::error::unknown platforms input: $PLATFORMS"; exit 1 ;;
esac
echo "unix=$(jq -c '{include: .}' <<<"$UNIX")" >> "$GITHUB_OUTPUT"
