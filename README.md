# Agent OS: Gemini in the terminal

After the existing login, type `ask <question>` for one answer, or `ai` to enter
question/answer mode. Type `/exit` to return to the shell. Long answers pause at
`[Enter for more]`. Each question is independent; conversation history is not
sent to Gemini. Existing filesystem commands still work.
Type `shutdown` at the shell to power off Bochs cleanly and release the disk lock.
RAM filesystem contents are lost at shutdown. This does not shut down Windows.

## Nono filesystem agent

After login, type a task beginning with `nono`:

```text
nono make a folder with 5 folders in it each has 3 files
nono write hello from nono into project/folder1/file1.txt
ls project
ls project/folder1
cat project/folder1/file1.txt
```

When you do not specify names, Nono is instructed to use `project`, `folder1`
through `folder5`, and `file1.txt` through `file3.txt`. Unspecified file contents
are empty. You can request your own names and text. Paths are relative to the
current OS directory (`cd` changes it). Files live inside the OS RAM filesystem,
not on the Windows host, and disappear when the OS closes.

Gemini returns a structured action plan. The bridge validates it, and the kernel
independently checks the entire plan before executing only mkdir/write calls.
Model output is never executed as shell commands. Absolute paths, parent traversal,
duplicate paths, missing parents, unknown operations, truncated plans, and entry
capacity conflicts are rejected before mutation. Completed actions are verified
against RAMFS. If allocation fails during execution, Nono stops and explicitly
reports that earlier completed actions remain. Writing a file replaces its current
contents. Limits: 64 actions per request, 512 ASCII bytes per file, 8192-byte plan,
and 96 total filesystem entries. This version does not delete files or run programs.

The five-folder/fifteen-file example and subsequent file write were tested with
live Gemini in Bochs, including inspection of the actual RAMFS entries.

## Run on this Windows machine

From PowerShell in this repository:

```powershell
python -m host_tools.gemini_bridge
```

The supplied key is saved in the Git-ignored `.env.gemini` file in this project
(raw key text), so no prompt is needed on this machine. Keep that terminal running.
The bridge first checks `GEMINI_API_KEY` in its process environment, then this file.
Use `--prompt-key` to enter a replacement key privately for one run.
The key is never compiled into the OS image, sent to the guest, or logged.
Choose a model with `--model <model-id>` or `GEMINI_MODEL`; the default is
`gemini-flash-latest`. The model must be available to your API project.

Start Bochs with `bochsrc.bxrc`, boot the updated `boot.img`, and log in using
your existing credentials. For example:

```text
agentic:/$ ask explain how an operating system works
Thinking...
AI: ...
agentic:/$ ai
You: what is virtual memory
AI: ...
You: /exit
```

For an offline network test without an API key:

```powershell
python -m host_tools.gemini_bridge --mock
```

Mock replies are explicitly labeled and do not contact Gemini. Stop that bridge
with Ctrl+C before starting the real one on the same port.

## Network architecture

The kernel sends real Ethernet frames through its ISA NE2000 driver. ARP resolves
the next hop, IPv4/UDP carries requests to `10.0.2.2:8765`, and Bochs **slirp**
maps this to a Python bridge listening only on host `127.0.0.1:8765`.
Bochs must use `clock: sync=realtime` (already configured). With `sync=none`,
the guest's 90-second deadline can expire in only a few seconds of host time,
before Gemini finishes generating its answer.
The bridge calls Google's `generateContent` HTTPS endpoint with certificate
verification and returns the answer to the guest.

This implementation uses a host bridge: the kernel does **not** implement TLS,
DNS, a complete TCP stack, or direct Gemini HTTPS. The guest-to-host link is
unencrypted and intended for the local emulator only. Do not expose the bridge
on a LAN or use this protocol for sensitive traffic over an external network.

The driver uses I/O port `0x300`, IRQ 5, and MAC `b0:c4:20:00:00:01`.
IRQ 5 avoids Bochs's PCI interrupt routing conflicts. The guest uses static
`10.0.2.15/24`, gateway `10.0.2.2`. Change `kernel/net/net_config.h` and rebuild
if your emulator uses different settings. The original `vnet` configuration
does not provide the host networking used here; `bochsrc.bxrc` now uses slirp.

Requests are retried once per second with a 90-second guest deadline. The bridge
caches request IDs for at least three minutes after last access, so dropped
packets/retries do not repeat an API call while that bridge remains running.
Replies are fetched in 512-byte chunks. UDP and IPv4 checksums, endpoints,
lengths, IDs, and offsets are checked. Prompt size is at most 511 ASCII bytes
(`ask ` consumes four characters of the shell's 511-character line limit).
Replies are capped at 8192 display bytes with a visible truncation marker.
The VGA console transliterates accents and replaces unsupported characters.
The bridge limits concurrent API calls to four and cached requests to 128.
Restarting it loses the in-memory deduplication cache.

## Build

The existing `bash build.sh` uses `i686-elf-gcc`, `i686-elf-objcopy`, and NASM.
It writes the image in the repository root.

Windows also has a portable build using Python, Zig **0.13.0**, and NASM:

```powershell
python build.py --zig path/to/zig.exe --nasm path/to/nasm.exe
```

On this machine, tools were downloaded into ignored `.build-tools/`, so
`python build.py` works directly. It produces `.build-test/boot.img` without
overwriting an image that might be running. After closing Bochs, copy that image
to `boot.img` to use the main configuration. The image built for this change is
also installed as the root `boot.img`.

## Verification

```powershell
python -m unittest discover -s tests -v
python tests/bochs_smoke.py --bochs "C:/Program Files/Bochs-3.0/bochs.exe"
python tests/bochs_smoke.py --bochs "C:/Program Files/Bochs-3.0/bochs.exe" --scenario reliability
python tests/bochs_smoke.py --bochs "C:/Program Files/Bochs-3.0/bochs.exe" --scenario errors
python tests/bochs_smoke.py --bochs "C:/Program Files/Bochs-3.0/bochs.exe" --scenario shutdown
python tests/bochs_smoke.py --bochs "C:/Program Files/Bochs-3.0/bochs.exe" --scenario delayed
python tests/bochs_smoke.py --bochs "C:/Program Files/Bochs-3.0/bochs.exe" --scenario nono
python tests/bochs_smoke.py --bochs "C:/Program Files/Bochs-3.0/bochs.exe" --scenario nono-invalid
```

The smoke test boots the actual image headlessly, supplies keyboard-buffer input
through the debugger, and checks VGA memory. It exercises failed/successful login,
`ask`, `ai`, `/exit`, real guest networking, and bridge replies. Reliability mode
checks an 8192-byte answer, pagination, receive-ring wrapping, dropped packets,
and stale/duplicate replies. Error mode checks a visible API error followed by
a successful question. Shutdown mode verifies ACPI power-off and disk lock cleanup.
Automated tests use a mock upstream. A separate live request with the configured
key successfully returned `Connection works` on October 15, 2026.
Delayed mode waits 30 real seconds before answering, covering the virtual-clock
timeout regression. Optional `--scenario live` uses the saved key and makes one
real Gemini request through the guest network and terminal.
Optional `--scenario nono-live` uses Gemini for both the folder/file creation task
and a subsequent file write, then verifies the guest filesystem, not just screen text.

References: [Gemini API](https://ai.google.dev/api) and
[Bochs slirp networking](https://bochs.sourceforge.io/doc/docbook/user/using-slirp.html).
