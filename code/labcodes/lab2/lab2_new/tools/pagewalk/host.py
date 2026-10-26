"""Source this file in native GDB debugging QEMU v4.1.1 (-O0 -g)."""
import json
import os
import gdb

OUT = os.environ["PAGEWALK_OUT"]
SRC = os.environ.get("QEMU_SRC", "/opt/qemu-src")
events = []
selected = None


def val(expr):
    return int(gdb.parse_and_eval(expr))


def record(kind, **fields):
    if os.path.exists(OUT + "/guest.json"):
        return
    event = dict(kind=kind, **fields)
    events.append(event)
    print("PAGEWALK " + json.dumps(event))
    with open(OUT + "/host.json", "w") as f:
        json.dump(events, f, indent=2)


class Done(gdb.FinishBreakpoint):
    def __init__(self):
        self.physical = gdb.parse_and_eval("physical")
        self.prot = gdb.parse_and_eval("prot")
        super().__init__(internal=True)

    def stop(self):
        record("translated", result=int(self.return_value),
               physical=int(self.physical.dereference()),
               prot=int(self.prot.dereference()))
        return False


class Walk(gdb.Breakpoint):
    def stop(self):
        global selected
        addr = val("addr")
        if val("access_type") != 1 or val("env->satp") >> 60 != 8 or addr <= 0xffffffffc0000000:
            return False
        if selected is None:
            selected = addr
        if addr == selected:
            record("walk", addr=addr, satp=val("env->satp"),
                   access_type=val("access_type"), mmu_idx=val("mmu_idx"))
            print(gdb.execute("bt 6", to_string=True))
            Done()
        return False


class Pte(gdb.Breakpoint):
    def stop(self):
        if selected == val("addr") and val("access_type") == 1:
            record("pte", **{name: val(name) for name in
                   ("i", "idx", "ptshift", "base", "pte_addr", "pte", "ppn")})
        return False


class Fill(gdb.Breakpoint):
    def stop(self):
        if selected is not None and val("vaddr") == selected & ~4095:
            record("fill", **{name: val(name) for name in
                   ("vaddr", "paddr", "prot", "mmu_idx", "size")})
        return False


class Entry(gdb.Breakpoint):
    def stop(self):
        if selected is not None and val("vaddr") == selected & ~4095:
            record("entry", index=val("index"),
                   addr_read=val("te->addr_read"), addr_write=val("te->addr_write"),
                   addr_code=val("te->addr_code"), addend=val("te->addend"))
        return False


Walk("get_physical_address", internal=True)
Pte(SRC + "/target/riscv/cpu_helper.c:256", internal=True)
Fill("tlb_set_page", internal=True)
Entry(SRC + "/accel/tcg/cputlb.c:836", internal=True)
