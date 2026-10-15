"""Local UDP-to-Gemini HTTPS bridge. Run: python -m host_tools.gemini_bridge.

Only binds loopback; the guest reaches it through slirp at 10.0.2.2:8765.
No external dependencies. Credentials and HTTPS remain on the host.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
import getpass
import json
import os
from pathlib import Path
import re
import socket
import struct
import time
import unicodedata
import urllib.error
import urllib.request
from .nono_plan import PLAN_INSTRUCTION, encode_plan

HEADER = struct.Struct("!4sIHHB3x")
MAGIC = b"AGI1"
REQUEST, FETCH, PENDING, DATA, ERROR = range(1, 6)
PLAN_REQUEST = 6
CHUNK = 512
MAX_REPLY = 8192
MAX_PROMPT = 511
CACHE_TTL = 180
MAX_JOBS = 128


class GeminiError(Exception):
    pass


def load_api_key():
    """Environment override, then the private project-local credential file."""
    key = os.environ.get("GEMINI_API_KEY", "").strip()
    if key:
        return key
    path = Path(__file__).resolve().parents[1] / ".env.gemini"
    try:
        return path.read_text(encoding="utf-8").strip()
    except FileNotFoundError:
        return ""


def display_text(text):
    """VGA currently supports printable ASCII, with newlines."""
    text = "".join(c for c in unicodedata.normalize("NFKD", text)
                   if not unicodedata.combining(c)).encode("ascii", "replace").decode()
    text = text.replace("\r\n", "\n").replace("\t", "    ")
    text = "".join(c for c in text if c == "\n" or " " <= c <= "~")
    data = text.strip().encode("ascii") or b"Gemini returned no displayable text."
    if len(data) > MAX_REPLY:
        suffix = b"\n[Answer truncated to fit the terminal.]"
        data = data[:MAX_REPLY - len(suffix)] + suffix
    return data


class GeminiClient:
    def __init__(self, api_key, model):
        if not re.fullmatch(r"[A-Za-z0-9._-]+", model):
            raise ValueError("Model must be a Gemini model ID, without a URL or models/ prefix.")
        self.api_key = api_key
        self.url = "https://generativelanguage.googleapis.com/v1beta/models/" + model + ":generateContent"

    def __call__(self, prompt):
        return self.generate(prompt)

    def plan(self, prompt):
        try:
            return encode_plan(self.generate(prompt, planning=True))
        except ValueError:
            raise GeminiError("Nono received an invalid plan; no files changed. Try a simpler request.") from None

    def generate(self, prompt, planning=False):
        if not self.api_key:
            raise GeminiError("Set GEMINI_API_KEY on the host and restart the bridge.")
        payload = {
            "systemInstruction": {"parts": [{"text":
                "Answer clearly and concisely for an 80-column plain text terminal. "
                "Use ASCII where possible. Do not use Markdown tables."}]},
            "contents": [{"role": "user", "parts": [{"text": prompt}]}],
            "generationConfig": {"maxOutputTokens": 2048},
        }
        if planning:
            payload["systemInstruction"] = {"parts": [{"text": PLAN_INSTRUCTION}]}
            payload["generationConfig"] = {"maxOutputTokens": 8192, "responseMimeType": "application/json"}
        req = urllib.request.Request(self.url, json.dumps(payload).encode("utf-8"), {
            "Content-Type": "application/json", "x-goog-api-key": self.api_key,
        }, method="POST")
        try:
            # Default HTTPS handler verifies Google's certificate and hostname.
            with urllib.request.urlopen(req, timeout=60) as response:
                raw = response.read(1_048_577)
                if len(raw) > 1_048_576:
                    raise GeminiError("Gemini response exceeded the bridge limit.")
                result = json.loads(raw)
        except urllib.error.HTTPError as exc:
            code = exc.code
            exc.close()
            message = {
                400: "Gemini rejected the request; check the model and API key.",
                401: "Gemini authentication failed; check GEMINI_API_KEY.",
                403: "Gemini access denied; check the key and project permissions.",
                404: "Gemini model not found; set GEMINI_MODEL on the host.",
                429: "Gemini rate or quota limit reached. Try again later.",
            }.get(code, f"Gemini HTTP error {code}. Try again later.")
            raise GeminiError(message) from None
        except (urllib.error.URLError, TimeoutError, OSError):
            raise GeminiError("Cannot reach Gemini over HTTPS. Check host internet access.") from None
        except (ValueError, UnicodeError):
            raise GeminiError("Gemini returned invalid JSON.") from None
        try:
            candidates = result.get("candidates", [])
            if not candidates:
                raise GeminiError("Gemini returned no answer (possibly blocked by safety settings).")
            candidate = candidates[0]
            parts = candidate.get("content", {}).get("parts", [])
            text = "\n".join(p["text"] for p in parts
                             if isinstance(p.get("text"), str) and not p.get("thought"))
            if not text.strip():
                raise GeminiError("Gemini returned no text answer. Try rephrasing your question.")
            if candidate.get("finishReason") == "MAX_TOKENS":
                text += "\n[Gemini reached its output limit.]"
            return text
        except (AttributeError, TypeError, KeyError, IndexError):
            raise GeminiError("Gemini returned an unexpected response format.") from None


@dataclass
class Job:
    prompt: str
    future: object
    touched: float
    request_kind: int = REQUEST
    answer: bytes = b""
    kind: int = DATA


class Bridge:
    def __init__(self, generate, plan=None):
        self.generate = generate
        self.plan = plan
        self.jobs = {}
        self.pool = ThreadPoolExecutor(max_workers=4)

    def close(self):
        self.pool.shutdown(wait=True, cancel_futures=True)

    @staticmethod
    def packet(request_id, offset, kind, answer=b""):
        return HEADER.pack(MAGIC, request_id, offset, len(answer), kind) + answer[offset:offset + CHUNK]

    def handle(self, packet, peer):
        """Called by one receive loop. Worker threads only perform HTTPS calls."""
        if len(packet) < HEADER.size or len(packet) > HEADER.size + MAX_PROMPT:
            return None
        magic, request_id, offset, total, kind = HEADER.unpack_from(packet)
        if magic != MAGIC or total or packet[13:16] != b"\0\0\0":
            return None
        if kind not in (REQUEST, FETCH, PLAN_REQUEST) or offset % CHUNK:
            return None
        if kind in (REQUEST, PLAN_REQUEST) and offset or kind == FETCH and len(packet) != HEADER.size:
            return None
        now = time.monotonic()
        for key, job in list(self.jobs.items()):
            if job.future.done() and now - job.touched > CACHE_TTL:
                del self.jobs[key]
        key = (peer, request_id)
        job = self.jobs.get(key)
        if kind in (REQUEST, PLAN_REQUEST):
            try:
                prompt = packet[HEADER.size:].decode("ascii")
            except UnicodeError:
                return None
            if not prompt.strip() or any(ord(c) < 32 or ord(c) > 126 for c in prompt):
                return None
            if job and (job.prompt != prompt or job.request_kind != kind):
                return self.packet(request_id, 0, ERROR, b"Request ID conflict. Please ask again.")
            if not job:
                if sum(not item.future.done() for item in self.jobs.values()) >= 4:
                    return self.packet(request_id, 0, ERROR, b"Bridge busy. Try again shortly.")
                if len(self.jobs) >= MAX_JOBS:
                    return self.packet(request_id, 0, ERROR, b"Bridge cache full. Retry in three minutes.")
                if kind == PLAN_REQUEST and self.plan is None:
                    return self.packet(request_id, 0, ERROR, b"This bridge does not support Nono actions.")
                job = Job(prompt, self.pool.submit(self.plan if kind == PLAN_REQUEST else self.generate, prompt), now, kind)
                self.jobs[key] = job
        elif not job:
            return None  # An expired/missing fetch must never start a new API call.
        job.touched = now
        if not job.future.done():
            return self.packet(request_id, offset, PENDING)
        if not job.answer:
            try:
                result = job.future.result()
                if job.request_kind == PLAN_REQUEST:
                    job.answer = result.encode("ascii")
                    if not 0 < len(job.answer) <= MAX_REPLY:
                        raise GeminiError("Nono plan exceeds the response limit.")
                else:
                    job.answer = display_text(result)
            except GeminiError as exc:
                job.answer, job.kind = display_text(str(exc)), ERROR
            except Exception:
                # Never echo exception strings that might contain credentials.
                job.answer, job.kind = b"Bridge internal error. Restart the host bridge.", ERROR
        if offset >= len(job.answer):
            return None
        return self.packet(request_id, offset, job.kind, job.answer)


def serve(bridge, sock):
    while True:
        try:
            packet, peer = sock.recvfrom(2048)
            reply = bridge.handle(packet, peer)
            if reply is not None:
                sock.sendto(reply, peer)
        except (ConnectionResetError, ConnectionRefusedError):
            # Windows UDP may report a previous client's closed ephemeral port.
            continue


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("--model", default=os.environ.get("GEMINI_MODEL", "gemini-flash-latest"))
    parser.add_argument("--prompt-key", action="store_true", help="Read API key without echoing it")
    parser.add_argument("--mock", action="store_true", help="Explicit offline transport test; no Gemini calls")
    args = parser.parse_args()
    if not 1 <= args.port <= 65535:
        parser.error("port must be between 1 and 65535")
    key = getpass.getpass("Gemini API key: ") if args.prompt_key else load_api_key()
    if not args.mock and not key:
        parser.error("Set GEMINI_API_KEY or use --prompt-key. Use --mock only for offline testing.")
    try:
        client = GeminiClient(key, args.model)
    except ValueError as exc:
        parser.error(str(exc))
    generate = (lambda prompt: "[MOCK - no Gemini call] Received: " + prompt) if args.mock else client
    bridge = Bridge(generate, None if args.mock else client.plan)
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.bind(("127.0.0.1", args.port))
            print(f"Bridge listening on 127.0.0.1:{args.port}; guest endpoint 10.0.2.2:{args.port}")
            print("MOCK mode" if args.mock else f"Gemini model: {args.model}")
            serve(bridge, sock)
    except KeyboardInterrupt:
        print("\nStopping bridge.")
    finally:
        bridge.close()


if __name__ == "__main__":
    main()
