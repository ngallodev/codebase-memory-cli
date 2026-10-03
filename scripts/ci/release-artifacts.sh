#!/usr/bin/env bash
set -euo pipefail
# Run in the staged release directory; nullglob permits Linux-only archives.
case "${1:-}" in
-h|--help) echo 'Usage: bash scripts/ci/release-artifacts.sh list|checksums|sign'; exit 0 ;;
list)
  shopt -s nullglob
  ls -la *.tar.gz *.zip *.tsv
  ;;
checksums)
  shopt -s nullglob
  sha256sum \
    *.tar.gz *.zip \
    release-candidates.tsv \
    virustotal-candidate-results.tsv \
    release-selection.tsv \
    > checksums.txt
  ;;
sign)
  shopt -s nullglob
  for f in \
    *.tar.gz *.zip \
    release-candidates.tsv \
    virustotal-candidate-results.tsv \
    release-selection.tsv \
    checksums.txt; do
    cosign sign-blob --yes --bundle "${f}.bundle" "$f"
  done
  ;;
*) echo 'Please consult --help.' >&2; exit 2 ;;
esac
