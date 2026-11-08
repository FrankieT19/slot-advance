/* slot. advance integration for the pinned Jagoomba Color v0.5 source. */
#include "includes.h"
#include "slot_return.h"
#include "slot_emulator_controls.h"
extern int main_ui_selection;
EWRAM_BSS volatile unsigned Slot_JagoombaVBlanks;
static EWRAM_BSS SlotEmulatorControls slot_keys;
static EWRAM_BSS unsigned slot_hold_enabled,slot_exit_requested;
#ifdef SLOT_EMULATOR_TEST
EWRAM_BSS SlotReturnRecord Slot_JagoombaTestRecord;
EWRAM_BSS unsigned Slot_JagoombaTestPage,Slot_JagoombaTestExit,Slot_JagoombaTestMenus;
#endif
static __attribute__((section(".ewram"),long_call,noinline)) void slot_sram_page(unsigned page)
{
#ifdef SLOT_EMULATOR_TEST
    Slot_JagoombaTestPage=page;
#else
    *(vu16*)0x09fe0000=0xd200;*(vu16*)0x08000000=0x1500;
    *(vu16*)0x08020000=0xd200;*(vu16*)0x08040000=0x1500;
    *(vu16*)0x09c00000=page;*(vu16*)0x09fc0000=0x1500;
#endif
}
static volatile unsigned char *slot_record_address(void)
{
#ifdef SLOT_EMULATOR_TEST
    return (volatile unsigned char*)&Slot_JagoombaTestRecord;
#else
    return (volatile unsigned char*)SLOT_RETURN_ADDRESS;
#endif
}
static unsigned slot_read_context(SlotReturnRecord *record)
{
    unsigned i;volatile unsigned char *source;
    slot_sram_page(SLOT_RETURN_PAGE);source=slot_record_address();
    for(i=0;i<sizeof(*record);i++)((unsigned char*)record)[i]=source[i];
    slot_sram_page(0);
    /* Validate the armed record with a local request; don't request an exit yet. */
    record->requested=SLOT_RETURN_REQUEST;record->requested_inverse=~SLOT_RETURN_REQUEST;
    return record->platform>0 && Slot_ReturnValid(record);
}
void Slot_JagoombaInit(void)
{
    SlotReturnRecord record;
    slot_hold_enabled=slot_read_context(&record)?!!(record.options&SLOT_RETURN_HOLD_ENABLED):1;
    memset(&slot_keys,0,sizeof(slot_keys));slot_exit_requested=0;
    Slot_JagoombaVBlanks=0;main_ui_selection=-1;
}
unsigned Slot_JagoombaInput(unsigned held)
{
    unsigned action=Slot_EmulatorKeys(&slot_keys,held,Slot_JagoombaVBlanks,slot_hold_enabled);
    if(action==2)slot_exit_requested=1;
    return action;
}
unsigned Slot_JagoombaTakeExit(void)
{
    unsigned requested=slot_exit_requested;slot_exit_requested=0;
#ifdef SLOT_EMULATOR_TEST
    Slot_JagoombaTestMenus++;
#endif
    return requested;
}
void Slot_JagoombaResume(void)
{
    memset(&slot_keys,0,sizeof(slot_keys));slot_keys.blocked=!!((~REG_P1)&12);
}
void Slot_JagoombaMarkReturn(void)
{
    uint32_t magic=0,checksum=0,platform=0;unsigned i;
    volatile uint32_t *cookie=(volatile uint32_t*)SLOT_RETURN_COOKIE_ADDRESS;
    volatile unsigned char *destination;
    slot_sram_page(SLOT_RETURN_PAGE);destination=slot_record_address();
    for(i=0;i<4;i++){
        magic|=(uint32_t)destination[i]<<(i*8);
        checksum|=(uint32_t)destination[4+i]<<(i*8);
        platform|=(uint32_t)destination[16+i]<<(i*8);
    }
    if(magic!=SLOT_RETURN_MAGIC || platform<1 || platform>2){slot_sram_page(0);return;}
    /* All emulator save work is finished before this reuses the RAM tail. */
    cookie[0]=SLOT_RETURN_COOKIE_MAGIC;cookie[1]=~SLOT_RETURN_COOKIE_MAGIC;
    cookie[2]=checksum;cookie[3]=~checksum;
    for(i=0;i<4;i++){
        destination[8+i]=(unsigned char)(SLOT_RETURN_REQUEST>>(i*8));
        destination[12+i]=(unsigned char)((~SLOT_RETURN_REQUEST)>>(i*8));
    }
    slot_sram_page(0);
}
#ifdef SLOT_EMULATOR_TEST
void Slot_JagoombaTestReset(void)
{
    Slot_JagoombaMarkReturn();Slot_JagoombaTestExit++;
    for(;;)waitframe();
}
#endif
