"""Validate IPv4, port, and timeout for TCP probes."""

from __future__ import annotations

from . import DEFAULT_PING_PORT, DEFAULT_TIMEOUT_MS, MAX_TIMEOUT_MS


class ParseError(ValueError):
    """Invalid host, port, or timeout for a probe."""

    def __init__(self, message: str) -> None:
        super().__init__(message)
        self.message = message


def parse_ipv4(s: str) -> str:
    s = s.strip()
    if not s:
        raise ParseError("host is empty")
    parts = s.split(".")
    if len(parts) != 4:
        raise ParseError("host must be IPv4 dotted-quad")
    octets: list[int] = []
    for p in parts:
        if not p.isdigit():
            raise ParseError("invalid IPv4 octet")
        n = int(p)
        if n < 0 or n > 255:
            raise ParseError("IPv4 octet out of range")
        octets.append(n)
    return ".".join(str(x) for x in octets)


def parse_port(value: int | str) -> int:
    if isinstance(value, str):
        value = value.strip()
        if not value.isdigit():
            raise ParseError("port must be an integer")
        p = int(value)
    else:
        p = int(value)
    if p < 1 or p > 65535:
        raise ParseError("port must be 1..65535")
    return p


def clamp_timeout_ms(raw: int | str | None, default: int = DEFAULT_TIMEOUT_MS) -> int:
    if raw is None or raw == "":
        return default
    if isinstance(raw, str):
        raw = raw.strip()
        if not raw.isdigit():
            raise ParseError("timeout_ms must be a non-negative integer")
        ms = int(raw)
    else:
        ms = int(raw)
    if ms < 1:
        raise ParseError("timeout_ms must be at least 1")
    if ms > MAX_TIMEOUT_MS:
        raise ParseError(f"timeout_ms must be at most {MAX_TIMEOUT_MS}")
    return ms


def resolve_probe_target(host: str, port: int | None) -> tuple[str, int]:
    h = parse_ipv4(host)
    p = DEFAULT_PING_PORT if port is None else parse_port(port)
    return h, p
