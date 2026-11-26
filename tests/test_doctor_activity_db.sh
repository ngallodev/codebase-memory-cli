#!/usr/bin/env bash
# test_doctor_activity_db.sh — process-level regression: `doctor --deep` must not
# count the UI activity log (activity.db) or other non-project SQLite files in
# the cache directory as project stores, and must still report a genuinely
# damaged *.db as corrupt.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BINARY="${CBM_TEST_BINARY:-${ROOT}/build/c/codebase-memory-cli}"
[ -x "${BINARY}" ] || { echo "missing test binary: ${BINARY}" >&2; exit 2; }
command -v python3 >/dev/null 2>&1 || { echo "python3 required" >&2; exit 2; }

# shellcheck source=../scripts/test-runtime.sh
source "${ROOT}/scripts/test-runtime.sh"
cbm_test_runtime_init
work="${CBM_TEST_RUNTIME_ROOT}"
cache="${CBM_TEST_CACHE_DIR_HOST}"
cleanup() {
  "${BINARY}" daemon stop >/dev/null 2>&1 || true
  cbm_test_runtime_cleanup "${BINARY}"
}
trap cleanup EXIT

fail() { echo "FAIL: $*" >&2; exit 1; }

repo="${work}/repo"
mkdir -p "${repo}"
printf '%s\n' 'int add(int a, int b) { return a + b; }' >"${repo}/sample.c"
export CBM_ALLOWED_ROOT="${repo}"
(cd "${repo}" && "${BINARY}" index . --mode fast --persistence false >/dev/null)
"${BINARY}" daemon stop >/dev/null 2>&1 || true

# Indexing normally leaves the real activity.db in the cache; create one with
# the same table shapes if it did not, plus a _config.db that the existing name
# rule already skips.
python3 - "${cache}" <<'PY'
import sqlite3, sys
cache = sys.argv[1]
import os
db = sqlite3.connect(cache + "/activity.db")
if db.execute("SELECT count(*) FROM sqlite_master WHERE name='daemon_logs'").fetchone()[0] == 0:
  db.executescript(
      "CREATE TABLE activity_meta(key TEXT PRIMARY KEY, value TEXT NOT NULL);"
      "CREATE TABLE agent_events(id INTEGER PRIMARY KEY AUTOINCREMENT, project TEXT, body TEXT);"
      "CREATE TABLE daemon_logs(id INTEGER PRIMARY KEY AUTOINCREMENT, level TEXT, line TEXT);"
      "INSERT INTO daemon_logs(level, line) VALUES('info', 'x');")
db.commit()
db.close()
cfg = sqlite3.connect(cache + "/_config.db")
cfg.execute("CREATE TABLE IF NOT EXISTS config(key TEXT PRIMARY KEY, value TEXT)")
cfg.commit()
cfg.close()
PY

check() { # label, json, expected "status databases ok corrupt"
  python3 - "$1" "$2" "$3" <<'PY'
import json, sys
label, raw, want = sys.argv[1], sys.argv[2], sys.argv[3].split()
d = json.loads(raw)
s = d["store"]
got = [d["status"], str(s["databases"]), str(s["integrity_ok"]), str(s["corrupt"])]
if got != want:
    sys.exit("FAIL: %s: got %s want %s (%s)" % (label, got, want, raw))
PY
}

ls "${cache}"/*.db >/dev/null
[ -f "${cache}/activity.db" ] || fail "activity.db fixture missing"

out="$("${BINARY}" doctor --deep --json || true)"
check "project store + activity.db" "${out}" "healthy 1 1 0" || exit 1
python3 -c 'import json,sys; assert json.loads(sys.argv[1])["store"]["other_databases"]==1' "${out}" ||
  fail "other_databases should be 1: ${out}"
out="$("${BINARY}" doctor --json || true)"
python3 -c 'import json,sys; s=json.loads(sys.argv[1]); assert s["status"]=="healthy" and s["store"]["databases"]==1 and s["store"]["unreadable"]==0' "${out}" ||
  fail "non-deep doctor miscounted activity.db: ${out}"

# A file that is not a SQLite database must still be reported as corrupt.
printf 'this is not a sqlite database, only garbage bytes\n' >"${cache}/broken.db"
out="$("${BINARY}" doctor --deep --json || true)"
check "garbage broken.db" "${out}" "corrupt 2 1 1" || exit 1
if "${BINARY}" doctor --deep --json >/dev/null 2>&1; then
  fail "doctor --deep must exit non-zero with a corrupt store"
fi

# A store truncated to zero bytes is an empty SQLite file with no tables. It
# must stay a store failure, not be skipped as another application's database.
rm -f "${cache}/broken.db"
: >"${cache}/truncated.db"
out="$("${BINARY}" doctor --deep --json || true)"
check "zero-byte truncated.db" "${out}" "corrupt 2 1 1" || exit 1
echo "doctor activity.db regression passed"
