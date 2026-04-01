"""Timed TCP connect probe."""

from __future__ import annotations

import socket
import time


def probe_tcp(host: str, port: int, timeout_sec: float) -> dict:
    """
    Attempt TCP connection to host:port. Returns a JSON-serializable dict.
    Does not send application data after connect.
    """
    t0 = time.perf_counter()
    try:
        with socket.create_connection((host, port), timeout=timeout_sec):
            pass
        elapsed_ms = (time.perf_counter() - t0) * 1000.0
        return {
            "ok": True,
            "host": host,
            "port": port,
            "elapsed_ms": round(elapsed_ms, 2),
        }
    except TimeoutError:
        elapsed_ms = (time.perf_counter() - t0) * 1000.0
        return {
            "ok": False,
            "host": host,
            "port": port,
            "elapsed_ms": round(elapsed_ms, 2),
            "error": "timeout",
        }
    except OSError as e:
        elapsed_ms = (time.perf_counter() - t0) * 1000.0
        return {
            "ok": False,
            "host": host,
            "port": port,
            "elapsed_ms": round(elapsed_ms, 2),
            "error": e.__class__.__name__,
            "detail": str(e),
        }
