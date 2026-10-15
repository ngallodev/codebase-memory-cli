#!/usr/bin/env python3
"""Run selected C tests in resumable, setup-aware batches."""
from __future__ import annotations

import argparse
import concurrent.futures
import importlib.util
import os
import pathlib
import queue
import re
import shutil
import subprocess
import sys
import threading
import time

from planner import ordered_batches, plan_batches, print_plan
from selection import select_tests
from timing import TimingDB
import valgrind

ROOT = pathlib.Path(__file__).resolve().parents[2]
MARK = re.compile(r"^@@cbm-test ([a-z0-9_]+:[a-zA-Z0-9_]+) status=(START|PASS|FAIL|SKIP) ms=(\d+)$")
MULTIPLIER = {"asan": 1, "plain": 1, "valgrind": 25, "tsan": 5}


def legacy_budgets() -> tuple[frozenset[str], int, int]:
    """Read the wave runner's slow-suite set and its configurable budgets."""
    path = ROOT / "scripts/run-test-wave.py"
    spec = importlib.util.spec_from_file_location("cbm_test_wave", path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    slow = module.SLOW_SUITES
    return (slow, int(os.getenv("CBM_SUITE_TIMEOUT", "900")),
            int(os.getenv("CBM_SUITE_TIMEOUT_SLOW", "3600")))


def kill(proc: subprocess.Popen) -> None:
    if os.name == "nt":
        if proc.poll() is None:
            proc.kill()
    else:
        import signal
        try:
            os.killpg(proc.pid, signal.SIGTERM)
        except ProcessLookupError:
            return
        try:
            proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            pass
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass


def main() -> int:
    run_started = time.monotonic()
    parser = argparse.ArgumentParser()
    parser.add_argument("--runner", required=True)
    parser.add_argument("--mode", choices=MULTIPLIER, default="asan")
    parser.add_argument("--tool", choices=("valgrind",))
    parser.add_argument("--jobs", type=int, default=min(16, os.cpu_count() or 4))
    parser.add_argument("--target", type=float, default=60)
    parser.add_argument("--floor", type=float, default=120)
    parser.add_argument("--factor", type=float, default=5)
    parser.add_argument("--dry-run", action="store_true")
    attr = parser.add_mutually_exclusive_group()
    attr.add_argument("--attribute", dest="attribute", action="store_true")
    attr.add_argument("--no-attribute", dest="attribute", action="store_false")
    parser.set_defaults(attribute=None)
    select = parser.add_mutually_exclusive_group()
    select.add_argument("--suites", nargs="+")
    select.add_argument("--tests")
    select.add_argument("--changed", nargs="?", const="HEAD")
    select.add_argument("--failed", action="store_true")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    mode = "valgrind" if args.tool else args.mode
    multiplier = MULTIPLIER[mode]
    attribute = args.tool == "valgrind" if args.attribute is None else args.attribute
    runner = pathlib.Path(args.runner).resolve()
    data_dir = ROOT / "build/test-timings"
    timing = TimingDB(data_dir, mode, multiplier)
    all_tests = subprocess.check_output([str(runner), "--list-tests"], text=True).splitlines()
    try:
        names = select_tests(args, all_tests, data_dir, ROOT)
    except ValueError as error:
        parser.error(str(error))
    previous = {}
    if (data_dir / "last-results-mode").exists() and (data_dir / "last-results-mode").read_text().strip() == mode:
        try:
            import json
            previous = json.loads((data_dir / "last-results.json").read_text())
        except (OSError, ValueError):
            pass
    for name in names:
        if previous.get(name, {}).get("status") == "TIMEOUT" and timing.test(name) is None:
            timing.record_test(name, "TIMEOUT", previous[name].get("ms", 1000))
    batches = plan_batches(names, timing, args.target, multiplier)
    print_plan(names, batches, args.jobs, mode)
    if args.dry_run:
        return 0
    if not names:
        print("No tests selected")
        return 0

    logs = ROOT / "build/test-timings/logs"
    logs.mkdir(parents=True, exist_ok=True)
    results: dict[str, dict] = {}
    reports: list[tuple[str, tuple[str, ...], list[valgrind.ProcessReport], bool]] = []
    slow_suites, normal_budget, slow_budget = legacy_budgets()
    lock = threading.RLock()
    active: set[subprocess.Popen] = set()
    stop = threading.Event()
    ordered = ordered_batches(batches)
    pending: queue.Queue = queue.Queue()
    for index, batch in ordered:
        pending.put((index, batch, batch.tests))
    for _ in range(args.jobs):
        pending.put(None)

    def run_one(index: int, batch, tests: tuple[str, ...], attribution=False) -> None:
        """Run one isolated suite batch and resume its suffix after a timeout."""
        suite = batch.suite
        print(f"RUN batch {index + 1}: {suite} ({len(tests)} tests)", flush=True)
        listfile = data_dir / f"selection-{os.getpid()}-{threading.get_ident()}.txt"
        listfile.write_text("\n".join(tests) + "\n")
        log = logs / f"{suite}-{index}-{time.time_ns()}.log"
        command = [str(runner), suite]
        if args.tool:
            executable = shutil.which("valgrind")
            if not executable:
                raise RuntimeError("valgrind not found")
            command = [executable, "--tool=memcheck", "--leak-check=full", "--show-leak-kinds=all",
                       "--errors-for-leak-kinds=none", "--track-origins=yes", "--num-callers=30",
                       "--error-exitcode=99", f"--log-file={log}.vg.%p", *command]
        env = os.environ.copy()
        env.update(CBM_TEST_ONLY_FILE=str(listfile), CBM_TEST_MACHINE_RESULTS="1")
        start = time.monotonic()
        creation = {"start_new_session": True} if os.name != "nt" else {}
        with log.open("w") as output:
            proc = subprocess.Popen(command, cwd=ROOT, env=env, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, text=True, bufsize=1, **creation)
            with lock:
                active.add(proc)
            lines: queue.Queue = queue.Queue()
            def pump():
                for line in proc.stdout:
                    lines.put(line)
                lines.put(None)
            threading.Thread(target=pump, daemon=True).start()
            started: dict[str, float] = {}
            got: set[str] = set()
            timeout_name = None
            while True:
                try:
                    line = lines.get(timeout=.1)
                except queue.Empty:
                    line = ""
                if line is None:
                    break
                if line:
                    output.write(line)
                    output.flush()
                    match = MARK.match(line.strip())
                    if match:
                        name, status, ms = match.groups()
                        now = time.monotonic()
                        if status == "START":
                            started[name] = now
                        else:
                            got.add(name)
                            timing.record_test(name, status, int(ms))
                            with lock:
                                results[name] = {"status": status, "ms": int(ms), "log": str(log)}
                if proc.poll() is None and not stop.is_set():
                    running = next((n for n in tests if n in started and n not in got), None)
                    if running:
                        budget = timing.timeout_budget_ms(
                            running, args.floor, args.factor,
                            (slow_budget if running.split(":", 1)[0] in slow_suites else normal_budget),
                        )
                        if (time.monotonic() - started[running]) * 1000 > budget:
                            timeout_name = running
                            kill(proc)
                            break
                    elif not got and tests:
                        first = tests[0]
                        setup_budget = (slow_budget if suite in slow_suites else normal_budget) * multiplier
                        if (time.monotonic() - start) > setup_budget:
                            timeout_name = first
                            kill(proc)
                            break
            rc = proc.wait()
            with lock:
                active.discard(proc)
        batch_ms = int((time.monotonic() - start) * 1000)
        test_ms = sum(int(results[n]["ms"]) for n in got if n in results)
        if timeout_name in started:
            test_ms += int((time.monotonic() - started[timeout_name]) * 1000)
        timing.record_setup(suite, max(0, batch_ms - test_ms))
        if timeout_name:
            elapsed = int((time.monotonic() - started.get(timeout_name, start)) * 1000)
            timing.record_test(timeout_name, "TIMEOUT", elapsed)
            with lock:
                results[timeout_name] = {"status": "TIMEOUT", "ms": elapsed, "log": str(log)}
            suffix = tuple(name for name in tests if name not in got and name != timeout_name)
            if suffix:
                run_one(index + 1_000_000, batch, suffix, attribution)
            return
        for name in tests:
            if name not in got and name != timeout_name:
                with lock:
                    results.setdefault(name, {"status": "CRASH", "ms": 0, "log": str(log)})
        if args.tool:
            parsed = valgrind.reports_for_batch(log, proc.pid)
            reports.append((f"{suite}#{index + 1}", tests, parsed, attribution))
            valgrind.print_batch(f"{suite}#{index + 1}", parsed)
            if attribute and valgrind.batch_has_findings(parsed) and not attribution:
                for offset, test in enumerate(tests):
                    run_one(index * 10000 + offset, batch, (test,), True)
        print(f"DONE batch {index + 1}: {suite} ({len(got)}/{len(tests)} results)", flush=True)
        if rc and not timeout_name:
            print(f"batch {suite} exited {rc}; see {log}")

    def worker() -> None:
        """Consume globally prioritized batches until the queue or run ends."""
        while not stop.is_set():
            item = pending.get()
            if item is None:
                return
            index, batch, tests = item
            run_one(index, batch, tests)

    interrupted = False
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        try:
            futures = [pool.submit(worker) for _ in range(args.jobs)]
            for future in futures:
                future.result()
        except KeyboardInterrupt:
            interrupted = True
            stop.set()
            with lock:
                for proc in list(active):
                    kill(proc)
            for future in futures:
                future.cancel()
    for name in names:
        results.setdefault(name, {"status": "INTERRUPTED" if interrupted else "CRASH", "ms": 0, "log": "no result"})
    import json
    data_dir.mkdir(parents=True, exist_ok=True)
    (data_dir / "last-results.json").write_text(json.dumps(results, indent=2, sort_keys=True) + "\n")
    (data_dir / "last-results-mode").write_text(mode + "\n")
    counts = {status: sum(row["status"] == status for row in results.values())
              for status in ("PASS", "FAIL", "SKIP", "TIMEOUT", "CRASH", "INTERRUPTED")}
    print(", ".join(f"{count} {status.lower()}" for status, count in counts.items()))
    print(f"scheduler wall time: {time.monotonic() - run_started:.1f}s")
    if args.tool:
        findings = []
        incomplete = 0
        for batch_id, tests_in_batch, parsed, attributed in reports:
            incomplete += sum(report.incomplete for report in parsed)
            if attributed and attribute:
                for test in tests_in_batch:
                    findings.extend(valgrind.finding_records(test, parsed))
            elif not attribute:
                findings.extend(valgrind.finding_records(f"{batch_id} (unattributed)", parsed))
        print(f"Valgrind incomplete process logs: {incomplete}")
        valgrind.print_findings(findings)
    tool_bad = args.tool and any(valgrind.batch_has_findings(parsed) for _, _, parsed, _ in reports)
    return 130 if interrupted else int(
        tool_bad or any(row["status"] not in ("PASS", "SKIP") for row in results.values())
    )


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"testsched: {error}", file=sys.stderr)
        raise SystemExit(2)
