#!/usr/bin/env python3
"""Run native + guest GDB in Linux; save evidence outside the repository."""
import argparse
import json
import os
from pathlib import Path
import signal
import subprocess
import time

p = argparse.ArgumentParser()
p.add_argument("--qemu", required=True)
p.add_argument("--source", required=True)
p.add_argument("--kernel-dir", required=True, help="Directory containing bin/kernel and bin/ucore.img")
p.add_argument("--out", required=True, help="New evidence directory, outside Git")
a = p.parse_args()
version = subprocess.check_output([a.qemu, "--version"], text=True).splitlines()[0]
if version != "QEMU emulator version 4.1.1":
    p.error("Source breakpoints in this experiment require QEMU 4.1.1")
out = Path(a.out).resolve()
out.mkdir(parents=True, exist_ok=False)
here = Path(__file__).resolve().parent
kernel = Path(a.kernel_dir).resolve()
env = dict(os.environ, PAGEWALK_OUT=str(out), QEMU_SRC=str(Path(a.source).resolve()))
qargs = [a.qemu, "-L", str(Path(a.source) / "pc-bios"), "-machine", "virt",
         "-m", "128M", "-smp", "1", "-display", "none", "-monitor", "none",
         "-serial", "file:" + str(out / "console.log"), "-bios", "default",
         "-kernel", str(kernel / "bin/ucore.img"), "-gdb", "tcp:127.0.0.1:1234", "-S"]
with open(out / "host.log", "w") as hf, open(out / "guest.log", "w") as gf:
    host = subprocess.Popen(["gdb", "-q", "-nx", "-batch", "-ex", "set pagination off",
        "-ex", "set confirm off", "-ex", "source " + str(here / "host.py"),
        "-ex", "run", "-ex", "quit", "--args"] + qargs,
        stdout=hf, stderr=subprocess.STDOUT, env=env, start_new_session=True)
    try:
        # GDB's auto-retry waits for QEMU to open its gdbstub; do not probe with
        # an extra socket connection that QEMU might mistake for the debugger.
        subprocess.run(["gdb-multiarch", "-q", "-nx", "-batch",
                        str(kernel / "bin/kernel"), "-ex", "source " + str(here / "guest.py")],
                       stdout=gf, stderr=subprocess.STDOUT, env=env, cwd=kernel, timeout=120, check=True)
        deadline = time.monotonic() + 60
        while time.monotonic() < deadline:
            if (out / "console.log").exists() and "[SLUB] all checks passed" in (out / "console.log").read_text():
                break
            time.sleep(0.2)
        else:
            raise RuntimeError("Kernel did not complete SLUB checks; inspect console.log")
    finally:
        if host.poll() is None:
            os.killpg(host.pid, signal.SIGINT)
            try:
                host.wait(timeout=10)
            except subprocess.TimeoutExpired:
                os.killpg(host.pid, signal.SIGKILL)
                host.wait()

guest = json.loads((out / "guest.json").read_text())
events = json.loads((out / "host.json").read_text())
by_kind = {kind: [e for e in events if e["kind"] == kind]
           for kind in ("walk", "pte", "translated", "fill", "entry")}
assert len(by_kind["walk"]) == 1, "Expected one walk for two identical stores"
assert len(by_kind["pte"]) == 1, "This kernel should use one top-level leaf"
assert all(len(by_kind[k]) == 1 for k in ("translated", "fill", "entry")), "Expected one translation and TLB insertion"
w, pt, tr, fill, entry = (by_kind[k][0] for k in by_kind)
addr = guest["addr"]
assert w["addr"] == addr and w["satp"] == guest["satp"]
assert pt["i"] == 0 and pt["ptshift"] == 18 and pt["idx"] == ((addr >> 30) & 511)
assert pt["pte"] == guest["pte"] and (pt["pte"] & 15) == 15
root = (guest["satp"] & ((1 << 44) - 1)) << 12
assert pt["pte_addr"] == root + pt["idx"] * 8
physical = ((pt["pte"] >> 10) << 12) | (addr & ((1 << 30) - 1))
assert tr["result"] == 0 and tr["physical"] == (physical & ~4095) and tr["prot"] == 7
assert fill["vaddr"] == (addr & ~4095) and fill["paddr"] == (physical & ~4095)
assert fill["size"] == 4096 and (entry["addr_write"] & ~4095) == (addr & ~4095)
assert len(guest["steps"]) == 2
assert all(s["stored"] == guest["ra"] and s["pc"] == guest["pc"] + guest["store_size"] for s in guest["steps"])
assert guest["physical"] == physical and guest["physical_value"] == guest["ra"]
summary = dict(result="PASS", virtual=hex(addr), physical=hex(physical),
               pte=hex(pt["pte"]), root=hex(root), pte_address=hex(pt["pte_addr"]),
               stores=2, walks=len(by_kind["walk"]), levels_visited=1,
               kernel_self_checks="PASS")
(out / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
print(json.dumps(summary, indent=2))
print("Evidence:", out)
