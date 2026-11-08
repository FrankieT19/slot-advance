/* Opaque span of a scaled RGB555 sprite. Maps contain byte offsets into row.
   Eight pixels per branch; kept in IWRAM for the GBA's limited frame budget. */
.section .iwram,"ax",%progbits
.arm
.align 2
.global Slot_BlitOpaqueRow
.type Slot_BlitOpaqueRow,%function
Slot_BlitOpaqueRow:
    stmfd sp!,{r4-r11,lr}
    cmp r3,#8
    blo 2f
1:
    ldmia r2!,{r4-r11}
    ldrh r4,[r0,r4]
    ldrh r5,[r0,r5]
    ldrh r6,[r0,r6]
    ldrh r7,[r0,r7]
    ldrh r8,[r0,r8]
    ldrh r9,[r0,r9]
    ldrh r10,[r0,r10]
    ldrh r11,[r0,r11]
    strh r4,[r1],#2
    strh r5,[r1],#2
    strh r6,[r1],#2
    strh r7,[r1],#2
    strh r8,[r1],#2
    strh r9,[r1],#2
    strh r10,[r1],#2
    strh r11,[r1],#2
    sub r3,r3,#8
    cmp r3,#8
    bhs 1b
2:
    cmp r3,#4
    blo 5f
    ldmia r2!,{r4-r7}
    ldrh r4,[r0,r4]
    ldrh r5,[r0,r5]
    ldrh r6,[r0,r6]
    ldrh r7,[r0,r7]
    strh r4,[r1],#2
    strh r5,[r1],#2
    strh r6,[r1],#2
    strh r7,[r1],#2
    sub r3,r3,#4
5:
    cmp r3,#0
    beq 4f
3:
    ldr r4,[r2],#4
    ldrh r4,[r0,r4]
    strh r4,[r1],#2
    subs r3,r3,#1
    bne 3b
4:
    ldmfd sp!,{r4-r11,lr}
    bx lr
.size Slot_BlitOpaqueRow,.-Slot_BlitOpaqueRow

/* Aligned BMP staging buffer: convert two BGR555 pixels per word, in IWRAM.
   Preserve the exact magenta key and ignore the unused input bit 15.
   Count is in pixels; an odd final pixel is supported as well. */
.align 2
.global Slot_DecodeBMP555
.type Slot_DecodeBMP555,%function
Slot_DecodeBMP555:
    stmfd sp!,{r4-r8,lr}
    ldr r2,=0x001f001f
    ldr r3,=0x03e003e0
    ldr r4,=0x7c1f
6:
    cmp r1,#2
    blo 7f
    ldr r6,[r0]
    and r7,r6,r2
    mov r7,r7,lsl #10
    and r8,r6,r3
    orr r7,r7,r8
    and r8,r2,r6,lsr #10
    orr r7,r7,r8
    mov r8,r6,lsl #16
    cmp r4,r8,lsr #16
    biceq r7,r7,#0xff
    biceq r7,r7,#0xff00
    orreq r7,r7,#0x8000
    cmp r4,r6,lsr #16
    biceq r7,r7,#0xff0000
    biceq r7,r7,#0xff000000
    orreq r7,r7,#0x80000000
    str r7,[r0],#4
    sub r1,r1,#2
    b 6b
7:
    cmp r1,#0
    beq 8f
    ldrh r6,[r0]
    mov r7,r6,lsr #10
    and r7,r7,#31
    and r8,r6,#31
    orr r7,r7,r8,lsl #10
    and r8,r6,#0x3e0
    orr r7,r7,r8
    cmp r6,r4
    moveq r7,#0x8000
    strh r7,[r0]
8:
    ldmfd sp!,{r4-r8,lr}
    bx lr
.size Slot_DecodeBMP555,.-Slot_DecodeBMP555
.ltorg
