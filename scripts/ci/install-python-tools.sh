#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
VENV="${CBM_CI_VENV:-$ROOT/.ci-venv}"

command -v python3 >/dev/null 2>&1 || {
    echo "install-python-tools: python3 is required" >&2
    exit 2
}

if [[ ! -x "$VENV/bin/python" ]]; then
    rm -rf "$VENV"
    python3 -m venv "$VENV" || {
        echo "install-python-tools: python3-venv is required" >&2
        exit 2
    }
fi

"$VENV/bin/python" -m pip install \
    --disable-pip-version-check \
    --requirement "$ROOT/requirements-ci.txt"

echo "CI Python tools installed in $VENV"
