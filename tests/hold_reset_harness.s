/* Emulator-only ABI harness for the exact hold-reset macro injected in games. */
.arm
.text
.include "slot_hold_reset.inc"
.global Slot_TestHoldStep
.type Slot_TestHoldStep,%function
.global Slot_TestHoldDisabled
.type Slot_TestHoldDisabled,%function
.global Slot_TestHoldClear
.type Slot_TestHoldClear,%function
Slot_TestHoldStep:
    stmfd sp!,{r4,lr}
    mov r4,r1
    mov r2,r0
    mov r0,#0x04000000
    SLOT_HOLD_TICK Slot_TestScratch,Slot_TestEnabled,Slot_TestReset
    cmp r1,r4
    moveq r0,#0
    movne r0,#2
    ldmfd sp!,{r4,lr}
    bx lr
Slot_TestReset:
    cmp r1,r4
    moveq r0,#1
    movne r0,#2
    ldmfd sp!,{r4,lr}
    bx lr
Slot_TestHoldDisabled:
    stmfd sp!,{r4,lr}
    mov r4,r1
    mov r2,r0
    mov r0,#0x04000000
    SLOT_HOLD_TICK Slot_TestScratch,Slot_TestDisabled,Slot_TestReset
    cmp r1,r4
    moveq r0,#0
    movne r0,#2
    ldmfd sp!,{r4,lr}
    bx lr
Slot_TestHoldClear:
    adr r1,Slot_TestScratch
    ldr r1,[r1]
    mov r0,#0
    str r0,[r1,#0x7c]
    bx lr
Slot_TestScratch: .word 0x0203ee00
Slot_TestEnabled: .word 1
Slot_TestDisabled: .word 0


