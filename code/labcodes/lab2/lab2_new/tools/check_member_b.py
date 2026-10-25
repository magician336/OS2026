#!/usr/bin/env python3
"""Build and boot member B's allocator checks on macOS or Linux/WSL.

Each case rebuilds because this Makefile does not track changes to DEFS.
QEMU runs are bounded and terminated after the expected output is observed.
"""

import argparse
import os
from pathlib import Path
import selectors
import shutil
import signal
import subprocess
import tempfile
import time


ROOT = Path(__file__).resolve().parent.parent
SLUB_MARKERS = [
    "[SLUB] size classes/large objects/data integrity: PASS",
    "[SLUB] reuse/bitmap/cross-slab/page reclamation: PASS",
    "[SLUB] alignment/zero/overflow/invalid requests: PASS",
    "[SLUB] real exhaustion/failure/recovery: PASS",
]
BEST_FIT = "[Best-Fit extra] choice/tie/split/fragmentation/merge: PASS"


def stop_group(process):
    try:
        os.killpg(process.pid, signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        process.wait(timeout=2)
    except subprocess.TimeoutExpired:
        os.killpg(process.pid, signal.SIGKILL)
        process.wait()


def run_command(command, log):
    process = subprocess.Popen(
        command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        start_new_session=True,
    )
    try:
        output, _ = process.communicate(timeout=60)
    finally:
        # grade.sh starts a background QEMU; terminate that child as well.
        stop_group(process)
    text = output.decode(errors="replace")
    log.write_text(text, encoding="utf-8")
    if process.returncode != 0:
        raise RuntimeError(f"command failed: {' '.join(command)}\n{text}")
    return text


def build(prefix, defines, logs, label):
    run_command(["make", "clean"], logs / f"{label}-clean.log")
    run_command(["make", f"GCCPREFIX={prefix}", f"DEFS={defines}"],
                logs / f"{label}-build.log")


def boot(qemu, markers, log, timeout, panic=None):
    process = subprocess.Popen(
        [qemu, "-machine", "virt", "-nographic", "-bios", "default",
         "-kernel", "bin/ucore.img"],
        cwd=ROOT, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, start_new_session=True,
    )
    output = bytearray()
    success = False
    deadline = time.monotonic() + timeout
    try:
        with selectors.DefaultSelector() as selector:
            selector.register(process.stdout, selectors.EVENT_READ)
            while time.monotonic() < deadline:
                for key, _ in selector.select(timeout=0.1):
                    chunk = os.read(key.fileobj.fileno(), 65536)
                    if not chunk:
                        selector.unregister(key.fileobj)
                        continue
                    output.extend(chunk)
                text = output.decode(errors="replace").replace("\r", "")
                normal_pass = all(marker in text for marker in markers)
                has_panic = "kernel panic" in text
                if panic is None:
                    if has_panic:
                        break
                    success = normal_pass
                else:
                    success = normal_pass and has_panic and panic in text
                    if "SLUB negative test unexpectedly returned" in text:
                        success = False
                        break
                if success or process.poll() is not None:
                    break
    finally:
        stop_group(process)
        process.stdout.close()
    text = output.decode(errors="replace").replace("\r", "")
    log.write_text(text, encoding="utf-8")
    if not success:
        raise RuntimeError(f"QEMU checks failed or timed out; see {log}\n{text}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--gccprefix")
    parser.add_argument("--qemu", default="qemu-system-riscv64")
    parser.add_argument("--timeout", type=float, default=20)
    parser.add_argument("--log-dir", type=Path)
    args = parser.parse_args()
    prefixes = [args.gccprefix] if args.gccprefix else [
        "riscv64-unknown-elf-", "riscv64-elf-"
    ]
    prefix = next((p for p in prefixes if all(
        shutil.which(p + tool) for tool in ("gcc", "ld", "objcopy", "objdump")
    )), None)
    qemu = shutil.which(args.qemu)
    if prefix is None or qemu is None:
        parser.error("RISC-V GNU toolchain or QEMU was not found")
    logs = args.log_dir or Path(tempfile.mkdtemp(prefix="os2026-member-b-"))
    logs = logs.resolve()
    logs.mkdir(parents=True, exist_ok=True)
    print(f"Logs: {logs}", flush=True)

    cases = [
        ("best-fit", "-DUSE_BEST_FIT_PMM", "best_fit_pmm_manager"),
        ("first-fit", "-DUSE_FIRST_FIT_PMM", "default_pmm_manager"),
        ("buddy", "-DUSE_BUDDY_PMM", "buddy_pmm_manager"),
    ]
    for label, defines, manager in cases:
        build(prefix, defines, logs, label)
        markers = SLUB_MARKERS + [f"[SLUB] all checks passed ({manager})"]
        if label == "best-fit":
            markers.append(BEST_FIT)
        boot(qemu, markers, logs / f"{label}-qemu.log", args.timeout)
        print(f"PASS: {label} + SLUB", flush=True)

    negatives = [
        (1, "small-interior", "kfree: pointer is not a live object"),
        (2, "small-double-free", "kfree: object is already free"),
        (3, "foreign-pointer", "kfree: pointer is not a live object"),
        (4, "large-interior", "kfree: pointer is not a live object"),
        (5, "large-double-free", "kfree: pointer is not a live object"),
    ]
    for number, label, panic in negatives:
        defines = f"-DUSE_BEST_FIT_PMM -DSLUB_NEGATIVE_TEST={number}"
        build(prefix, defines, logs, label)
        boot(qemu, SLUB_MARKERS + [BEST_FIT], logs / f"{label}-qemu.log",
             args.timeout, panic=panic)
        print(f"PASS: {label} rejected with expected panic", flush=True)

    grade = run_command(
        ["make", f"GCCPREFIX={prefix}", "DEFS=-DUSE_BEST_FIT_PMM", "grade"],
        logs / "grade.log",
    )
    if "Total Score: 20/20" not in grade:
        raise RuntimeError(f"grade did not report 20/20:\n{grade}")
    print("PASS: course grade 20/20", flush=True)
    build(prefix, "-DUSE_BEST_FIT_PMM", logs, "final-best-fit")
    print(f"All member B checks passed. Logs: {logs}", flush=True)


if __name__ == "__main__":
    main()
