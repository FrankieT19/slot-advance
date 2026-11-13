/* Count VBlank ticks only, in the scratch area DS Style already reserves for
   its addon wrapper. Never write a counter into game ROM or steal a timer.
   r1: IE/IF snapshot, r2: active-low keys, r0: IO base. r0/r2 are preserved. */
.macro SLOT_HOLD_TICK scratch, hold_key, menu_keys, reset_target
    tst r1,#1
    beq .Lhold_exit\@
    tst r1,#0x10000
    beq .Lhold_exit\@
    stmfd sp!,{r1}
    adrl r12,\scratch
    ldr r12,[r12]
    adrl r3,\hold_key
    ldr r3,[r3]
    cmp r3,#0
    beq .Lhold_clear\@
    tst r2,r3
    bne .Lhold_clear\@
    adrl r3,\menu_keys
    ldr r3,[r3]
    tst r2,r3
    beq .Lhold_clear\@
    ldr r3,[r12,#0x7c]
    mov r1,r3,lsr #16
    sub r1,r1,#0x5300
    cmp r1,#0x4c
    movne r3,#0
    andeq r3,r3,#255
    cmp r3,#180
    movhs r3,#0
    add r3,r3,#1
    cmp r3,#180
    bhs .Lhold_reset\@
    mov r1,#0x53000000
    orr r1,r1,#0x004c0000
    orr r3,r3,r1
    str r3,[r12,#0x7c]
    b .Lhold_done\@
.Lhold_clear\@:
    mov r3,#0
    str r3,[r12,#0x7c]
    b .Lhold_done\@
.Lhold_reset\@:
    mov r3,#0
    str r3,[r12,#0x7c]
    ldmfd sp!,{r1}
    b \reset_target
.Lhold_done\@:
    ldmfd sp!,{r1}
.Lhold_exit\@:
.endm
