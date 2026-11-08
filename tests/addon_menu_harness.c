/* Offline emulator fixture: actual injected menu/marker, with SRAM bank shims. */
#include <gba.h>
#include <stdint.h>
#include <string.h>
#include "slot_return.h"
extern void ingamemenu_now(void),Slot_MarkReturn(void);
extern unsigned Slot_RTS_Background,Slot_RTS_Panel,Slot_RTS_Ink,RTS_switch,Cheat_count;
#define RESULT ((volatile unsigned*)0x0203f000)
#define SRAM30 ((SlotReturnRecord*)0x0202ff00)
static unsigned checksum_video(void)
{
    unsigned i,h=2166136261u;
    for(i=0;i<240*160;i++)h=(h^((volatile uint16_t*)0x06000000)[i])*16777619u;
    return h;
}
static void picture(void)
{
    unsigned x,y;
    for(y=0;y<160;y++)for(x=0;x<240;x++)
        ((volatile uint16_t*)0x06000000)[y*240+x]=RGB5((x/8)%32,(y/6)%32,((x+y)/9)%32);
}
static void return_tests(void)
{
    SlotReturnRecord record;uint32_t cookie[4];unsigned i,checks=0,failed=0;
#define CHECK(x) do{checks++;if(!(x))failed++;}while(0)
    memset(&record,0,sizeof(record));record.magic=SLOT_RETURN_MAGIC;
    strcpy(record.filename,"Return test.gba");record.checksum=Slot_ReturnChecksum(&record);
    memset(cookie,0,sizeof(cookie));CHECK(!Slot_ReturnWarmValid(&record,cookie));
    memcpy(SRAM30,&record,sizeof(record));Slot_MarkReturn();
    memcpy(&record,SRAM30,sizeof(record));
    for(i=0;i<4;i++)cookie[i]=((volatile uint32_t*)SLOT_RETURN_COOKIE_ADDRESS)[i];
    CHECK(Slot_ReturnWarmValid(&record,cookie));CHECK(*(volatile unsigned*)0x0203f080==0);
    for(i=0;i<4;i++){uint32_t saved=cookie[i];cookie[i]^=1;CHECK(!Slot_ReturnWarmValid(&record,cookie));cookie[i]=saved;}
    /* A hard start clears EWRAM even if the battery-backed request survives. */
    memset(cookie,0,sizeof(cookie));CHECK(!Slot_ReturnWarmValid(&record,cookie));
    for(i=0;i<4;i++)cookie[i]=((volatile uint32_t*)SLOT_RETURN_COOKIE_ADDRESS)[i];
    for(i=0;i<sizeof(record);i++){
        ((unsigned char*)&record)[i]^=1;
        CHECK(!Slot_ReturnWarmValid(&record,cookie));
        ((unsigned char*)&record)[i]^=1;
    }
    record.magic=0;memcpy(SRAM30,&record,sizeof(record));
    for(i=0;i<4;i++)((volatile uint32_t*)SLOT_RETURN_COOKIE_ADDRESS)[i]=0;
    Slot_MarkReturn();CHECK(*(volatile uint32_t*)SLOT_RETURN_COOKIE_ADDRESS==0);
    RESULT[4]=checks;RESULT[5]=failed;
#undef CHECK
}
int main(void)
{
    irqInit();irqEnable(IRQ_VBLANK);SetMode(MODE_3|BG2_ENABLE);
    return_tests();RTS_switch=0;Cheat_count=0;
    Slot_RTS_Background=0;Slot_RTS_Panel=0x1cc6;Slot_RTS_Ink=0x7fff;
    picture();*(volatile uint16_t*)0x04000050=0x40;
    RESULT[0]=0x534d3131;RESULT[1]=checksum_video();RESULT[2]=0;
    while(1){
        VBlankIntrWait();scanKeys();
        if(RESULT[16]){
            unsigned command=RESULT[16];RESULT[16]=0;
            if(command==1){Slot_RTS_Background=0;Slot_RTS_Panel=0x1cc6;Slot_RTS_Ink=0x7fff;Cheat_count=0;}
            if(command==2){Slot_RTS_Background=0x7bff;Slot_RTS_Panel=0x7bde;Slot_RTS_Ink=0x10a5;Cheat_count=0;}
            if(command==3){Cheat_count=1;*(volatile unsigned*)0x03007fe0=0;}
            RESULT[17]=command;
        }
        if((keysDown()&(KEY_START|KEY_SELECT)) && (keysHeld()&(KEY_START|KEY_SELECT))==(KEY_START|KEY_SELECT)){
            RESULT[2]=1;ingamemenu_now();RESULT[2]=0;RESULT[3]=checksum_video();
            RESULT[6]=*(volatile uint16_t*)0x04000050;RESULT[7]=*(volatile unsigned*)0x03007fe0;
            RESULT[8]++;scanKeys();
        }
    }
}
