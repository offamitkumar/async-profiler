/*
 * Copyright The async-profiler authors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __s390x__

#include <errno.h>
#include <sys/syscall.h>
#include "stackFrame.h"
#include "vmStructs.h"

// On s390x Linux, ucontext_t::uc_mcontext.gregs[] holds general-purpose
// registers r0–r15. The PSW (program status word, i.e. the PC) is in
// uc_mcontext.psw.addr.
//
// Register roles in the s390x Linux ABI:
//   r0       - caller-saved / return value (high half for 64-bit)
//   r1       - caller-saved
//   r2       - first integer argument / return value
//   r3       - second integer argument
//   r4       - third integer argument
//   r5       - fourth integer argument
//   r6       - fifth integer argument (callee-saved)
//   r7-r11   - callee-saved; r11 used as frame pointer by JVM
//   r12      - callee-saved / GOT pointer
//   r13      - callee-saved / literal pool pointer
//   r14      - return address (link register)
//   r15      - stack pointer
//
// JVM-specific register assignments (HotSpot s390x):
//   r6       - method register (current Method*)
//   r14      - link / return address
//   r11      - frame pointer (Java frame's FP)
//   r7       - first Java argument (jarg0)
//   r8       - sender SP

#define GREGS(n)  _ucontext->uc_mcontext.gregs[n]
// psw.addr stores the PC. On s390x the PSW address mask (bit 31) indicates
// 64-bit addressing mode; the actual instruction address is the lower 64 bits
// with that mode bit stripped. We expose the raw field as a reference for
// read/write, so callers that set pc() write back a clean address.
#define PSW_ADDR  _ucontext->uc_mcontext.psw.addr

uintptr_t& StackFrame::pc() {
    // Strip the 64-bit addressing-mode bit (bit 31 from the right in the
    // 64-bit PSW address word) before returning the PC.
    // The field itself is writable so we return a reference to it; the
    // masking only affects what we read back via pc(), not what we store.
    PSW_ADDR &= 0x7fffffffffffffffUL;
    return (uintptr_t&)PSW_ADDR;
}

uintptr_t& StackFrame::sp() {
    return (uintptr_t&)GREGS(15);
}

uintptr_t& StackFrame::fp() {
    return (uintptr_t&)GREGS(11);
}

uintptr_t& StackFrame::retval() {
    return (uintptr_t&)GREGS(2);
}

uintptr_t StackFrame::link() {
    return (uintptr_t)GREGS(14);
}

uintptr_t StackFrame::arg0() {
    return (uintptr_t)GREGS(2);
}

uintptr_t StackFrame::arg1() {
    return (uintptr_t)GREGS(3);
}

uintptr_t StackFrame::arg2() {
    return (uintptr_t)GREGS(4);
}

uintptr_t StackFrame::arg3() {
    return (uintptr_t)GREGS(5);
}

uintptr_t StackFrame::jarg0() {
    // On s390x HotSpot, the first Java argument is in r2 (same as arg0)
    return arg0();
}

uintptr_t StackFrame::method() {
    // HotSpot s390x: Method* is in r6
    return (uintptr_t)GREGS(6);
}

uintptr_t StackFrame::senderSP() {
    // HotSpot s390x: sender SP is in r8
    return (uintptr_t)GREGS(8);
}

void StackFrame::ret() {
    pc() = link();
}

bool StackFrame::unwindStub(instruction_t* entry, const char* name, uintptr_t& pc, uintptr_t& sp, uintptr_t& fp) {
    instruction_t* ip = (instruction_t*)pc;
    if (ip == entry
        || startsWith(name, "itable")
        || startsWith(name, "vtable")
        || streq(name, "InlineCacheBuffer"))
    {
        pc = link();
        return true;
    }
    return false;
}

bool StackFrame::unwindPrologue(NMethod* nm, uintptr_t& pc, uintptr_t& sp, uintptr_t& fp) {
    // Not yet implemented
    return false;
}

bool StackFrame::unwindEpilogue(NMethod* nm, uintptr_t& pc, uintptr_t& sp, uintptr_t& fp) {
    // Not yet implemented
    return false;
}

bool StackFrame::unwindAtomicStub(const void*& pc) {
    // Not needed
    return false;
}

void StackFrame::adjustSP(const void* entry, const void* pc, uintptr_t& sp) {
    // Not yet implemented
}

bool StackFrame::checkInterruptedSyscall() {
    return retval() == (uintptr_t)-EINTR;
}

bool StackFrame::isSyscall(instruction_t* pc) {
    // s390x SVC instruction: opcode 0x0A (upper byte of first halfword)
    return (*pc >> 8) == 0x0A;
}

#endif // __s390x__
