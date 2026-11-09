/* Offline emulator fixture: actual injected menu/marker, with SRAM bank shims. */
#include <gba.h>
#include <stdint.h>
#include <string.h>
#include "slot_return.h"
extern void ingamemenu_now(void),Slot_MarkReturn(void),Slot_TestIrq(void);
extern unsigned RTS_Reset_key;
extern unsigned Slot_RTS_Background,Slot_RTS_Panel,Slot_RTS_Ink,RTS_switch,Cheat_count;
#define RESULT ((volatile unsigned*)0x0203f000)
#define SRAM30 ((SlotReturnRecord*)0x0202ff00)
static void arm_return(void)
{
    memset(SRAM30,0,sizeof(*SRAM30));
    SRAM30->magic=SLOT_RETURN_MAGIC;
    strcpy(SRAM30->filename,"Return test.gba");
    SRAM30->checksum=Slot_ReturnChecksum(SRAM30);
}
static void solid_frame(uint16_t color)
{
    unsigned i;
    for(i=0;i<240*160;i++)((volatile uint16_t*)0x06000000)[i]=color;
}
static void prepare_hold_return(void)
{
    static volatile uint16_t raster_blend;
    solid_frame(RGB5(12,20,10));
    REG_WIN0H=240;REG_WIN0V=160;REG_WININ=4;REG_WINOUT=4;
    SetMode(MODE_3|BG2_ENABLE|WIN0_ENABLE);
    REG_DMA0SAD=(uintptr_t)&raster_blend;
    REG_DMA0DAD=(uintptr_t)&REG_BLDY;
    REG_DMA0CNT=DMA_ENABLE|DMA_HBLANK|DMA_REPEAT|DMA_SRC_FIXED|DMA_DST_FIXED|1;
    RTS_Reset_key=0x3f3;
    REG_IME=0;
    *(volatile unsigned*)0x03007ff4=*(volatile unsigned*)0x03007ffc;
    *(volatile uintptr_t*)0x03007ffc=(uintptr_t)Slot_TestIrq;
    REG_IME=1;
}
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
    volatile uint32_t *cookie=(volatile uint32_t*)SLOT_RETURN_COOKIE_ADDRESS;
    irqInit();irqEnable(IRQ_VBLANK);SetMode(MODE_3|BG2_ENABLE);
    if(cookie[0]==SLOT_RETURN_COOKIE_MAGIC && cookie[1]==~SLOT_RETURN_COOKIE_MAGIC && cookie[2]==~cookie[3]){
        int level;
        cookie[0]=0;
        REG_BLDCNT=0xff;REG_BLDY=16;
        solid_frame(RGB5(8,10,18));
        for(level=16;level>=0;level-=2){VBlankIntrWait();REG_BLDY=level;}
        RESULT[9]=SLOT_RETURN_COOKIE_MAGIC;
        while(1)VBlankIntrWait();
    }
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
            if(command==4 || command==5)arm_return();
            if(command==4)prepare_hold_return();
            RESULT[17]=command;
        }
        if((keysDown()&(KEY_START|KEY_SELECT)) && (keysHeld()&(KEY_START|KEY_SELECT))==(KEY_START|KEY_SELECT)){
            RESULT[2]=1;ingamemenu_now();RESULT[2]=0;RESULT[3]=checksum_video();
            RESULT[6]=*(volatile uint16_t*)0x04000050;RESULT[7]=*(volatile unsigned*)0x03007fe0;
            RESULT[8]++;scanKeys();
        }
    }
}
