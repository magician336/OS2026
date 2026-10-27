#include <trap.h>

/*
 * M0 scaffold: member B implements the real handlers in trap.c.
 * These weak definitions keep the lab3 framework linkable before that work
 * lands; the strong definitions in trap.c override them at link time.
 */
__attribute__((weak)) void
interrupt_handler(struct trapframe *tf) {
    print_trapframe(tf);
}

__attribute__((weak)) void
exception_handler(struct trapframe *tf) {
    print_trapframe(tf);
}
