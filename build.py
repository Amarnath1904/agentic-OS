"""Portable Windows/Linux build with Zig 0.13 and NASM (no WSL required).

python build.py --zig path/to/zig --nasm path/to/nasm
Defaults also find the local .build-tools installations used during development.
The original i686-elf build.sh remains supported.
"""
import argparse
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--zig", default=shutil.which("zig") or str(ROOT / ".build-tools/ziglang/zig.exe"))
    parser.add_argument("--nasm", default=shutil.which("nasm") or str(ROOT / ".build-tools/nasm-2.16.03/nasm.exe"))
    parser.add_argument("--output", type=Path, default=ROOT / ".build-test")
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)

    def run(*command):
        subprocess.run([str(item) for item in command], cwd=ROOT, check=True)

    flags = ["-target", "x86-freestanding-none", "-march=i686", "-mno-sse", "-mno-mmx",
             "-std=gnu99", "-ffreestanding", "-fno-stack-protector", "-fno-pic",
             "-O2", "-Wall", "-Wextra", "-I.", "-Ikernel"]
    objects = []
    for source in ["kernel/entry.asm", "kernel/gdt.asm", "kernel/interrupts.asm", "kernel/syscalls/syscall.asm"]:
        obj = out / (source.replace("/", "_") + ".o")
        run(args.nasm, "-f", "elf32", "-o", obj, source)
        objects.append(obj)
    for source in sorted((ROOT / "kernel").rglob("*.c")):
        relative = source.relative_to(ROOT).as_posix()
        obj = out / (relative.replace("/", "_") + ".o")
        run(args.zig, "cc", *flags, "-c", source, "-o", obj)
        objects.append(obj)
    elf = out / "kernel.elf"
    run(args.zig, "cc", *flags, "-nostdlib", "-Wl,-T,linker.ld", "-Wl,--build-id=none",
        *objects, "-o", elf)
    run(args.zig, "objcopy", "-O", "binary", elf, out / "kernel.bin")
    kernel = (out / "kernel.bin").read_bytes()
    sectors = (len(kernel) + 511) // 512
    if sectors > 127:
        raise SystemExit("Kernel exceeds the bootloader's single EDD read limit (127 sectors).")
    run(args.nasm, "-f", "bin", f"-DKERNEL_SECTORS={sectors}", "boot.asm", "-o", out / "boot.bin")
    image = (out / "boot.bin").read_bytes() + kernel
    # Whole 63-sector cylinders match the checked-in Bochs geometry.
    size = (len(image) + 63 * 512 - 1) // (63 * 512) * (63 * 512)
    (out / "boot.img").write_bytes(image + bytes(size - len(image)))
    print(f"Built {out / 'boot.img'} ({len(kernel)} kernel bytes, {sectors} sectors)")


if __name__ == "__main__":
    main()
