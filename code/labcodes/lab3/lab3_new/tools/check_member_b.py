"""Reproduce member B's tests on QEMU 11 using a disposable source copy."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

source = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="lab3-b-") as temp:
    root = Path(temp) / "src"
    shutil.copytree(source, root, ignore=shutil.ignore_patterns("bin", "obj"))
    entry = root / "kern/init/entry.S"
    # The physical PC needs a mapping until kern_entry jumps to the high VA.
    asm = entry.read_text()
    assert asm.count(".zero 8 * 511") == 1
    entry.write_text(asm.replace(".zero 8 * 511",
                                 ".zero 8 * 2\n    .quad (0x80000 << 10) | 0xcf\n    .zero 8 * 508"))
    init = root / "kern/init/init.c"
    marker = "    clock_init();"
    code = r'''    asm volatile(".2byte 0x0000\n.4byte 0x0000007b\n.2byte 0x9002\n.4byte 0x00100073");
    cputs("B: four exception returns");
'''
    src = init.read_text()
    assert src.count(marker) == 1
    init.write_text(src.replace(marker, code + marker))
    make = ["make", "GCCPREFIX=" + os.environ.get("GCCPREFIX", "riscv64-elf-")]
    subprocess.run(make, cwd=root, check=True)
    run = subprocess.run(make + ["QEMU=qemu-system-riscv64 -kernel bin/ucore.img", "qemu"],
                         cwd=root, check=True, capture_output=True, text=True, timeout=30)
    print(run.stdout)
    events = re.findall(r"^(Illegal instruction|Breakpoint) at 0x([0-9a-f]+)$",
                        run.stdout, re.MULTILINE)
    assert [name for name, _ in events] == ["Illegal instruction"] * 2 + ["Breakpoint"] * 2
    pcs = [int(pc, 16) for _, pc in events]
    assert [pc - pcs[0] for pc in pcs] == [0, 2, 6, 8]
    assert run.stdout.splitlines().count("B: four exception returns") == 1
    assert run.stdout.splitlines().count("100 ticks") == 10
    print("PASS: four exception returns, ten reports, QEMU exited normally")
