#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char u8;
#ifndef _Static_assert
#define _Static_assert(condition,message) typedef char ReturnRecordSizeCheck[(condition)?1:-1]
#endif
static uint32_t cookie[4];
static unsigned char storage[160];
static unsigned page,invalid_io;
#define SLOT_RETURN_IO_TEST 1
#define SLOT_RETURN_MAILBOX cookie
static void SetRampage(unsigned p){page=p;}
static void ReadSram(unsigned address,u8 *data,unsigned size)
{
    if(page!=0x30 || address!=0x0e00ff00 || size>128){invalid_io++;return;}
    memcpy(data,storage+16,size);
}
static void WriteSram(unsigned address,u8 *data,unsigned size)
{
    if(page!=0x30 || address!=0x0e00ff00 || size>128){invalid_io++;return;}
    memcpy(storage+16,data,size);
}
#include "slot_return_io.h"
static void request(void)
{
    SlotReturnRecord record;memcpy(&record,storage+16,sizeof(record));
    record.requested=SLOT_RETURN_REQUEST;record.requested_inverse=~SLOT_RETURN_REQUEST;
    cookie[0]=SLOT_RETURN_COOKIE_MAGIC;cookie[1]=~SLOT_RETURN_COOKIE_MAGIC;
    cookie[2]=record.checksum;cookie[3]=~record.checksum;
    memcpy(storage+16,&record,sizeof(record));
}
__declspec(dllexport) unsigned run_tests(void)
{
    unsigned checks=0,failed=0,i;SlotReturnRecord record;
#define CHECK(c) do{checks++;if(!(c))failed++;}while(0)
    memset(storage,0xcc,sizeof(storage));memset(cookie,0,sizeof(cookie));
    Slot_ArmReturn("Game.gba",0,1);CHECK(page==0);memcpy(&record,storage+16,sizeof(record));
    CHECK(record.magic==SLOT_RETURN_MAGIC && !record.requested);
    Slot_ReadReturn();CHECK(!slot_return_pending && page==0);
    Slot_ArmReturn("Game.gba",0,1);request();Slot_ReadReturn();
    CHECK(slot_return_pending && !strcmp(slot_return_filename,"Game.gba"));
    for(i=0;i<4;i++)CHECK(cookie[i]==0);
    for(i=0;i<16;i++)CHECK(storage[16+i]==0);
    Slot_ReadReturn();CHECK(!slot_return_pending); /* consumed, never replayed */
    Slot_ArmReturn("Game.gba",0,1);request();memset(cookie,0,sizeof(cookie));
    Slot_ReadReturn();CHECK(!slot_return_pending); /* hard power-on, SRAM retained */
    Slot_ArmReturn("Game.gba",0,1);request();cookie[2]^=1;
    Slot_ReadReturn();CHECK(!slot_return_pending);
    Slot_ArmReturn("Game.gba",0,1);request();storage[16+25]^=1;
    Slot_ReadReturn();CHECK(!slot_return_pending);
    Slot_ArmReturn("../Other.gba",0,1);request();Slot_ReadReturn();CHECK(!slot_return_pending);
    Slot_ArmReturn("Game.gba",3,0);request();Slot_ReadReturn();CHECK(!slot_return_pending);
    for(i=1;i<3;i++){
        Slot_ArmReturn(i==1?"Game.gb":"Game.gbc",i,1);request();Slot_ReadReturn();
        CHECK(slot_return_pending && slot_return_platform==i);
        Slot_ReadReturn();CHECK(!slot_return_pending);
        Slot_ArmReturn("Game.gb",i,0);request();memset(cookie,0,sizeof(cookie));
        Slot_ReadReturn();CHECK(!slot_return_pending);
    }
    Slot_ArmReturn("Game.gb",1,2);request();Slot_ReadReturn();CHECK(!slot_return_pending);
    Slot_ArmReturn("Game.gb",1,1);request();storage[16+120]^=1;
    Slot_ReadReturn();CHECK(!slot_return_pending);
    CHECK(!invalid_io && page==0);
    for(i=0;i<16;i++){CHECK(storage[i]==0xcc);CHECK(storage[144+i]==0xcc);}
    printf("Return IO: %u checks, %u failed\n",checks,failed);
    return failed;
#undef CHECK
}
