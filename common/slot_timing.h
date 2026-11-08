/* Shared VBlank navigation timebase. */
#ifndef SLOT_TIMING_H
#define SLOT_TIMING_H
#include "slot_motion.h"
#include "slot_navigation.h"
static volatile unsigned slot_vblanks;
static volatile SlotNavigation slot_navigation;
static unsigned Slot_NavLock(void) {
#ifdef __arm__
    unsigned previous=REG_IME;REG_IME=0;return previous;
#else
    return 0;
#endif
}
static void Slot_NavUnlock(unsigned previous) {
#ifdef __arm__
    REG_IME=previous;
#else
    (void)previous;
#endif
}
static void Slot_ClearNav(void) {unsigned irq=Slot_NavLock();slot_navigation.enabled=0;slot_navigation.direction=0;Slot_NavUnlock(irq);}
static void Slot_ResumeNav(unsigned count,unsigned index,unsigned wrap,unsigned animated)
{unsigned irq=Slot_NavLock();SlotNav_Reset(&slot_navigation,count,index,wrap,animated);Slot_NavUnlock(irq);}
static void Slot_NavSnapshot(unsigned *index,int *pose,unsigned *steps)
{unsigned irq=Slot_NavLock();*index=slot_navigation.index;*pose=SlotNav_Pose(slot_navigation.offset);*steps=slot_navigation.steps;Slot_NavUnlock(irq);}
static void Slot_VBlank(void) {
    slot_vblanks++;
#ifdef __arm__
    SlotNav_Tick(&slot_navigation,(~REG_KEYINPUT)&0x3ff,(unsigned)(((uint64_t)slot_vblanks*4389000)>>18));
#endif
}
static unsigned Slot_Ease(unsigned elapsed,unsigned duration)
{if(elapsed>=duration)return 256;return Slot_Smooth(elapsed*256/duration);}
#define SLOT_INSERT_FRAMES 24
#endif
