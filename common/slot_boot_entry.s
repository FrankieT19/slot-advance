/* The standard header and secondary entry branch remain unchanged. The CRT's
   first instruction branches here, before BSS setup; replay it before return.
   This controls kernel-owned frames, not a cartridge firmware loader. */
.arm
.section .text
.global Slot_BlackEntry
Slot_BlackEntry:
    ldr r0, Slot_BlackIO
    mov r1, #0
    add r2,r0,#0x200
    strh r1, [r2, #8]
    ldr r1, Slot_BlackMode
    strh r1, [r0]
    mov r1, #0xff
    strh r1, [r0, #0x50]
    mov r1, #16
    strh r1, [r0, #0x54]
    mov r0, #0x04000000  /* Replay the CRT instruction displaced by our branch. */
    ldr pc, Slot_OriginalEntry
Slot_BlackIO: .word 0x04000000
Slot_BlackMode: .word 0x0403
Slot_OriginalEntry: .word 0x12345678
