#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "slot_emulator_controls.h"
static unsigned checks;
__declspec(dllexport) unsigned run_tests(void)
{
    SlotEmulatorControls s;unsigned i,failed=0;
#define CHECK(x) do{checks++;if(!(x)){failed++;printf("Control failure at line %u\n",__LINE__);}}while(0)
    memset(&s,0,sizeof(s));
    for(i=0;i<179;i++)CHECK(!Slot_EmulatorKeys(&s,4,i,1));
    CHECK(Slot_EmulatorKeys(&s,4,179,1)==2);
    for(i=180;i<400;i++)CHECK(!Slot_EmulatorKeys(&s,4,i,1));
    CHECK(!Slot_EmulatorKeys(&s,0,400,1));
    CHECK(Slot_EmulatorKeys(&s,12,401,1)==1);
    for(i=402;i<650;i++)CHECK(!Slot_EmulatorKeys(&s,12,i,1));
    CHECK(!Slot_EmulatorKeys(&s,4,650,1)); /* release Start while Select held */
    CHECK(!Slot_EmulatorKeys(&s,4,900,1));
    Slot_EmulatorKeys(&s,0,901,1);
    for(i=0;i<500;i++)CHECK(!Slot_EmulatorKeys(&s,4,902+i,0));
    Slot_EmulatorKeys(&s,0,1402,1);
    /* Emulating many frames inside one physical VBlank cannot shorten a hold. */
    for(i=0;i<1000;i++)CHECK(!Slot_EmulatorKeys(&s,4,1500,1));
    CHECK(!Slot_EmulatorKeys(&s,0,1501,1));
    for(i=0;i<178;i++)CHECK(!Slot_EmulatorKeys(&s,4,1600+i,1));
    CHECK(Slot_EmulatorKeys(&s,12,1778,1)==1); /* menu wins over exit */
    Slot_EmulatorKeys(&s,0,1779,1);
    for(i=0;i<179;i++)CHECK(!Slot_EmulatorKeys(&s,4,0xfffffff0u+i,1));
    CHECK(Slot_EmulatorKeys(&s,4,0xfffffff0u+179,1)==2); /* clock wrap */
    Slot_EmulatorKeys(&s,0,4000,1);
    CHECK(!Slot_EmulatorKeys(&s,0x300,4001,1)); /* L+R is no menu */
    printf("Emulator controls: %u checks, %u failed\n",checks,failed);
    return failed;
}
__declspec(dllexport) unsigned check_count(void){return checks;}
