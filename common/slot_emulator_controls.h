/* Physical GBA VBlank clock, independent of emulator fast-forward/slomo. */
#ifndef SLOT_EMULATOR_CONTROLS_H
#define SLOT_EMULATOR_CONTROLS_H
#include "slot_hotkeys.h"
typedef struct {
    unsigned previous,holding,since,blocked;
} SlotEmulatorControls;
static unsigned Slot_EmulatorKeys(SlotEmulatorControls *s,unsigned held,unsigned now,unsigned hold,unsigned menu)
{
    unsigned chord=(held&menu)==menu;
    if(s->blocked){
        if(!(held&(hold|menu)))s->blocked=0;
        s->previous=held;s->holding=0;return 0;
    }
    if(chord && (s->previous&menu)!=menu){
        s->previous=held;s->holding=0;s->blocked=1;return 1;
    }
    s->previous=held;
    if(!hold || !(held&hold) || chord){s->holding=0;return 0;}
    if(!s->holding){s->holding=1;s->since=now;}
    if((unsigned)(now-s->since)>=179){s->holding=0;s->blocked=1;return 2;}
    return 0;
}
#endif
