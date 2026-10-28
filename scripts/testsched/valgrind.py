"""Parse Memcheck logs and render batch-scoped findings."""
from __future__ import annotations

import pathlib
import re
from dataclasses import dataclass, field


PREFIX = re.compile(r"^==(?P<pid>\d+)==(?: ?(?P<body>.*))?$")
SUMMARY = re.compile(
    r"ERROR SUMMARY: (?P<errors>[\d,]+) errors? from (?P<contexts>[\d,]+) contexts?"
)
LOSS_TOTAL = re.compile(
    r"(?P<kind>definitely|indirectly|possibly) lost: "
    r"(?P<bytes>[\d,]+) bytes in (?P<blocks>[\d,]+) blocks"
)
LOSS_RECORD = re.compile(
    r"(?P<bytes>[\d,]+) bytes in (?P<blocks>[\d,]+) blocks are "
    r"(?P<kind>definitely|indirectly|possibly) lost in loss record"
)
FRAME = re.compile(r"\s*(?:at|by) 0x[0-9A-Fa-f]+: (?P<frame>.+)$")
ERROR_START = re.compile(
    r"^(?:Invalid (?:read|write|free)|Conditional jump|Use of uninitialised|"
    r"Use of uninitialized|Syscall param|Mismatched free|Source and destination|"
    r"Fishy value|Jump to the invalid|Address .* is .* allocated|"
    r"Uninitialised value was created|Uninitialized value was created)"
)
LIBC = re.compile(
    r"(?:libc\.so|libpthread|ld-linux|vg_replace|memcheck-amd64|"
    r"__libc_|__GI_|start_thread|clone(?:3)?\s*\()",
    re.IGNORECASE,
)


@dataclass
class StackRecord:
    message: str
    frames: list[str] = field(default_factory=list)


@dataclass
class ProcessReport:
    pid: int
    role: str
    path: pathlib.Path
    errors: int | None = None
    contexts: int | None = None
    leaks: dict[str, tuple[int, int]] = field(default_factory=dict)
    error_stacks: list[StackRecord] = field(default_factory=list)
    leak_stacks: list[StackRecord] = field(default_factory=list)

    @property
    def incomplete(self) -> bool:
        return self.contexts is None

    def has_finding(self) -> bool:
        if self.incomplete:
            return False
        if self.contexts and self.contexts > 0:
            return True
        if self.role == "runner":
            return any(self.leaks.get(kind, (0, 0))[0] > 0 for kind in ("definitely", "indirectly"))
        return False


def _frames(record: StackRecord) -> list[str]:
    return [frame for frame in record.frames if not LIBC.search(frame)][:6]


def parse_log(path: pathlib.Path, runner_pid: int) -> ProcessReport:
    """Read one PID log; missing summaries mean incomplete, not a finding."""
    suffix = path.name.rsplit(".", 1)[-1]
    pid = int(suffix) if suffix.isdigit() else -1
    report = ProcessReport(pid, "runner" if pid == runner_pid else "child", path)
    try:
        lines = path.read_text(errors="replace").splitlines()
    except OSError:
        return report

    active: StackRecord | None = None
    active_kind = ""

    def finish_record() -> None:
        nonlocal active, active_kind
        if active is None:
            return
        if active_kind == "error":
            report.error_stacks.append(active)
        else:
            report.leak_stacks.append(active)
        active = None
        active_kind = ""

    for raw in lines:
        match = PREFIX.match(raw)
        line = match.group("body") if match else raw
        line = line or ""
        summary = SUMMARY.search(line)
        if summary:
            report.errors = int(summary.group("errors").replace(",", ""))
            report.contexts = int(summary.group("contexts").replace(",", ""))
            finish_record()
            continue
        total = LOSS_TOTAL.search(line)
        if total:
            report.leaks[total.group("kind")] = (
                int(total.group("bytes").replace(",", "")),
                int(total.group("blocks").replace(",", "")),
            )
            finish_record()
            continue
        loss = LOSS_RECORD.search(line)
        if loss:
            finish_record()
            active = StackRecord(line)
            active_kind = "leak"
            continue
        if ERROR_START.match(line):
            finish_record()
            active = StackRecord(line)
            active_kind = "error"
            continue
        frame = FRAME.match(line)
        if frame and active is not None:
            active.frames.append(frame.group("frame"))
        elif not line.strip():
            finish_record()
    finish_record()
    return report


def reports_for_batch(log: pathlib.Path, runner_pid: int) -> list[ProcessReport]:
    return [parse_log(path, runner_pid) for path in sorted(log.parent.glob(log.name + ".vg.*"))]


def batch_has_findings(reports: list[ProcessReport]) -> bool:
    return any(report.has_finding() for report in reports)


def finding_records(test: str, reports: list[ProcessReport]) -> list[tuple[str, ProcessReport, StackRecord]]:
    findings = []
    for report in reports:
        if report.incomplete:
            continue
        if report.contexts:
            records = report.error_stacks or [StackRecord("Memcheck error context; stack unavailable")]
            findings.extend((test, report, record) for record in records)
        if report.role == "runner":
            for record in report.leak_stacks:
                if "definitely lost" in record.message or "indirectly lost" in record.message:
                    findings.append((test, report, record))
    return findings


def print_batch(batch_id: str, reports: list[ProcessReport]) -> None:
    print("Valgrind batch | pid role | contexts | definitely lost B/blocks | indirectly lost B/blocks | possibly lost B/blocks | state")
    for report in reports:
        state = "incomplete" if report.incomplete else "complete"
        contexts = "-" if report.contexts is None else str(report.contexts)
        values = [report.leaks.get(kind, (0, 0)) for kind in ("definitely", "indirectly", "possibly")]
        totals = " | ".join(f"{byte_count}/{blocks}" for byte_count, blocks in values)
        print(f"{batch_id} | {report.pid} {report.role} | {contexts} | {totals} | {state}")


def print_findings(findings: list[tuple[str, ProcessReport, StackRecord]]) -> None:
    if not findings:
        print("Valgrind attributed findings: none")
        return
    print("test | pid role | finding | first non-libc frames")
    for test, report, record in findings:
        stack = " <- ".join(_frames(record)) or "stack unavailable"
        print(f"{test} | {report.pid} {report.role} | {record.message} | {stack}")
