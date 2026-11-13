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
    for(i=0;i<179;i++)CHECK(!Slot_EmulatorKeys(&s,4,i,SLOT_DEFAULT_RETURN_KEY,SLOT_DEFAULT_MENU_KEYS));
    CHECK(Slot_EmulatorKeys(&s,4,179,SLOT_DEFAULT_RETURN_KEY,SLOT_DEFAULT_MENU_KEYS)==2);
    for(i=180;i<400;i++)CHECK(!Slot_EmulatorKeys(&s,4,i,SLOT_DEFAULT_RETURN_KEY,SLOT_DEFAULT_MENU_KEYS));
    CHECK(!Slot_EmulatorKeys(&s,0,400,SLOT_DEFAULT_RETURN_KEY,SLOT_DEFAULT_MENU_KEYS));
    CHECK(Slot_EmulatorKeys(&s,12,401,SLOT_DEFAULT_RETURN_KEY,SLOT_DEFAULT_MENU_KEYS)==1);
    for(i=402;i<650;i++)CHECK(!Slot_EmulatorKeys(&s,12,i,SLOT_DEFAULT_RETURN_KEY,SLOT_DEFAULT_MENU_KEYS));
    CHECK(!Slot_EmulatorKeys(&s,4,650,SLOT_DEFAULT_RETURN_KEY,SLOT_DEFAULT_MENU_KEYS)); /* release Start while Select held */
    CHECK(!Slot_EmulatorKeys(&s,4,900,SLOT_DEFAULT_RETURN_KEY,SLOT_DEFAULT_MENU_KEYS));
    Slot_EmulatorKeys(&s,0,901,SLOT_DEFAULT_RETURN_KEY,SLOT_DEFAULT_MENU_KEYS);
    for(i=0;i<500;i++)CHECK(!Slot_EmulatorKeys(&s,4,902+i,0,SLOT_DEFAULT_MENU_KEYS));
    Slot_EmulatorKeys(&s,0,1402,SLOT_DEFAULT_RETURN_KEY,SLOT_DEFAULT_MENU_KEYS);
    /* Emulating many frames inside one physical VBlank cannot shorten a hold. */
    for(i=0;i<1000;i++)CHECK(!Slot_EmulatorKeys(&s,4,1500,SLOT_DEFAULT_RETURN_KEY,SLOT_DEFAULT_MENU_KEYS));
    CHECK(!Slot_EmulatorKeys(&s,0,1501,SLOT_DEFAULT_RETURN_KEY,SLOT_DEFAULT_MENU_KEYS));
    for(i=0;i<178;i++)CHECK(!Slot_EmulatorKeys(&s,4,1600+i,SLOT_DEFAULT_RETURN_KEY,SLOT_DEFAULT_MENU_KEYS));
    CHECK(Slot_EmulatorKeys(&s,12,1778,SLOT_DEFAULT_RETURN_KEY,SLOT_DEFAULT_MENU_KEYS)==1); /* menu wins over exit */
    Slot_EmulatorKeys(&s,0,1779,SLOT_DEFAULT_RETURN_KEY,SLOT_DEFAULT_MENU_KEYS);
    for(i=0;i<179;i++)CHECK(!Slot_EmulatorKeys(&s,4,0xfffffff0u+i,SLOT_DEFAULT_RETURN_KEY,SLOT_DEFAULT_MENU_KEYS));
    CHECK(Slot_EmulatorKeys(&s,4,0xfffffff0u+179,SLOT_DEFAULT_RETURN_KEY,SLOT_DEFAULT_MENU_KEYS)==2); /* clock wrap */
    Slot_EmulatorKeys(&s,0,4000,SLOT_DEFAULT_RETURN_KEY,SLOT_DEFAULT_MENU_KEYS);
    CHECK(!Slot_EmulatorKeys(&s,0x300,4001,SLOT_DEFAULT_RETURN_KEY,SLOT_DEFAULT_MENU_KEYS)); /* L+R is no menu */
    memset(&s,0,sizeof(s));
    for(i=0;i<179;i++)CHECK(!Slot_EmulatorKeys(&s,512,i,512,768));
    CHECK(Slot_EmulatorKeys(&s,768,179,512,768)==1);
    CHECK(!Slot_EmulatorKeys(&s,512,400,512,768));
    CHECK(!Slot_EmulatorKeys(&s,0,401,512,768));
    CHECK(!Slot_EmulatorKeys(&s,12,402,512,768));
    CHECK(!Slot_EmulatorKeys(&s,512,403,512,768));
    CHECK(Slot_EmulatorKeys(&s,512,582,512,768)==2);
    printf("Emulator controls: %u checks, %u failed\n",checks,failed);
    return failed;
}
__declspec(dllexport) unsigned check_count(void){return checks;}
