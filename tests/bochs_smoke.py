"""Boot the real image and test login + NIC + bridge + terminal in Bochs.

Debugger injects characters into the keyboard ring at line_input_read; all login,
shell, network, and rendering code executes normally. Only live/nono-live scenarios use an API key.
python tests/bochs_smoke.py --bochs "C:/Program Files/Bochs-3.0/bochs.exe"
"""
import argparse
from pathlib import Path
import socket
import struct
import subprocess
import sys
import threading
import time
import json

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from host_tools.gemini_bridge import Bridge, GeminiError, GeminiClient, load_api_key, display_text
from host_tools.nono_plan import encode_plan


def symbols(path):
    data = path.read_bytes()
    header = struct.unpack_from("<16sHHIIIIIHHHHHH", data)
    sections = [struct.unpack_from("<IIIIIIIIII", data, header[6] + i * header[11])
                for i in range(header[12])]
    result = {}
    for section in sections:
        if section[1] != 2:
            continue
        strings_section = sections[section[6]]
        strings = data[strings_section[4]:strings_section[4] + strings_section[5]]
        for offset in range(section[4], section[4] + section[5], section[9]):
            name, value, *_ = struct.unpack_from("<IIIBBH", data, offset)
            name = strings[name:strings.find(b"\0", name)].decode()
            result[name] = value
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bochs", required=True, type=Path)
    parser.add_argument("--build", type=Path, default=ROOT / ".build-test")
    parser.add_argument("--display", default="nogui")
    parser.add_argument("--scenario", choices=("basic", "reliability", "errors", "shutdown", "delayed", "live", "nono", "nono-live", "nono-invalid"), default="basic")
    parser.add_argument("--clock", choices=("none", "realtime"), default="realtime")
    parser.add_argument("--ips", type=int, default=4000000)
    args = parser.parse_args()
    out = args.build.resolve()
    syms = symbols(out / "kernel.elf")
    commands = [f"lb {syms['line_input_read']}", "c"]
    lines = ["wrong\n", "wrong\n", "amarnath\n", "pandey12\n"]
    if args.scenario in ("nono", "nono-live"):
        lines += ["mkdir workspace\n", "cd workspace\n",
                  "nono make a folder with 5 folders in it each has 3 files\n",
                  "nono write hello from nono into project/folder1/file1.txt\n",
                  "cat project/folder1/file1.txt\n"]
    elif args.scenario == "nono-invalid":
        lines += ["nono bad path\n", "nono bad operation\n", "nono truncated\n"]
    elif args.scenario == "live":
        lines += ["ask explain a black hole in one short sentence\n"]
    elif args.scenario == "delayed":
        lines += ["ask delayed answer\n"]
    elif args.scenario == "shutdown":
        lines += ["shutdown\n"]
    elif args.scenario == "reliability":
        # Full response buffer, six VGA pages, receive-ring wrap on next answer.
        lines += ["ask long reply\n"] + ["\n"] * 5 + ["ask smoke network\n"]
    elif args.scenario == "errors":
        lines += ["ask fail\n", "ask smoke network\n"]
    else:
        lines += ["ask smoke network\n", "ai\n", "chat smoke\n", "/exit\n"]
    for line in lines:
        commands.append(f"setpmem {syms['key_buffer_head']} 4 0")
        for index, byte in enumerate(line.encode()):
            commands.append(f"setpmem {syms['key_buffer'] + index} 1 {byte}")
        commands += [f"setpmem {syms['key_buffer_tail']} 4 {len(line)}", "c"]
    commands += [f'writemem "{(out / "vga.bin").as_posix()}" 0xb8000 4000',
                 f'writemem "{(out / "ramfs.bin").as_posix()}" {syms["g_entries"]} {96 * 144}', "q"]
    (out / "smoke.rc").write_text("\n".join(commands) + "\n")
    bochs_dir = args.bochs.resolve().parent
    config = f'''megs: 32
romimage: file="{bochs_dir.as_posix()}/BIOS-bochs-latest"
vgaromimage: file="{bochs_dir.as_posix()}/VGABIOS-lgpl-latest.bin"
boot: disk
pci: enabled=1, chipset=i440fx
ata0-master: type=disk, path="{out.as_posix()}/boot.img", mode=flat, cylinders={(out / 'boot.img').stat().st_size // (63 * 512)}, heads=1, spt=63
cpu: count=1, ips={args.ips}, reset_on_triple_fault=0
clock: sync={args.clock}
display_library: {args.display}
config_interface: textconfig
ne2k: enabled=1, type=isa, ioaddr=0x300, irq=5, mac=b0:c4:20:00:00:01, ethmod=slirp
log: "{out.as_posix()}/bochs-smoke.log"
panic: action=fatal
error: action=report
info: action=report
debug: action=ignore
'''
    (out / "smoke.bxrc").write_text(config)
    seen = []
    live_answers = []
    def generate(prompt):
        seen.append(prompt)
        if args.scenario == "live":
            answer = GeminiClient(load_api_key(), "gemini-flash-latest")(prompt)
            live_answers.append(display_text(answer).decode("ascii"))
            return answer
        if prompt == "delayed answer": time.sleep(30)
        if prompt == "long reply": return "L" * 8192
        if prompt == "fail": raise GeminiError("TEST API quota error")
        return "MOCK network answer: " + prompt
    def plan(prompt):
        if args.scenario == "nono-live":
            return GeminiClient(load_api_key(), "gemini-flash-latest").plan(prompt)
        if args.scenario == "nono-invalid":
            return {"bad path": "NPLAN1\nD safe\nW ../escape -\nEND",
                    "bad operation": "NPLAN1\nD safe\nX shutdown\nEND",
                    "truncated": "NPLAN1\nD safe\n"}[prompt]
        actions = [{"op": "mkdir", "path": "project"}]
        if prompt.startswith("write"):
            actions += [{"op": "mkdir", "path": "project/folder1"},
                        {"op": "write", "path": "project/folder1/file1.txt", "content": "hello from nono"}]
        else:
            for i in range(1, 6):
                actions.append({"op": "mkdir", "path": f"project/folder{i}"})
                for j in range(1, 4):
                    actions.append({"op": "write", "path": f"project/folder{i}/file{j}.txt", "content": ""})
        return encode_plan(json.dumps({"actions": actions}))
    bridge = Bridge(generate, plan)
    stop = threading.Event()
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.bind(("127.0.0.1", 8765))
        sock.settimeout(.2)
        def serve():
            dropped = set()
            previous = None
            while not stop.is_set():
                try:
                    packet, peer = sock.recvfrom(2048)
                    reply = bridge.handle(packet, peer)
                    if reply:
                        if args.scenario == "reliability" and reply[12] == 4:
                            offset = struct.unpack_from("!H", reply, 8)[0]
                            key = (peer, offset)
                            # Lose the first response at offsets 0 and 512.
                            if offset in (0, 512) and key not in dropped:
                                dropped.add(key)
                                continue
                            if previous is not None:
                                sock.sendto(previous, peer)  # stale/duplicate chunk
                            previous = reply
                        sock.sendto(reply, peer)
                except (socket.timeout, ConnectionResetError):
                    pass
        thread = threading.Thread(target=serve)
        thread.start()
        try:
            flags = subprocess.CREATE_NO_WINDOW if sys.platform == "win32" else 0
            result = subprocess.run([str(args.bochs), "-q", "-f", str(out / "smoke.bxrc"),
                                     "-debugger", "-rc", str(out / "smoke.rc")],
                                    input="\n", capture_output=True, text=True, timeout=90,
                                    creationflags=flags)
            (out / "smoke-console.log").write_text(result.stdout + result.stderr)
            if result.returncode and args.scenario != "shutdown":
                raise RuntimeError((result.stdout + result.stderr)[-4000:])
        finally:
            stop.set(); thread.join(); bridge.close()
    if args.scenario == "shutdown":
        log = (out / "bochs-smoke.log").read_text()
        assert "ACPI control: soft power off" in log, log[-2000:]
        assert not (out / "boot.img.lock").exists(), "Disk lock left after shutdown"
        print("PASS: shutdown powers off Bochs and releases the image lock")
        return
    raw = (out / "vga.bin").read_bytes()
    screen = "\n".join(raw[row * 160:row * 160 + 160:2].decode("ascii", "replace") for row in range(25))
    (out / "smoke-screen.txt").write_text(screen)
    print(screen)
    if args.scenario.startswith("nono"):
        entries = (out / "ramfs.bin").read_bytes()
        nodes = {}
        for i in range(96):
            entry = entries[i * 144:(i + 1) * 144]
            if entry[0]:
                name = entry[8:136].split(b"\0")[0].decode()
                nodes[name] = (struct.unpack_from("<I", entry, 4)[0], struct.unpack_from("<I", entry, 140)[0])
        if args.scenario == "nono-invalid":
            assert nodes == {}, nodes
            assert "No files changed" in screen
        else:
            assert len(nodes) == 22, nodes
            assert nodes["/workspace/project"] == (1, 0)
            for i in range(1, 6):
                assert nodes[f"/workspace/project/folder{i}"] == (1, 0)
                for j in range(1, 4):
                    node = nodes[f"/workspace/project/folder{i}/file{j}.txt"]
                    assert node[0] == 0
                    if (i, j) != (1, 1): assert node[1] == 0
            assert "hello from nono" in screen
            assert "Nono: done. Verified 6 folders and 15 files" in screen
        print("PASS: verified actual RAMFS nodes and contents:", args.scenario)
        return
    if args.scenario == "live":
        assert live_answers, "Gemini did not return an answer"
        assert "".join(live_answers[0].split()) in "".join(screen.split()), screen
        assert "AI timed out" not in screen
        print("PASS: real guest Ethernet/UDP -> bridge -> live Gemini -> VGA terminal")
        return
    expected = {"basic": ["smoke network", "chat smoke"],
                "delayed": ["delayed answer"],
                "reliability": ["long reply", "smoke network"],
                "errors": ["fail", "smoke network"]}[args.scenario]
    assert seen == expected, seen
    assert "MOCK network answer: " + expected[-1] in screen
    if args.scenario == "errors": assert "TEST API quota error" in screen
    assert "agentic:/$" in screen
    print("PASS: real guest login/network/terminal scenario:", args.scenario)


if __name__ == "__main__":
    main()
