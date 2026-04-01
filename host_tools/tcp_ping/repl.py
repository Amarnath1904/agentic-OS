"""Interactive REPL: ping <ipv4> [port], help, quit."""

from __future__ import annotations

import shlex

from . import DEFAULT_PING_PORT, DEFAULT_TIMEOUT_MS
from .parse_endpoint import ParseError, clamp_timeout_ms, parse_ipv4, parse_port
from .tcp_probe import probe_tcp


def format_probe_line(result: dict) -> str:
    if result.get("ok"):
        return "ok {host}:{port} {elapsed_ms} ms".format(**result)
    err = result.get("error", "failed")
    detail = result.get("detail")
    if detail:
        return "fail {host}:{port} {err}: {detail}".format(
            host=result["host"], port=result["port"], err=err, detail=detail
        )
    return "fail {host}:{port} {err}".format(
        host=result["host"], port=result["port"], err=err
    )


def run_probe_cli(host: str, port: int, timeout_ms: int = DEFAULT_TIMEOUT_MS) -> dict:
    timeout_ms = clamp_timeout_ms(timeout_ms)
    return probe_tcp(host, port, timeout_ms / 1000.0)


def run_repl(prompt: str = "agentic-ping> ") -> None:
    print("Type: ping <ipv4> [port], help, quit")
    while True:
        try:
            line = input(prompt)
        except EOFError:
            print()
            break
        parts = shlex.split(line, posix=False)
        if not parts:
            continue
        cmd = parts[0].lower()
        if cmd in ("quit", "exit"):
            break
        if cmd == "help":
            print("  ping <ipv4>           TCP connect to <ipv4>:%d" % DEFAULT_PING_PORT)
            print("  ping <ipv4> <port>    TCP connect to that port")
            print("  quit                  Exit")
            continue
        if cmd != "ping":
            print("Unknown command. Try help.")
            continue
        if len(parts) < 2:
            print("ping: missing host")
            continue
        host_arg = parts[1]
        if len(parts) >= 3:
            try:
                port = parse_port(parts[2])
            except ParseError as e:
                print("ping:", e.message)
                continue
        else:
            port = DEFAULT_PING_PORT
        try:
            host = parse_ipv4(host_arg)
        except ParseError as e:
            print("ping:", e.message)
            continue
        result = run_probe_cli(host, port)
        print(format_probe_line(result))


def run_one_shot(host: str, port: int | None, timeout_ms: int) -> int:
    try:
        h = parse_ipv4(host)
        p = DEFAULT_PING_PORT if port is None else parse_port(port)
    except ParseError as e:
        print(e.message)
        return 2
    result = run_probe_cli(h, p, timeout_ms)
    print(format_probe_line(result))
    return 0 if result.get("ok") else 1
