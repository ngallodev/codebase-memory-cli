#!/usr/bin/env python3
"""Test-level scheduler with resumable per-test watchdogs."""
import argparse
import concurrent.futures
import importlib.util
import json
import os
import pathlib
import queue
import re
import shutil
import signal
import subprocess
import sys
import threading
import time
import types

ROOT = pathlib.Path(__file__).resolve().parents[2]
MARK = re.compile(r"^@@cbm-test ([a-z0-9_]+:[a-zA-Z0-9_]+) status=(PASS|FAIL|SKIP) ms=(\d+)$")
MODES = {"asan": 1.0, "plain": 1.0, "valgrind": 25.0, "tsan": 5.0}
DEFAULT = 1.0


def load(path, default):
    try:
        return json.loads(path.read_text())
    except (OSError, ValueError):
        return default


def save(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    temp = path.with_name(f".{path.name}.{os.getpid()}.{threading.get_ident()}.tmp")
    temp.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n")
    try:
        temp.replace(path)
    finally:
        temp.unlink(missing_ok=True)


def terminate(proc):
    global WAVE
    if WAVE is None:
        spec = importlib.util.spec_from_file_location("cbm_test_wave", ROOT / "scripts/run-test-wave.py")
        WAVE = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = WAVE
        spec.loader.exec_module(WAVE)
    WAVE.terminate_process_tree(types.SimpleNamespace(name="test batch", process=proc), 2)


WAVE = None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runner", required=True)
    ap.add_argument("--mode", choices=MODES, default="asan")
    ap.add_argument("--tool", choices=("valgrind",))
    ap.add_argument("--jobs", type=int, default=min(16, os.cpu_count() or 4))
    ap.add_argument("--target", type=float, default=60)
    ap.add_argument("--floor", type=float, default=30)
    ap.add_argument("--factor", type=float, default=5)
    ap.add_argument("--dry-run", action="store_true")
    group = ap.add_mutually_exclusive_group()
    group.add_argument("--suites", nargs="+")
    group.add_argument("--tests", help="comma-separated suite:test names")
    group.add_argument("--changed", nargs="?", const="HEAD")
    group.add_argument("--failed", action="store_true")
    args = ap.parse_args()
    if args.jobs < 1:
        ap.error("--jobs must be positive")
    mode = "valgrind" if args.tool else args.mode
    multiplier = MODES[mode]
    runner = pathlib.Path(args.runner).resolve()
    data_dir = ROOT / "build/test-timings"
    timings = load(data_dir / (mode + ".json"), {"tests": {}})
    tests_db = timings.setdefault("tests", {})
    lock = threading.Lock()
    prior_mode = (data_dir / "last-results-mode").read_text().strip() if (data_dir / "last-results-mode").exists() else ""
    prior_results = load(data_dir / "last-results.json", {}) if prior_mode == mode else {}
    all_names = subprocess.check_output([str(runner), "--list-tests"], text=True).splitlines()
    if args.tests:
        names = args.tests.split(",")
    elif args.suites:
        wanted = set(args.suites)
        known_suites = {n.split(":", 1)[0] for n in all_names}
        unknown_suites = sorted(wanted - known_suites)
        if unknown_suites:
            ap.error("unknown suites: " + ", ".join(unknown_suites))
        names = [n for n in all_names if n.split(":", 1)[0] in wanted]
    elif args.changed is not None:
        changed = data_dir / "changed.txt"
        selected = data_dir / "prediction.json"
        selection = data_dir / "selector.json"
        selection.write_text('{"full": false, "tier": "smart"}\n')
        base = args.changed
        diff = subprocess.run(["git", "diff", "--name-only", base, "--"], cwd=ROOT, text=True, capture_output=True)
        if diff.returncode:
            print("changed diff failed; falling back to all tests", file=sys.stderr)
            names = all_names
        else:
            changed.write_text(diff.stdout)
            cmd = [sys.executable, str(ROOT / "scripts/test-impact/predict.py"), "--root", str(ROOT), "--changed", str(changed), "--selection", str(selection), "--out", str(selected)]
            p = subprocess.run(cmd, cwd=ROOT)
            if p.returncode:
                print("changed selection failed; falling back to all tests", file=sys.stderr)
                names = all_names
            else:
                prediction = load(selected, {})
                selected_tests = prediction.get("suites", {})
                names = []
                for name in all_names:
                    suite, test = name.split(":", 1)
                    selection = selected_tests.get(suite)
                    if selection == "ALL" or isinstance(selection, list) and test in selection:
                        names.append(name)
    elif args.failed:
        previous = load(data_dir / "last-results.json", {})
        names = [n for n, r in previous.items() if r["status"] not in ("PASS", "SKIP")]
    else:
        names = all_names
    if args.tests:
        requested = args.tests.split(",")
        if len(requested) != len(set(requested)):
            ap.error("duplicate test in --tests")
        unknown = sorted(set(requested) - set(all_names))
        if unknown:
            ap.error("unknown tests: " + ", ".join(unknown))
        wanted = set(requested)
        names = [name for name in all_names if name in wanted]
    unknown = sorted(set(names) - set(all_names))
    if unknown:
        ap.error("unknown tests: " + ", ".join(unknown))
    selected_names = set(names)
    names = [name for name in all_names if name in selected_names]
    seeded = False
    for name in names:
        prior = prior_results.get(name, {})
        if prior.get("status") == "TIMEOUT":
            elapsed_ms = max(1, int(prior.get("ms", 0)))
            old = tests_db.get(name, {})
            ema_ms = old.get("ema_ms", int(DEFAULT * 1000))
            if "last_status" not in old:
                ema_ms = int(DEFAULT * 1000)
            tests_db[name] = {"ema_ms": ema_ms, "last_ms": elapsed_ms,
                              "max_ms": max(elapsed_ms, old.get("max_ms", 0)),
                              "last_status": "TIMEOUT"}
            seeded = True
    if seeded:
        save(data_dir / (mode + ".json"), timings)
    if not names:
        print("No tests selected")
        return 0
    def estimate(name):
        with lock:
            if name in tests_db:
                return max(.001, tests_db[name].get("ema_ms", 1000) / 1000)
            suite = name.split(":")[0]
            suite_prior = [v["ema_ms"] for k, v in tests_db.items() if k.startswith(suite + ":")]
        prior_result = prior_results.get(name, {})
        if prior_result.get("status") == "TIMEOUT":
            return max(.001, prior_result.get("ms", 1000) / 1000)
        if suite_prior:
            return sum(suite_prior) / len(suite_prior) / 1000
        return DEFAULT * multiplier
    batches = []
    batch = []
    cost = 0
    last_suite = None
    for name in names:
        suite = name.split(":")[0]
        duration = estimate(name)
        if batch and (suite != last_suite or cost + duration > args.target * multiplier):
            batches.append((batch, cost)); batch = []; cost = 0
        batch.append(name); cost += duration; last_suite = suite
    if batch: batches.append((batch, cost))
    bins = [[] for _ in range(args.jobs)]
    loads = [0.0] * args.jobs
    suite_batches = {}
    for b, cost in batches:
        suite_batches.setdefault(b[0].split(":", 1)[0], []).append((b, cost))
    for suite, sequence in sorted(suite_batches.items(), key=lambda pair: sum(x[1] for x in pair[1]), reverse=True):
        i = min(range(args.jobs), key=loads.__getitem__)
        bins[i].extend(b for b, _ in sequence)
        loads[i] += sum(cost for _, cost in sequence)
    print(f"plan: {len(names)} tests, {len(batches)} batches, {args.jobs} workers, {mode}")
    for i, worker in enumerate(bins):
        print(f"worker {i+1}: {sum(map(len, worker))} tests, estimate {loads[i]:.1f}s")
        for b in worker:
            print(f"  {b[0].split(':')[0]}: {len(b)} tests, {sum(estimate(x) for x in b):.1f}s")
    if args.dry_run:
        return 0
    result_path = data_dir / "last-results.json"
    result = {}
    result_counts = {}
    tool_errors = 0
    valgrind_rows = []
    logs = ROOT / "build/test-timings/logs"
    logs.mkdir(parents=True, exist_ok=True)
    def run_batch(item):
        nonlocal tool_errors
        wi, batch_names, retry = item
        while batch_names:
            suite = batch_names[0].split(":")[0]
            chunk = [n for n in batch_names if n.startswith(suite + ":")]
            remaining = chunk[:]
            print(f"RUN {suite}: {len(chunk)} tests (retry={retry})", flush=True)
            listfile = data_dir / f"selection-{wi}.txt"
            listfile.write_text("\n".join(remaining) + "\n")
            log = logs / f"{suite}-{wi}-{time.time_ns()}.log"
            cmd = [str(runner), suite]
            if args.tool:
                vg = shutil.which("valgrind")
                if not vg:
                    raise RuntimeError("valgrind not found")
                cmd = [vg, "--tool=memcheck", "--leak-check=full", "--show-leak-kinds=all", "--errors-for-leak-kinds=none", "--track-origins=yes", "--num-callers=30", "--error-exitcode=99", f"--log-file={log}.vg.%p", *cmd]
            env = os.environ.copy(); env["CBM_TEST_ONLY_FILE"] = str(listfile); env["CBM_TEST_MACHINE_RESULTS"] = "1"
            creation = {"start_new_session": True} if os.name != "nt" else {"creationflags": subprocess.CREATE_NEW_PROCESS_GROUP}
            with log.open("w") as output:
                proc = subprocess.Popen(cmd, cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1, **creation)
                lines = queue.Queue()
                def pump():
                    for line in proc.stdout: lines.put(line)
                    lines.put(None)
                threading.Thread(target=pump, daemon=True).start()
                last = time.monotonic(); timed_out = False
                while True:
                    try: line = lines.get(timeout=.1)
                    except queue.Empty: line = ""
                    if line is None: break
                    if line:
                        output.write(line); output.flush()
                        m = MARK.match(line.strip())
                        if m:
                            name, status, ms = m.groups(); last = time.monotonic()
                            with lock:
                                result_counts[name] = result_counts.get(name, 0) + 1
                                result[name] = {"status": status, "ms": int(ms), "log": str(log)}
                                if name in remaining:
                                    remaining.remove(name)
                                old = tests_db.get(name, {}).get("ema_ms", int(ms))
                                tests_db[name] = {"ema_ms": round(old * .7 + int(ms) * .3), "last_ms": int(ms), "max_ms": max(int(ms), tests_db.get(name, {}).get("max_ms", 0)), "last_status": status}
                                save(data_dir / (mode + ".json"), timings)
                    if proc.poll() is None and remaining:
                        active = remaining[0]
                        timeout = max(args.floor * multiplier, args.factor * estimate(active))
                        if time.monotonic() - last > timeout:
                            timed_out = True; terminate(proc); break
                rc = proc.wait()
            print(f"BATCH {suite}: {len([n for n in chunk if n in result])}/{len(chunk)} results, exit={rc}", flush=True)
            if args.tool:
                for vg_log in sorted(log.parent.glob(log.name + ".vg.*")):
                    content = vg_log.read_text(errors="replace")
                    pid = vg_log.name.rsplit(".", 1)[-1]
                    summary = re.search(r"ERROR SUMMARY: (\d+) errors?", content)
                    leak_rows = re.findall(r"(definitely lost|indirectly lost|possibly lost): ([\d,]+) bytes in ([\d,]+) blocks", content)
                    leaks = {kind: (int(n.replace(",", "")), int(blocks.replace(",", ""))) for kind, n, blocks in leak_rows}
                    errors = int(summary.group(1)) if summary else -1
                    with lock:
                        tool_errors += errors if errors >= 0 else 1
                        role = "runner" if pid == str(proc.pid) else "child"
                        for name in chunk:
                            valgrind_rows.append((name, role, pid, errors,
                                                  leaks.get("definitely lost", (0, 0)),
                                                  leaks.get("indirectly lost", (0, 0)),
                                                  leaks.get("possibly lost", (0, 0))))
            done = [n for n in chunk if n in result]
            remaining = [n for n in remaining if n not in result]
            if timed_out:
                active = remaining.pop(0) if remaining else (chunk[len(done)] if len(done) < len(chunk) else None)
                if active:
                    elapsed_ms = int((time.monotonic() - last) * 1000)
                    with lock:
                        result[active] = {"status": "TIMEOUT", "ms": elapsed_ms, "log": str(log)}
                        result_counts[active] = result_counts.get(active, 0) + 1
                        old = tests_db.get(active, {})
                        tests_db[active] = {"ema_ms": old.get("ema_ms", int(DEFAULT * 1000)), "last_ms": elapsed_ms, "max_ms": max(elapsed_ms, old.get("max_ms", 0)), "last_status": "TIMEOUT"}
                        save(data_dir / (mode + ".json"), timings)
                    print(f"TIMEOUT {active} ({log})", flush=True)
                batch_names = remaining + [n for n in batch_names if n not in chunk]
            elif rc and remaining:
                active = remaining.pop(0)
                result[active] = {"status": "CRASH", "ms": 0, "log": str(log)}
                result_counts[active] = result_counts.get(active, 0) + 1
                print(f"CRASH {active} ({log})", flush=True)
                batch_names = remaining + [n for n in batch_names if n not in chunk]
            else:
                batch_names = [n for n in batch_names if n not in chunk]
            if remaining and retry >= 1:
                for n in remaining:
                    if n not in result:
                        result[n] = {"status": "CRASH", "ms": 0, "log": str(log)}
                        result_counts[n] = result_counts.get(n, 0) + 1
                remaining = []
        return

    # Workers consume preplanned batches; a failed batch resumes its suffix inline.
    def run_worker(wi, worker_batches):
        for batch_names in worker_batches:
            run_batch((wi, batch_names, 0))
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        list(pool.map(lambda x: run_worker(*x), enumerate(bins)))
    for name in names:
        if name not in result:
            result[name] = {"status": "CRASH", "ms": 0, "log": "no result"}
            result_counts[name] = result_counts.get(name, 0) + 1
    save(result_path, result)
    (data_dir / "last-results-mode").write_text(mode + "\n")
    counts = {s: sum(r["status"] == s for r in result.values()) for s in ("PASS", "FAIL", "SKIP", "TIMEOUT", "CRASH")}
    print(f"{counts['PASS']} passed, {counts['FAIL']} failed, {counts['SKIP']} skipped, {counts['TIMEOUT']} timeouts, {counts['CRASH']} crashes")
    if args.tool and valgrind_rows:
        print("Valgrind PID summary (PID metrics are batch-scoped):")
        print("test | process | pid | errors | definitely lost bytes/blocks | indirectly lost bytes/blocks | possibly lost bytes/blocks")
        for name, role, pid, errors, definite, indirect, possible in valgrind_rows:
            print(f"{name} | {role} | {pid} | {errors} | {definite[0]}/{definite[1]} | {indirect[0]}/{indirect[1]} | {possible[0]}/{possible[1]}")
    if args.tool and tool_errors:
        print(f"valgrind reported {tool_errors} error context(s) across batch process logs")
    elapsed = time.monotonic() - start_all
    print(f"wall time: {elapsed:.1f}s")
    bad = [n for n in names if result[n]["status"] not in ("PASS", "SKIP")]
    duplicates = [n for n in names if result_counts.get(n, 0) > 1]
    missing = [n for n in names if result_counts.get(n, 0) == 0]
    if len(result) != len(set(names)) or bad or duplicates or missing or tool_errors:
        print("non-passing tests:")
        for name in bad: print(f"  {name}: {result[name]['status']} ({result[name]['log']})")
        for name in duplicates: print(f"  {name}: duplicate result")
        for name in missing: print(f"  {name}: missing result")
        return 1
    return 0


if __name__ == "__main__":
    start_all = time.monotonic()
    try: raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as exc:
        print(f"testsched: {exc}", file=sys.stderr); raise SystemExit(2)
