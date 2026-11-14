#ifndef SLOT_CARTRIDGE_SETTINGS_H
#define SLOT_CARTRIDGE_SETTINGS_H

static void Slot_ClockMenu(void)
{
    static const char *const labels[]={"Year","Month","Day","Hour","Minute","Second"};
    static const unsigned char fields[]={SLOT_YEAR,SLOT_MONTH,SLOT_DAY,SLOT_HOUR,SLOT_MINUTE,SLOT_SECOND};
    unsigned char datetime[7];
    unsigned selected=0,dirty=1,armed=0,refresh=slot_vblanks;
    Slot_ReadDateTime(datetime);
    while(1){
        unsigned down,repeat;
        if(dirty){
            char text[6][5];
            const char *values[]={text[0],text[1],text[2],text[3],text[4],text[5]};
            unsigned i;
            snprintf(text[0],sizeof(text[0]),"%04u",2000+datetime[SLOT_YEAR]);
            for(i=1;i<6;i++)snprintf(text[i],sizeof(text[i]),"%02u",datetime[fields[i]]);
            Slot_SettingsPage("Set date and time",labels,values,6,selected);
            Slot_Present();dirty=0;
        }
        VBlankIntrWait();UIAudio_Update();scanKeys();
        if(!armed){armed=!keysHeld();continue;}
        down=keysDown();repeat=keysDownRepeat()|down;
        if(down&KEY_B)return;
        if(repeat&KEY_UP){selected=(selected+5)%6;dirty=1;}
        if(repeat&KEY_DOWN){selected=(selected+1)%6;dirty=1;}
        if(down&(KEY_A|KEY_LEFT|KEY_RIGHT)){
            /* Take the current RTC value so editing a field keeps the live time. */
            Slot_ReadDateTime(datetime);
            Slot_AdjustDateTime(datetime,fields[selected],down&KEY_LEFT?-1:1);
            datetime[SLOT_WEEKDAY]=Slot_DateWeekday(datetime);
            rtc_enable();rtc_set(datetime);rtc_disenable();
            delay(0x200);
            Slot_RefreshClock();
            refresh=slot_vblanks;dirty=1;
        }else if(slot_vblanks-refresh>=60){
            Slot_ReadDateTime(datetime);Slot_RefreshClock();
            refresh=slot_vblanks;dirty=1;
        }
    }
}

typedef struct {
    const char *label;
    u16 *value;
    unsigned address;
    unsigned mask;
    unsigned count;
    const char *const *choices;
} SlotCartridgeSetting;

static const char *const slot_off_on[]={"Off","On"};
static const char *const slot_patch_engines[]={"Fast","Manual"};
#ifdef SLOT_OMEGA_DE
static const char *const slot_mode_b[]={"Rumble","RAM","Link"};
static const char *const slot_led_colours[]={"Off","Blue","Green","Cyan","Red","Magenta","Yellow","White"};
#endif

static unsigned Slot_SaveCartridgeSettings(const SlotCartridgeSetting *settings,const unsigned *values,unsigned count)
{
    unsigned i,changed=0;
    for(i=0;i<count;i++)if(*settings[i].value!=values[i])changed=1;
    if(!changed)return 1;
    /* Preserve fields from the stock kernel, including save type and Mode B's
       packed boot preference. Save_SET_info rewrites the settings sector. */
    for(i=0;i<0x200/sizeof(*SET_info_buffer);i++)SET_info_buffer[i]=Read_SET_info(i);
    for(i=0;i<count;i++){
        unsigned address=settings[i].address,mask=settings[i].mask;
        SET_info_buffer[address]=(SET_info_buffer[address]&~mask)|values[i];
    }
    Launcher_PrepareSettingsFlashWrite();
    Save_SET_info(SET_info_buffer,0x200);
    for(i=0;i<count;i++)if(Read_SET_info(settings[i].address)!=SET_info_buffer[settings[i].address]){
        unsigned armed=0;
        Slot_Message("Could not save settings","B: return");
        do{VBlankIntrWait();UIAudio_Update();scanKeys();}
        while(!Slot_DialogCanClose(&armed,keysHeld(),keysDown()));
        return 0;
    }
    for(i=0;i<count;i++)*settings[i].value=values[i];
    return 1;
}

#ifdef SLOT_OMEGA_DE
static unsigned Slot_LedColours(u16 red,u16 green,u16 blue)
{return (red<<2)|(green<<1)|blue;}

static void Slot_EditLedSettings(void)
{
    static const char *const labels[]={"LED","Breathing colour","SD activity colour"};
    static const SlotCartridgeSetting channels[]={
        {NULL,&gl_led_open_sel,assress_led_open_sel,0xffff,0,NULL},
        {NULL,&gl_Breathing_R,assress_Breathing_R,0xffff,0,NULL},
        {NULL,&gl_Breathing_G,assress_Breathing_G,0xffff,0,NULL},
        {NULL,&gl_Breathing_B,assress_Breathing_B,0xffff,0,NULL},
        {NULL,&gl_SD_R,assress_SD_R,0xffff,0,NULL},
        {NULL,&gl_SD_G,assress_SD_G,0xffff,0,NULL},
        {NULL,&gl_SD_B,assress_SD_B,0xffff,0,NULL},
    };
    unsigned selected=0,dirty=1,armed=0;
    while(1){
        unsigned down,repeat;
        unsigned breathing=Slot_LedColours(gl_Breathing_R,gl_Breathing_G,gl_Breathing_B);
        unsigned activity=Slot_LedColours(gl_SD_R,gl_SD_G,gl_SD_B);
        if(dirty){
            const char *names[]={slot_off_on[gl_led_open_sel],slot_led_colours[breathing],slot_led_colours[activity]};
            Slot_SettingsPage("LED settings",labels,names,3,selected);
            Slot_Present();dirty=0;
        }
        VBlankIntrWait();UIAudio_Update();scanKeys();
        if(!armed){armed=!keysHeld();continue;}
        down=keysDown();
        if(down&KEY_B)return;
        repeat=keysDownRepeat()|down;
        if(repeat&KEY_UP){selected=(selected+2)%3;dirty=1;}
        if(repeat&KEY_DOWN){selected=(selected+1)%3;dirty=1;}
        if(down&(KEY_A|KEY_LEFT|KEY_RIGHT)){
            unsigned saved;
            if(!selected){
                unsigned value=gl_led_open_sel^1;
                saved=Slot_SaveCartridgeSettings(channels,&value,1);
            }else{
                unsigned colour=selected==1?breathing:activity;
                unsigned values[3];
                colour=(colour+(down&KEY_LEFT?7:1))%8;
                values[0]=colour>>2;values[1]=(colour>>1)&1;values[2]=colour&1;
                saved=Slot_SaveCartridgeSettings(channels+1+(selected-1)*3,values,3);
            }
            if(saved)Set_LED_control((gl_led_open_sel<<7)|
                     (Slot_LedColours(gl_Breathing_R,gl_Breathing_G,gl_Breathing_B)<<3)|
                      Slot_LedColours(gl_SD_R,gl_SD_G,gl_SD_B));
            dirty=1;
        }
    }
}
#endif

static void Slot_CartridgeSettings(void)
{
    static const SlotCartridgeSetting settings[]={
        {"In-game RTC",&gl_ingame_RTC_open_status,assress_ingame_RTC_open_status,0xffff,2,slot_off_on},
        {"Back up saves",&gl_toggle_backup,assress_toggle_backup,0xffff,2,slot_off_on},
        {"GBA boot intro",&gl_toggle_reset,assress_toggle_reset,0xffff,2,slot_off_on},
        {"Patch engine",&gl_engine_sel,assress_engine_sel,0xffff,2,slot_patch_engines},
#ifdef SLOT_OMEGA_DE
        {"Mode B",&gl_ModeB_init,assress_ModeB_INIT,0xff,3,slot_mode_b},
        {"LED settings",NULL,0,0,0,NULL},
#endif
    };
    enum { count=sizeof(settings)/sizeof(*settings) };
    unsigned selected=0,dirty=1,armed=0,i;
    const char *labels[count];
    for(i=0;i<count;i++)labels[i]=settings[i].label;
    while(1){
        unsigned down,repeat;
        if(dirty){
            const char *text[count];
            for(i=0;i<count;i++)text[i]=settings[i].count?settings[i].choices[*settings[i].value]:">";
            Slot_SettingsPage("Cartridge settings",labels,text,count,selected);
            Slot_Present();dirty=0;
        }
        VBlankIntrWait();UIAudio_Update();scanKeys();
        if(!armed){armed=!keysHeld();continue;}
        down=keysDown();repeat=keysDownRepeat()|down;
        if(down&KEY_B)return;
        if(repeat&KEY_UP){selected=(selected+count-1)%count;dirty=1;}
        if(repeat&KEY_DOWN){selected=(selected+1)%count;dirty=1;}
        if(down&(KEY_A|KEY_LEFT|KEY_RIGHT)){
            const SlotCartridgeSetting *setting=&settings[selected];
            if(setting->count){
                unsigned step=down&KEY_LEFT?setting->count-1:1;
                unsigned value=(*setting->value+step)%setting->count;
                Slot_SaveCartridgeSettings(setting,&value,1);
            }
#ifdef SLOT_OMEGA_DE
            else if(down&KEY_A)Slot_EditLedSettings();
#endif
            dirty=1;
        }
    }
}
#endif
