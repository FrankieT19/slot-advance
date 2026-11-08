/* Slot shelf.rs navigation, adapted to fixed point for ARM7TDMI.
   One continuously retargeted critically damped spring; no input queue.
   Upstream spring: slot-cfw/slot 0fe8f5c, OMEGA=16.
   GBA hold: 400 ms delay, steady 200 ms repeat. Never gated by loading/drawing. */
#ifndef SLOT_NAVIGATION_H
#define SLOT_NAVIGATION_H
#include <stdint.h>
#define SLOT_NAV_DECAY 821409328
#define SLOT_HOLD_DELAY_MS 400
#define SLOT_HOLD_REPEAT_MS 200
typedef struct {
    unsigned enabled,count,index,wrap,animated,previous,due,fired,steps;
    int direction;
    int32_t offset,velocity; /* Q16, relative to the selected target. */
} SlotNavigation;
static void SlotNav_Reset(volatile SlotNavigation *n,unsigned count,unsigned index,unsigned wrap,unsigned animated)
{
    n->enabled=0;n->count=count;n->index=index;n->wrap=wrap;n->animated=animated;
    n->direction=0;n->fired=0;n->steps=0;n->offset=0;n->velocity=0;
    /* Preserve previous: a button held across a modal is not a new press. */
    n->enabled=1;
}
static void SlotNav_Step(volatile SlotNavigation *n,int by)
{
    int target=(int)n->index+by;
    if(n->count<2)return;
    if(target<0){if(!n->wrap)return;target=n->count-1;}
    if((unsigned)target>=n->count){if(!n->wrap)return;target=0;}
    n->index=target;n->offset-=by*65536;n->steps++;
}
static void SlotNav_Tick(volatile SlotNavigation *n,unsigned held,unsigned now)
{
    unsigned horizontal=held&48;
    if(horizontal==48)horizontal=0;
    if(!n->enabled){n->previous=horizontal;n->direction=0;return;}
    if(horizontal!=n->previous) {
        n->direction=horizontal?(horizontal&16?1:-1):0;
        if(n->direction){SlotNav_Step(n,n->direction);n->due=now+SLOT_HOLD_DELAY_MS;n->fired=0;}
    }
    n->previous=horizontal;
    if(n->direction && (int32_t)(now-n->due)>=0) {
        SlotNav_Step(n,n->direction);
        n->due=now+SLOT_HOLD_REPEAT_MS;
    }
    if(!n->animated){n->offset=0;n->velocity=0;}
    else {
        /* Exact GBA VBlank dt=280896/16777216 s. Q30 exp(-16*dt).
           Closed-form spring update, matching Slot rather than Euler steps. */
        int32_t pull=n->velocity+16*n->offset;
        int32_t off=n->offset+(int32_t)(((int64_t)pull*17977344)>>30);
        int32_t vel=n->velocity-(int32_t)(((int64_t)16*pull*17977344)>>30);
        n->offset=(int32_t)(((int64_t)off*SLOT_NAV_DECAY)>>30);
        n->velocity=(int32_t)(((int64_t)vel*SLOT_NAV_DECAY)>>30);
        if(n->offset>-8 && n->offset<8 && n->velocity>-128 && n->velocity<128){n->offset=0;n->velocity=0;}
    }
}
static int SlotNav_Pose(int32_t offset)
{
    return (offset+128)>>8; /* Q8 pose; at most 0.24 GBA pixels of rounding. */
}
static unsigned Slot_DialogCanClose(unsigned *armed,unsigned held,unsigned down)
{
    if(!*armed){if(!held)*armed=1;return 0;}
    return !!(down&3); /* fresh A/B only, after releasing the entering keys */
}
#endif
