/* Freeze emulator drawing and fade the existing display at physical VBlanks. */
#ifndef SLOT_EMULATOR_FADE_H
#define SLOT_EMULATOR_FADE_H
#include <stdint.h>
static void Slot_EmulatorWaitBlank(void)
{
    volatile uint16_t *scanline=(volatile uint16_t*)0x04000006;
    while(*scanline>=160){}
    while(*scanline<160){}
}
static void Slot_EmulatorFadeBlack(void)
{
    unsigned level;
    *(volatile uint16_t*)0x04000208=0;
    /* Keep the frozen HBlank scanline data while preventing IRQ redraws.
       Enable the fade in both windows, the outside area and the OBJ window. */
    *(volatile uint16_t*)0x04000048|=0x2020;
    *(volatile uint16_t*)0x0400004a|=0x2020;
    for(level=0;level<=16;level+=2){
        Slot_EmulatorWaitBlank();
        *(volatile uint16_t*)0x04000050=0x00ff;
        *(volatile uint16_t*)0x04000054=level;
    }
}
#endif
