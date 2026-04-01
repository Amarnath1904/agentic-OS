"""CLI: serve | shell | ping (default: serve)."""

from __future__ import annotations

import argparse
import threading

from . import DEFAULT_TIMEOUT_MS
from .http_server import run_http_server
from .repl import run_one_shot, run_repl


def _cmd_serve(args: argparse.Namespace) -> None:
    run_http_server(args.bind_host, args.bind_port)


def _cmd_shell(args: argparse.Namespace) -> None:
    if args.with_api:
        t = threading.Thread(
            target=run_http_server,
            args=(args.bind_host, args.bind_port),
            name="tcp-ping-http",
            daemon=True,
        )
        t.start()
        print(
            "HTTP API on http://%s:%s/ (use /ping for probes)"
            % (args.bind_host, args.bind_port)
        )
    run_repl()


def _cmd_ping(args: argparse.Namespace) -> None:
    raise SystemExit(run_one_shot(args.host, args.port, args.timeout_ms))


def main() -> None:
    parent = argparse.ArgumentParser(add_help=False)
    parent.add_argument(
        "--bind-host",
        default="127.0.0.1",
        help="HTTP listen address (default: 127.0.0.1)",
    )
    parent.add_argument(
        "--bind-port",
        type=int,
        default=8765,
        help="HTTP listen port (default: 8765)",
    )

    parser = argparse.ArgumentParser(
        description="TCP connect probe: HTTP API and/or interactive shell.",
        parents=[parent],
    )
    sub = parser.add_subparsers(dest="command", required=False)
    sub.add_parser(
        "serve",
        parents=[parent],
        help="Run HTTP API only (no probes until /ping)",
    )
    shell_p = sub.add_parser(
        "shell",
        parents=[parent],
        help="Interactive ping shell",
    )
    shell_p.add_argument(
        "--with-api",
        action="store_true",
        help="Also listen for HTTP in a background thread",
    )
    ping_p = sub.add_parser("ping", help="One-shot TCP probe")
    ping_p.add_argument("host", help="IPv4 address")
    ping_p.add_argument(
        "port",
        nargs="?",
        type=int,
        default=None,
        help="TCP port (default: 53)",
    )
    ping_p.add_argument(
        "--timeout-ms",
        type=int,
        default=DEFAULT_TIMEOUT_MS,
        help="Connect timeout in ms (default: %d)" % DEFAULT_TIMEOUT_MS,
    )

    args = parser.parse_args()
    command = args.command or "serve"

    if command == "serve":
        _cmd_serve(args)
    elif command == "shell":
        _cmd_shell(args)
    elif command == "ping":
        _cmd_ping(args)
    else:
        parser.error("unknown command")


if __name__ == "__main__":
    main()
