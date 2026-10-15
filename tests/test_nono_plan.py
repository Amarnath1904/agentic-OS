import json
import unittest
from host_tools.nono_plan import encode_plan
from host_tools.gemini_bridge import Bridge, HEADER, MAGIC, PLAN_REQUEST, REQUEST, ERROR


class PlanTests(unittest.TestCase):
    def test_example_exact_counts(self):
        actions = [{"op": "mkdir", "path": "project"}]
        for i in range(1, 6):
            actions.append({"op": "mkdir", "path": f"project/folder{i}"})
            for j in range(1, 4):
                actions.append({"op": "write", "path": f"project/folder{i}/file{j}.txt", "content": ""})
        plan = encode_plan(json.dumps({"actions": actions}))
        self.assertEqual(sum(s.startswith("D ") for s in plan.splitlines()), 6)
        self.assertEqual(sum(s.startswith("W ") for s in plan.splitlines()), 15)
        self.assertTrue(plan.endswith("\nEND"))

    def test_content_encoded_as_data(self):
        text = "hello\nshutdown\nrm file\t\"quoted\""
        plan = encode_plan(json.dumps({"actions": [{"op": "write", "path": "note.txt", "content": text}]}))
        self.assertEqual(bytes.fromhex(plan.splitlines()[1].split()[2]).decode(), text)
        self.assertEqual(len(plan.splitlines()), 3)

    def test_reject_unsafe_or_unsupported_paths(self):
        for path in ("/outside", "../outside", "x/../outside", "x//y", "./x", "x/", "a b", "a;shutdown", "C:\\x", "x" * 111):
            with self.subTest(path=path), self.assertRaises(ValueError):
                encode_plan(json.dumps({"actions": [{"op": "mkdir", "path": path}]}))

    def test_reject_unordered_duplicate_and_oversized_plans(self):
        cases = [[], [{"op": "delete", "path": "x"}],
                 [{"op": "write", "path": "missing/x", "content": ""}],
                 [{"op": "mkdir", "path": "x"}] * 2,
                 [{"op": "mkdir", "path": f"x{i}"} for i in range(65)],
                 [{"op": "write", "path": "x", "content": "a" * 513}],
                 [{"op": "write", "path": "x", "content": "\x00"}]]
        for actions in cases:
            with self.subTest(actions=actions), self.assertRaises(ValueError):
                encode_plan(json.dumps({"actions": actions}))

    def test_request_modes_do_not_share_cached_actions(self):
        bridge = Bridge(lambda p: "chat", lambda p: "NPLAN1\nD x\nEND")
        try:
            packet = HEADER.pack(MAGIC, 1, 0, 0, REQUEST) + b"hello"
            bridge.handle(packet, ("127.0.0.1", 1))
            packet = HEADER.pack(MAGIC, 1, 0, 0, PLAN_REQUEST) + b"hello"
            self.assertEqual(bridge.handle(packet, ("127.0.0.1", 1))[12], ERROR)
        finally:
            bridge.close()


if __name__ == "__main__":
    unittest.main()
