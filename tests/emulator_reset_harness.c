/* Exercise the actual copied reset stub, BIOS calls and black-entry CRT hook. */
#include <gba.h>
#include <stdint.h>
#include "slot_emulator_fade.h"
#include "slot_return.h"
#define RESULT ((volatile uint32_t*)0x0203f000)
extern void Slot_EmulatorReset(void);
void Slot_JagoombaMarkReturn(void)
{
    volatile uint32_t *cookie=(volatile uint32_t*)SLOT_RETURN_COOKIE_ADDRESS;
    cookie[0]=SLOT_RETURN_COOKIE_MAGIC;cookie[1]=~SLOT_RETURN_COOKIE_MAGIC;
    cookie[2]=0x1234abcd;cookie[3]=~0x1234abcdu;
}
int main(void)
{
    volatile uint32_t *cookie=(volatile uint32_t*)SLOT_RETURN_COOKIE_ADDRESS;
    unsigned warm=cookie[0]==SLOT_RETURN_COOKIE_MAGIC && cookie[1]==~SLOT_RETURN_COOKIE_MAGIC &&
        cookie[2]==0x1234abcd && cookie[3]==~0x1234abcdu;
    unsigned i;int level;
    for(i=0;i<4;i++)cookie[i]=0;
    irqInit();irqEnable(IRQ_VBLANK);SetMode(MODE_3|BG2_ENABLE);
    REG_BLDCNT=0xff;REG_BLDY=16;
    for(i=0;i<240*160;i++)((volatile uint16_t*)0x06000000)[i]=warm?RGB5(8,10,18):RGB5(12,20,10);
    if(warm){
        for(level=16;level>=0;level-=2){VBlankIntrWait();REG_BLDY=level;}
    }
    REG_BLDCNT=0;REG_BLDY=0;
    RESULT[0]=0x42524644;RESULT[1]=warm;
    while(1){
        VBlankIntrWait();
        if(RESULT[16]==1){Slot_EmulatorFadeBlack();Slot_EmulatorReset();}
    }
}
