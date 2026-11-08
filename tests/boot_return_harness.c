/* Re-enter the real devkitARM CRT, including its full EWRAM clear. */
#include <gba.h>
#include <stdint.h>
#include "slot_return.h"
#define RESULT ((volatile uint32_t*)0x0203f000)
int main(void)
{
    volatile uint32_t *cookie=(volatile uint32_t*)SLOT_RETURN_COOKIE_ADDRESS;
    unsigned warm=cookie[0]==SLOT_RETURN_COOKIE_MAGIC && cookie[1]==~SLOT_RETURN_COOKIE_MAGIC &&
        cookie[2]==0x1234abcd && cookie[3]==~0x1234abcdu;
    RESULT[0]=0x42543132;RESULT[1]=warm;
    RESULT[2]=*(volatile uint32_t*)0x02001000;
    irqInit();irqEnable(IRQ_VBLANK);SetMode(MODE_3|BG2_ENABLE);
    while(1){
        VBlankIntrWait();
        if(RESULT[16]==1){
            cookie[0]=SLOT_RETURN_COOKIE_MAGIC;cookie[1]=~SLOT_RETURN_COOKIE_MAGIC;
            cookie[2]=0x1234abcd;cookie[3]=~0x1234abcdu;
            *(volatile uint32_t*)0x02001000=0xfeedbeef;
            REG_IME=0;
            __asm__ volatile("bx %0"::"r"(0x08000000):"memory");
        }
    }
}
