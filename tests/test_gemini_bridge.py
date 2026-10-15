import io
import json
import socket
import threading
import time
import unittest
from unittest.mock import patch
from urllib.error import HTTPError, URLError

from host_tools.gemini_bridge import (
    Bridge, GeminiClient, GeminiError, HEADER, MAGIC, REQUEST, FETCH,
    PENDING, DATA, ERROR, display_text, load_api_key,
)


def request(rid=42, prompt=b"hello", offset=0, kind=REQUEST):
    return HEADER.pack(MAGIC, rid, offset, 0, kind) + prompt


class BridgeTests(unittest.TestCase):
    def setUp(self):
        self.calls = []

        def generate(prompt):
            self.calls.append(prompt)
            return "answer " * 200

        self.bridge = Bridge(generate)
        self.peer = ("127.0.0.1", 12345)

    def tearDown(self):
        self.bridge.close()

    def completed(self, packet):
        for _ in range(200):
            reply = self.bridge.handle(packet, self.peer)
            if reply[12] != PENDING:
                return reply
            time.sleep(.005)
        self.fail("worker did not complete")

    def test_lost_replies_duplicates_and_chunk_reassembly(self):
        initial = self.completed(request())
        self.assertEqual(initial, self.completed(request()))
        result = initial[16:]
        total = HEADER.unpack_from(initial)[3]
        while len(result) < total:
            packet = request(prompt=b"", offset=len(result), kind=FETCH)
            reply = self.completed(packet)
            self.assertEqual(reply, self.completed(packet))
            self.assertEqual(reply[12], DATA)
            result += reply[16:]
        self.assertEqual(result, ("answer " * 200).strip().encode())
        self.assertEqual(self.calls, ["hello"])

    def test_malformed_packets_do_not_call_api(self):
        for packet in (b"", b"AGI1", request(prompt=b""), request(prompt=b"a" * 512),
                       request(prompt=b"\xff"), request(prompt=b"\x1b"),
                       request(offset=1), request(kind=99),
                       request(prompt=b"", kind=FETCH, offset=512),
                       request()[:13] + b"bad" + b"hello"):
            self.assertIsNone(self.bridge.handle(packet, self.peer))
        self.assertEqual(self.calls, [])

    def test_id_conflict_does_not_execute_again(self):
        self.completed(request())
        reply = self.completed(request(prompt=b"different"))
        self.assertEqual(reply[12], ERROR)
        self.assertEqual(self.calls, ["hello"])

    def test_pending_retries_do_not_duplicate_upstream_request(self):
        gate = threading.Event()
        self.addCleanup(gate.set)
        self.bridge.generate = lambda prompt: (gate.wait(2), "done")[1]
        for _ in range(10):
            self.assertEqual(self.bridge.handle(request(), self.peer)[12], PENDING)
        self.assertEqual(len(self.bridge.jobs), 1)
        gate.set()
        self.assertEqual(self.completed(request())[16:], b"done")

    def test_errors_are_displayed_and_cached_without_secret_leaks(self):
        def fail(prompt):
            raise RuntimeError("SECRET_API_KEY")
        self.bridge.generate = fail
        reply = self.completed(request())
        self.assertEqual(reply[12], ERROR)
        self.assertNotIn(b"SECRET", reply)
        self.assertEqual(reply, self.completed(request()))

    def test_real_udp_socket_roundtrip(self):
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as server:
            server.bind(("127.0.0.1", 0))
            server.settimeout(2)
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as client:
                client.settimeout(2)
                client.sendto(request(), server.getsockname())
                packet, peer = server.recvfrom(2048)
                server.sendto(self.bridge.handle(packet, peer), peer)
                reply, _ = client.recvfrom(2048)
                self.assertEqual(reply[:8], request()[:8])
                self.assertIn(reply[12], (DATA, PENDING))

    def test_vga_sanitization_and_bound(self):
        result = display_text("café\ttext\r\n\x1b" + "x" * 9000)
        self.assertLessEqual(len(result), 8192)
        self.assertTrue(result.startswith(b"cafe    text\n"))
        self.assertNotIn(b"\x1b", result)
        self.assertIn(b"truncated", result)


class GeminiTests(unittest.TestCase):
    def test_local_key_and_environment_override(self):
        with patch.dict("os.environ", {"GEMINI_API_KEY": ""}):
            with patch("pathlib.Path.read_text", return_value="local-test-key\n"):
                self.assertEqual(load_api_key(), "local-test-key")
            with patch("pathlib.Path.read_text", side_effect=FileNotFoundError):
                self.assertEqual(load_api_key(), "")
        with patch.dict("os.environ", {"GEMINI_API_KEY": "override-test-key"}):
            with patch("pathlib.Path.read_text") as read:
                self.assertEqual(load_api_key(), "override-test-key")
                read.assert_not_called()

    def setUp(self):
        self.client = GeminiClient("test-key", "test-model")

    def test_https_request_and_text_extraction(self):
        body = {"candidates": [{"content": {"parts": [
            {"text": "private", "thought": True}, {"text": "answer"}, {"text": "next"}
        ]}}]}
        with patch("urllib.request.urlopen", return_value=io.BytesIO(json.dumps(body).encode())) as send:
            self.assertEqual(self.client('quotes " and slash \\'), "answer\nnext")
        req = send.call_args.args[0]
        self.assertTrue(req.full_url.startswith("https://generativelanguage.googleapis.com/"))
        self.assertNotIn("test-key", req.full_url)
        self.assertEqual(req.get_header("X-goog-api-key"), "test-key")
        self.assertEqual(json.loads(req.data)["contents"][0]["parts"][0]["text"], 'quotes " and slash \\')

    def test_http_errors_and_timeout(self):
        for code in (400, 401, 403, 404, 429, 500):
            with patch("urllib.request.urlopen", side_effect=HTTPError("url", code, "secret", {}, None)):
                with self.assertRaises(GeminiError) as caught:
                    self.client("hello")
                self.assertNotIn("secret", str(caught.exception))
        for error in (TimeoutError(), URLError("secret")):
            with patch("urllib.request.urlopen", side_effect=error):
                with self.assertRaises(GeminiError):
                    self.client("hello")

    def test_missing_key_bad_json_and_blocked_response(self):
        with self.assertRaises(GeminiError):
            GeminiClient("", "test")("hello")
        for body in (b"bad json", b"{}", b"[]", b'{"candidates": [{}]}'):
            with patch("urllib.request.urlopen", return_value=io.BytesIO(body)):
                with self.assertRaises(GeminiError):
                    self.client("hello")

    def test_invalid_model(self):
        with self.assertRaises(ValueError):
            GeminiClient("key", "../../other?key=value")


if __name__ == "__main__":
    unittest.main()
