"""Source this file in gdb-multiarch with the Lab2 kernel ELF loaded."""
import json
import os
import gdb

OUT = os.environ["PAGEWALK_OUT"]


def val(expr):
    return int(gdb.parse_and_eval(expr)) & ((1 << 64) - 1)


gdb.execute("set pagination off")
gdb.execute("set confirm off")
gdb.execute("set tcp auto-retry on")
gdb.execute("set tcp connect-timeout 30")
gdb.execute("target remote 127.0.0.1:1234")
gdb.execute("tbreak kern_init")
gdb.execute("continue")
start = val("$pc")
instructions = gdb.selected_frame().architecture().disassemble(start, start + 64)
store = next(i["addr"] for i in instructions
             if i["asm"].replace(" ", "").replace("\t", "").startswith("sdra,8(sp)"))
gdb.execute("tbreak *%#x" % store)
gdb.execute("continue")
gdb.execute("x/i $pc")
gdb.execute("info registers pc sp ra satp")
data = dict(pc=val("$pc"), sp=val("$sp"), ra=val("$ra"), satp=val("$satp"))
data["store_size"] = next(i["length"] for i in instructions if i["addr"] == store)
data["addr"] = data["sp"] + 8
data["root_virtual"] = val("&boot_page_table_sv39")
data["pte"] = val("((unsigned long long *)&boot_page_table_sv39)[511]")
data["steps"] = []
for number in (1, 2):
    if number == 2:
        # Repeat only the same idempotent stack store; do not flush the TLB.
        gdb.execute("set $pc = %#x" % store)
    gdb.execute("si")
    gdb.execute("x/i $pc")
    value = val("*(unsigned long long *)%#x" % data["addr"])
    data["steps"].append(dict(number=number, pc=val("$pc"), stored=value))
    print("STORE %d: VA=%#x value=%#x" % (number, data["addr"], value))
data["physical"] = ((data["pte"] >> 10) << 12) | (data["addr"] & ((1 << 30) - 1))
reply = gdb.execute("maintenance packet Qqemu.PhyMemMode:1", to_string=True)
print(reply)
assert '"OK"' in reply, "QEMU did not enable physical-memory debug access"
try:
    data["physical_value"] = int.from_bytes(
        bytes(gdb.selected_inferior().read_memory(data["physical"], 8)), "little")
    print("PHYSICAL: PA=%#x value=%#x" % (data["physical"], data["physical_value"]))
finally:
    gdb.execute("maintenance packet Qqemu.PhyMemMode:0")
with open(OUT + "/guest.json", "w") as f:
    json.dump(data, f, indent=2)
print("GUEST " + json.dumps(data))
gdb.execute("detach")
