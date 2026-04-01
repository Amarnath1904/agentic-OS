"""HTTP API: /health, /status, /ping (GET and POST). No outbound probes until /ping is called."""

from __future__ import annotations

import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse

from . import DEFAULT_PING_PORT
from .parse_endpoint import ParseError, clamp_timeout_ms, parse_ipv4, parse_port
from .tcp_probe import probe_tcp


def _json_bytes(obj: dict) -> bytes:
    return json.dumps(obj, separators=(",", ":")).encode("utf-8")


def make_handler_class(bind_display_host: str, bind_port: int):
    class TcpPingHandler(BaseHTTPRequestHandler):
        server_version = "TcpPingAPI/1.0"

        def log_message(self, fmt: str, *args) -> None:
            return

        def _send_json(self, code: int, body: dict) -> None:
            data = _json_bytes(body)
            self.send_response(code)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)

        def _send_error_json(self, code: int, message: str) -> None:
            self._send_json(code, {"ok": False, "error": message})

        def do_GET(self) -> None:
            parsed = urlparse(self.path)
            path = parsed.path.rstrip("/") or "/"
            if path == "/health":
                self._send_json(200, {"status": "ok"})
                return
            if path == "/status":
                self._send_json(
                    200,
                    {
                        "status": "ok",
                        "bind": bind_display_host,
                        "port": bind_port,
                    },
                )
                return
            if path == "/ping":
                self._handle_ping_get(parse_qs(parsed.query))
                return
            self._send_error_json(404, "not found")

        def do_POST(self) -> None:
            parsed = urlparse(self.path)
            path = parsed.path.rstrip("/") or "/"
            if path != "/ping":
                self._send_error_json(404, "not found")
                return
            length = int(self.headers.get("Content-Length", "0") or "0")
            raw = self.rfile.read(length) if length > 0 else b"{}"
            try:
                payload = json.loads(raw.decode("utf-8"))
            except (json.JSONDecodeError, UnicodeDecodeError):
                self._send_error_json(400, "invalid JSON body")
                return
            if not isinstance(payload, dict):
                self._send_error_json(400, "JSON body must be an object")
                return
            self._run_probe_from_mapping(payload)

        def _handle_ping_get(self, qs: dict) -> None:
            host_vals = qs.get("host", [])
            port_vals = qs.get("port", [])
            timeout_vals = qs.get("timeout_ms", [])
            if not host_vals or not host_vals[0].strip():
                self._send_error_json(400, "missing host")
                return
            try:
                host = parse_ipv4(host_vals[0])
                if not port_vals or not str(port_vals[0]).strip():
                    port = DEFAULT_PING_PORT
                else:
                    port = parse_port(port_vals[0])
                timeout_ms = clamp_timeout_ms(
                    timeout_vals[0] if timeout_vals else None
                )
            except ParseError as e:
                self._send_error_json(400, e.message)
                return
            self._execute_probe(host, port, timeout_ms)

        def _run_probe_from_mapping(self, payload: dict) -> None:
            if "host" not in payload:
                self._send_error_json(400, "missing host")
                return
            try:
                host = parse_ipv4(str(payload["host"]))
                if "port" not in payload or payload["port"] is None:
                    port = DEFAULT_PING_PORT
                else:
                    port = parse_port(payload["port"])
                timeout_ms = clamp_timeout_ms(payload.get("timeout_ms"))
            except ParseError as e:
                self._send_error_json(400, e.message)
                return
            self._execute_probe(host, port, timeout_ms)

        def _execute_probe(self, host: str, port: int, timeout_ms: int) -> None:
            result = probe_tcp(host, port, timeout_ms / 1000.0)
            self._send_json(200, result)

    return TcpPingHandler


def run_http_server(bind_host: str, bind_port: int) -> None:
    handler = make_handler_class(bind_host, bind_port)
    server = ThreadingHTTPServer((bind_host, bind_port), handler)
    try:
        server.serve_forever()
    finally:
        server.server_close()
