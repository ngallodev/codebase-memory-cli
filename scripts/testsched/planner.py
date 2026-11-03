"""Build setup-aware consecutive batches and expose their timing plan."""
from __future__ import annotations

from dataclasses import dataclass


# Add a suite only after a full split run proves that parallel batches break
# its order/state assumptions. Each entry needs a short reason.
SERIAL_SUITES: dict[str, str] = {
    "daemon_application": "The 10-second default-capacity admission fixture timed out in the split tail batch under full-run load.",
}


@dataclass(frozen=True)
class Batch:
    suite: str
    tests: tuple[str, ...]
    setup_ms: float
    work_ms: float

    @property
    def estimate_ms(self) -> float:
        return self.setup_ms + self.work_ms


def plan_batches(
    names: list[str], timing, target_seconds: float, multiplier: float
) -> list[Batch]:
    """Keep registration order within each suite and account for setup cost."""
    by_suite: dict[str, list[str]] = {}
    for name in names:
        suite = name.split(":", 1)[0]
        by_suite.setdefault(suite, []).append(name)

    batches = []
    target_ms = target_seconds * multiplier * 1000
    for suite, tests in by_suite.items():
        setup_ms = timing.setup_ms(suite)
        if suite in SERIAL_SUITES:
            batches.append(
                Batch(suite, tuple(tests), setup_ms, sum(timing.estimate_ms(n) for n in tests))
            )
            continue

        # Keep setup at most one quarter of the estimate, even when setup cost
        # pushes the minimum batch above the requested target.
        minimum_work_ms = 3 * setup_ms
        target_work_ms = max(target_ms - setup_ms, minimum_work_ms)
        current: list[str] = []
        work_ms = 0.0
        for name in tests:
            test_ms = timing.estimate_ms(name)
            if current and work_ms >= target_work_ms:
                batches.append(Batch(suite, tuple(current), setup_ms, work_ms))
                current = []
                work_ms = 0.0
            current.append(name)
            work_ms += test_ms
        if current:
            if batches and batches[-1].suite == suite and work_ms < minimum_work_ms:
                previous = batches.pop()
                current = list(previous.tests) + current
                work_ms += previous.work_ms
            batches.append(Batch(suite, tuple(current), setup_ms, work_ms))
    return batches


def ordered_batches(batches: list[Batch]) -> list[tuple[int, Batch]]:
    """Return stable longest-processing-time-first queue priorities."""
    return sorted(enumerate(batches), key=lambda item: (-item[1].estimate_ms, item[0]))


def print_plan(names: list[str], batches: list[Batch], jobs: int, mode: str) -> None:
    suites = {}
    for batch in batches:
        suites[batch.suite] = batch.setup_ms
    print(f"plan: {len(names)} tests, {len(batches)} batches, {jobs} workers, {mode}")
    for suite in sorted(suites):
        print(f"setup {suite}: {suites[suite]:.0f}ms")
    for index, batch in ordered_batches(batches):
        print(
            f"batch {index + 1}: {batch.suite}, {len(batch.tests)} tests, "
            f"setup {batch.setup_ms:.0f}ms, work {batch.work_ms:.0f}ms, "
            f"estimate {batch.estimate_ms:.0f}ms"
        )
