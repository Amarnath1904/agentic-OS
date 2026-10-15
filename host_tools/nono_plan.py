"""Validate Gemini filesystem plans and encode the kernel's small action format."""
import json
import re

PLAN_INSTRUCTION = """You plan filesystem actions for a small operating system.
Return only JSON: {"actions": [{"op":"mkdir","path":"relative/path"},
{"op":"write","path":"relative/path/file.txt","content":"text"}]}.
Only mkdir and write are supported. All paths are relative to the user's current
directory. Include every parent directory before its children. No absolute paths,
dot/dot-dot components, spaces in names, shell commands, deletion, or code execution.
For unspecified names use project, folder1..folderN, file1.txt..fileN.txt.
For files with unspecified content use an empty string. Follow counts exactly.
At most 64 actions, path length 110, each component at most 63, ASCII file content
at most 512 bytes. Each path may appear only once. Existing files may be replaced
only when the user asks to write/change them. Do not claim execution; only plan.
If the task cannot be done with these actions return {"error":"brief reason"}.
"""


def encode_plan(text):
    obj = json.loads(text)
    if not isinstance(obj, dict):
        raise ValueError("Nono needs a JSON action plan.")
    if "error" in obj:
        raise ValueError("Nono can currently create folders and write small text files only.")
    actions = obj.get("actions")
    if not isinstance(actions, list) or not 1 <= len(actions) <= 64:
        raise ValueError("Nono needs between 1 and 64 actions.")
    paths = set()
    directories = set()
    lines = ["NPLAN1"]
    for action in actions:
        if not isinstance(action, dict):
            raise ValueError("Invalid Nono action.")
        op, path = action.get("op"), action.get("path")
        if op not in ("mkdir", "write") or not isinstance(path, str):
            raise ValueError("Unsupported Nono action.")
        if not 1 <= len(path) <= 110 or not re.fullmatch(r"[A-Za-z0-9_./-]+", path):
            raise ValueError("Invalid Nono path.")
        components = path.split("/")
        if any(p in ("", ".", "..") or len(p) > 63 for p in components) or path in paths:
            raise ValueError("Invalid or repeated Nono path.")
        parent = path.rpartition("/")[0]
        if parent and parent not in directories:
            raise ValueError("Nono plan must list parent directories first.")
        paths.add(path)
        if op == "mkdir":
            directories.add(path)
            lines.append("D " + path)
        else:
            content = action.get("content")
            if not isinstance(content, str):
                raise ValueError("Nono file content must be text.")
            try:
                data = content.encode("ascii")
            except UnicodeError:
                raise ValueError("Nono currently supports ASCII text files.") from None
            if len(data) > 512 or any(b < 32 and b not in (9, 10, 13) or b > 126 for b in data):
                raise ValueError("Nono file content exceeds its text limits.")
            lines.append("W " + path + " " + (data.hex() if data else "-"))
    lines.append("END")
    result = "\n".join(lines)
    if len(result) > 8192:
        raise ValueError("Nono plan is too large; request a smaller task.")
    return result
