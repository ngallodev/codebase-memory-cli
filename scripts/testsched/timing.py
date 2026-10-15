"""Thread-safe, mode-specific timing and setup history."""
from __future__ import annotations

import json
import os
import pathlib
import threading


class TimingDB:
    """Store test durations and process setup cost using atomic snapshots."""

    def __init__(self, directory: pathlib.Path, mode: str, multiplier: float):
        self.directory = directory
        self.mode = mode
        self.multiplier = multiplier
        self.path = directory / f"{mode}.json"
        self.lock = threading.RLock()
        self.data = self._load()

    def _load(self) -> dict:
        try:
            data = json.loads(self.path.read_text())
        except (OSError, ValueError):
            data = {}
        data.setdefault("tests", {})
        data.setdefault("suites", {})
        return data

    def _save(self) -> None:
        self.directory.mkdir(parents=True, exist_ok=True)
        temporary = self.path.with_name(
            f".{self.path.name}.{os.getpid()}.{threading.get_ident()}.tmp"
        )
        temporary.write_text(json.dumps(self.data, indent=2, sort_keys=True) + "\n")
        try:
            temporary.replace(self.path)
        finally:
            temporary.unlink(missing_ok=True)

    def test(self, name: str) -> dict | None:
        with self.lock:
            value = self.data["tests"].get(name)
            return dict(value) if value else None

    def tests_for_suite(self, suite: str) -> list[dict]:
        prefix = suite + ":"
        with self.lock:
            return [
                dict(value)
                for name, value in self.data["tests"].items()
                if name.startswith(prefix) and value.get("ema_ms", 0) > 0
            ]

    def estimate_ms(self, name: str) -> float:
        suite = name.split(":", 1)[0]
        with self.lock:
            value = self.data["tests"].get(name)
            if value and value.get("ema_ms", 0) > 0:
                return float(value["ema_ms"])
            values = [
                item.get("ema_ms", 0)
                for key, item in self.data["tests"].items()
                if key.startswith(suite + ":") and item.get("ema_ms", 0) > 0
            ]
        if values:
            return sum(values) / len(values)
        return 1000.0 * self.multiplier

    def timeout_budget_ms(
        self,
        name: str,
        floor_seconds: float,
        factor: float,
        legacy_seconds: int,
    ) -> float:
        value = self.test(name)
        if value:
            history_ms = max(
                float(value.get("ema_ms", 0)),
                float(value.get("max_ms", 0)),
            )
            return max(floor_seconds * self.multiplier * 1000, factor * history_ms)
        return legacy_seconds * self.multiplier * 1000

    def setup_ms(self, suite: str) -> float:
        with self.lock:
            value = self.data["suites"].get(suite, {}).get("setup_ms")
        if value is not None and value > 0:
            return float(value)
        return 1000.0 * self.multiplier

    def record_test(self, name: str, status: str, duration_ms: int) -> None:
        duration_ms = max(1, int(duration_ms))
        with self.lock:
            old = self.data["tests"].get(name, {})
            ema = old.get("ema_ms", duration_ms)
            self.data["tests"][name] = {
                "ema_ms": round(ema * 0.7 + duration_ms * 0.3),
                "last_ms": duration_ms,
                "max_ms": max(duration_ms, int(old.get("max_ms", 0))),
                "last_status": status,
            }
            self._save()

    def record_setup(self, suite: str, duration_ms: int) -> None:
        duration_ms = max(0, int(duration_ms))
        with self.lock:
            old = self.data["suites"].get(suite, {})
            ema = old.get("setup_ms", duration_ms)
            self.data["suites"][suite] = {
                "setup_ms": round(ema * 0.7 + duration_ms * 0.3),
                "last_setup_ms": duration_ms,
                "max_setup_ms": max(duration_ms, int(old.get("max_setup_ms", 0))),
            }
            self._save()

    def save(self) -> None:
        with self.lock:
            self._save()
