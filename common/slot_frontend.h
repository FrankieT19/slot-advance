/* slot. advance: cartridge shelves on the DS Style backend. GPL-3.0-or-later. */
#ifndef SLOT_FRONTEND_H
#define SLOT_FRONTEND_H
#include "slot_render.h"
#include "slot_format.h"
#include "slot_fast_cache.h"
#include "slot_timing.h"
#include "slot_transition.h"
#include "slot_return_io.h"
#define SLOT_DIR "/SYSTEM/SLOT"
#define SLOT_CONFIG SLOT_DIR "/SETTINGS.TXT"
#define SLOT_CONFIG_TMP SLOT_DIR "/SETTINGS.TMP"
#define SLOT_CONFIG_BAK SLOT_DIR "/SETTINGS.BAK"
/* Page-zero PSRAM: 5 x 24 KiB images, ending before DS Style's 64 KiB file table. */
#define SLOT_CACHE_BASE 0x08FD0000
#define SLOT_CACHE_STRIDE 0x6000
#define SLOT_STAGE (pReadCache+240*160*2)
#define SLOT_CACHE_PIXELS 12288
#define SLOT_CACHE_SLOTS 5
_Static_assert(SLOT_CACHE_BASE+SLOT_CACHE_STRIDE*SLOT_CACHE_SLOTS <= LAUNCHER_FILE_LIST_PSRAM_ADDRESS,"PSRAM overlap");
_Static_assert(240*160*2+SLOT_CACHE_STRIDE<=MAX_pReadCache_size,"EWRAM scratch overflow");
_Static_assert(104*112<=SLOT_CACHE_PIXELS,"Cartridge too big");
typedef struct { int index; unsigned valid, missing; } SlotCache;
static SlotCache slot_cache[SLOT_CACHE_SLOTS];
/* Scrolling streams at most 24 BMP rows between frames. The job owns its FIL
   and buffer, so drawing never overwrites a half-loaded image. */
#define SLOT_ART_ROWS 24
static FIL slot_art_file EWRAM_BSS;
static uint16_t slot_art_pixels[120*SLOT_ART_ROWS] EWRAM_BSS __attribute__((aligned(4)));
static unsigned slot_art_active,slot_art_bank,slot_art_row,slot_art_changed;
static void Slot_CancelArt(void)
{if(slot_art_active){f_close(&slot_art_file);slot_art_active=0;}}
static unsigned slot_index[3],slot_counts[3];
static unsigned slot_animations=1,slot_wrap=1,slot_remember=1,slot_addon=1;
static unsigned slot_last_platform=0,slot_config_dirty=0,slot_warning=0;
static unsigned slot_skipped=0,slot_scan_error=0;
static char slot_library[3][MAX_path_len] EWRAM_BSS;
static char slot_title[100] EWRAM_BSS;
static char slot_path[300] EWRAM_BSS;
static char slot_line[256] EWRAM_BSS;
static unsigned slot_scan_missing=0;
static unsigned slot_count;

static void Slot_ReadUsername(void)
{
    unsigned char data[64];UINT got;FRESULT result;
    slot_username[0]=0;
    if(f_open(&gfile,"/USERNAME.TXT",FA_READ)!=FR_OK)return;
    result=f_read(&gfile,data,sizeof(data),&got);f_close(&gfile);
    if(result==FR_OK)Slot_ParseUsername(data,got,slot_username);
}

static void Slot_Present(void)
{
    VBlankIntrWait();
    dmaCopy(slot_frame+slot_redraw_top*240,VideoBuffer+slot_redraw_top*240,(slot_redraw_bottom-slot_redraw_top)*240*2);
}
static void Slot_ResetCache(void)
{
    Slot_CancelArt();slot_art_changed=0;
    Slot_InvalidateFast();
    unsigned i; for(i=0;i<SLOT_CACHE_SLOTS;i++) { slot_cache[i].valid=0; slot_cache[i].index=-1; }
}
static int Slot_Normalize(int index)
{
    return Slot_IndexForCount(index,slot_count,slot_wrap);
}
static void Slot_Stem(unsigned index,char *out,unsigned size)
{
    char *dot;
    snprintf(out,size,"%s",pFilename_buffer[index].filename);
    dot=strrchr(out,'.'); if(dot) *dot=0;
}
static void Slot_ReadTitle(void)
{
    slot_title[0]=0;
    if(slot_count) Slot_Stem(slot_index[slot_platform],slot_title,sizeof(slot_title));
}
static unsigned Slot_LoadBMP(const char *path,uint16_t *destination)
{
    unsigned char header[54]; UINT got; unsigned w,h,y; FRESULT result;
    result=f_open(&gfile,path,FA_READ);
    if(result!=FR_OK) return 0;
    result=f_read(&gfile,header,sizeof(header),&got);
    if(result!=FR_OK || got!=sizeof(header) ||
       !Slot_BMPValid(header,f_size(&gfile),slot_widths[slot_platform],slot_heights[slot_platform])) {
        f_close(&gfile); return 0;
    }
    w=Slot_Le32(header+18); h=Slot_Le32(header+22);
    /* The read stage aliases only drawing bank zero. Preserve the other
       neighbours instead of reloading every sprite whenever the shelf moves. */
    slot_fast_source[0]=0;Slot_InvalidateFastSource(destination);
    result=f_read(&gfile,SLOT_STAGE,w*h*2,&got);
    if(result!=FR_OK || got!=w*h*2){f_close(&gfile);return 0;}
    f_close(&gfile);
    Slot_DecodeBMP555((uint16_t*)SLOT_STAGE,w*h);
    for(y=0;y<h;y++) {
        uint16_t *row=(uint16_t*)SLOT_STAGE+y*w;
        dmaCopy(row,destination+(h-1-y)*w,w*2);
    }
    return 1;
}
static SlotCache *Slot_GetCache(int index)
{
    unsigned i; index=Slot_Normalize(index);
    if(index<0) return 0;
    for(i=0;i<SLOT_CACHE_SLOTS;i++) if(slot_cache[i].valid && slot_cache[i].index==index) return &slot_cache[i];
    return 0;
}
static const uint16_t *Slot_CachedPixels(SlotCache *cache)
{
    unsigned i=(unsigned)(cache-slot_cache);
    return (const uint16_t*)(SLOT_CACHE_BASE+i*SLOT_CACHE_STRIDE);
}
static void Slot_LoadFallback(uint16_t *pixels,const char *name)
{
    unsigned w=slot_widths[slot_platform],h=slot_heights[slot_platform],row,col,n;
    unsigned panel_x=slot_platform==0?16:15,panel_y=slot_platform==0?17:slot_platform==1?32:37;
    unsigned panel_w=slot_platform==0?88:74,panel_h=slot_platform==0?45:66;
    unsigned cells=(panel_w-8)/6,len=(unsigned)strlen(name),lines=(len+cells-1)/cells;
    if(lines>(panel_h-4)/12)lines=(panel_h-4)/12;
    Slot_InvalidateFastSource(pixels);dmaCopy(Slot_Fallback(slot_platform),pixels,w*h*2);
    for(n=0;n<lines;n++) {
        unsigned count=len-n*cells;if(count>cells)count=cells;
        unsigned x=panel_x+(panel_w-count*6)/2,y=panel_y+(panel_h-lines*12)/2+n*12,i;
        for(i=0;i<count;i++) {
            unsigned c=(unsigned char)name[n*cells+i];if(c<32 || c>126)c='?';
            for(row=0;row<12;row++)for(col=0;col<6;col++)
                if(slot_font[c][row]&(128>>col))pixels[(y+row)*w+x+i*6+col]=SLOT_RGB(40,40,47);
        }
    }
}
static void Slot_FillCacheAround(int centre)
{
    int wanted[5],d; unsigned i,j;
    /* Keep neighbours during a move. All SD reads finish before drawing starts. */
    for(d=-2;d<=2;d++) wanted[d+2]=Slot_Normalize(centre+d);
    for(i=0;i<SLOT_CACHE_SLOTS;i++) {
        unsigned keep=0;
        for(j=0;j<5;j++) if(slot_cache[i].index==wanted[j]) keep=1;
        if(!keep) slot_cache[i].valid=0;
    }
    for(j=0;j<5;j++) {
        int index=wanted[j];
        if(index<0 || Slot_GetCache(index)) continue;
        for(i=0;i<SLOT_CACHE_SLOTS;i++) if(!slot_cache[i].valid) break;
        if(i==SLOT_CACHE_SLOTS) break;
        Slot_Stem(index,slot_line,sizeof(slot_line));
        snprintf(slot_path,sizeof(slot_path),SLOT_DIR "/%s/%s/%s.bmp",slot_skeuo?"CARTS":"CARTS_FLAT",slot_platform_names[slot_platform],slot_line);
        slot_cache[i].missing=!Slot_LoadBMP(slot_path,(uint16_t*)(SLOT_CACHE_BASE+i*SLOT_CACHE_STRIDE));
        if(slot_cache[i].missing)Slot_LoadFallback((uint16_t*)(SLOT_CACHE_BASE+i*SLOT_CACHE_STRIDE),slot_line);
        slot_cache[i].index=index; slot_cache[i].valid=1;
    }
}
static void Slot_FillCache(void){Slot_FillCacheAround((int)slot_index[slot_platform]);}
static void Slot_FinishArt(unsigned missing)
{
    SlotCache *cache=&slot_cache[slot_art_bank];
    Slot_CancelArt();
    if(missing){Slot_Stem(cache->index,slot_line,sizeof(slot_line));Slot_LoadFallback((uint16_t*)Slot_CachedPixels(cache),slot_line);}
    Slot_InvalidateFastSource(Slot_CachedPixels(cache));
    cache->valid=1;cache->missing=missing;slot_art_changed=1;
}
static void Slot_RequestArtAround(int centre)
{
    static const int order[5]={0,1,-1,2,-2};
    int wanted[5];unsigned i,j,bank;
    for(j=0;j<5;j++)wanted[j]=Slot_Normalize(centre+order[j]);
    if(slot_art_active) {
        for(j=0;j<5;j++)if(slot_cache[slot_art_bank].index==wanted[j])return;
        Slot_CancelArt(); /* Fast manual taps can leave a pending cart behind. */
    }
    for(j=0;j<5;j++)if(wanted[j]>=0 && !Slot_GetCache(wanted[j])) {
        unsigned char header[54];UINT got;
        for(bank=0;bank<SLOT_CACHE_SLOTS;bank++) {
            unsigned keep=0;
            for(i=0;i<5;i++)if(slot_cache[bank].valid && slot_cache[bank].index==wanted[i])keep=1;
            if(!keep)break;
        }
        if(bank==SLOT_CACHE_SLOTS)return;
        slot_art_bank=bank;slot_art_row=0;slot_cache[bank].valid=0;slot_cache[bank].index=wanted[j];
        Slot_InvalidateFastSource(Slot_CachedPixels(&slot_cache[bank]));
        Slot_Stem(wanted[j],slot_line,sizeof(slot_line));
        snprintf(slot_path,sizeof(slot_path),SLOT_DIR "/%s/%s/%s.bmp",slot_skeuo?"CARTS":"CARTS_FLAT",slot_platform_names[slot_platform],slot_line);
        if(f_open(&slot_art_file,slot_path,FA_READ)!=FR_OK){Slot_FinishArt(1);return;}
        slot_art_active=1;
        if(f_read(&slot_art_file,header,sizeof(header),&got)!=FR_OK || got!=sizeof(header) ||
           !Slot_BMPValid(header,f_size(&slot_art_file),slot_widths[slot_platform],slot_heights[slot_platform]))Slot_FinishArt(1);
        return;
    }
}
static void Slot_PumpArt(void)
{
    unsigned w,h,rows,y;UINT got;
    if(!slot_art_active)return;
    w=slot_widths[slot_platform];h=slot_heights[slot_platform];rows=h-slot_art_row;
    if(rows>SLOT_ART_ROWS)rows=SLOT_ART_ROWS;
    if(f_read(&slot_art_file,slot_art_pixels,w*rows*2,&got)!=FR_OK || got!=w*rows*2){Slot_FinishArt(1);return;}
    Slot_DecodeBMP555(slot_art_pixels,w*rows);
    for(y=0;y<rows;y++)dmaCopy(slot_art_pixels+y*w,(uint16_t*)Slot_CachedPixels(&slot_cache[slot_art_bank])+(h-1-slot_art_row-y)*w,w*2);
    slot_art_row+=rows;
    if(slot_art_row==h)Slot_FinishArt(0);
}
static void Slot_DrawShelf(int centre_index,int progress,int drop,unsigned ticks)
{
    int d,pass;
    Slot_BeginShelf(slot_partial_motion && !drop,slot_partial_motion?(slot_platform==0?44:13):0,slot_partial_motion?(drop?160:(slot_platform==0?122:131)):160);
    for(pass=0;pass<2;pass++) {
    if(pass)Slot_BeginShelfPaint();
    Slot_BeginFastFrame();
    for(d=-2;d<=2;d++) {
        if(slot_count==1 && d!=0) continue;
        int relative=d*256-progress;
        int scale=Slot_CarouselScale(relative);
        int centre=Slot_CarouselCentre(slot_platform,relative);
        SlotCache *cache=Slot_GetCache(centre_index+d);
        if(Slot_Normalize(centre_index+d)>=0 && (drop==0 || d==0)) {
            int half=Slot_ScaledExtent(slot_widths[slot_platform],scale)/2;
            if(centre+half>0 && centre-half<240)Slot_CartQ8(Slot_FastPixels(cache?Slot_CachedPixels(cache):Slot_Fallback(slot_platform),slot_platform),slot_platform,centre,scale,d==0?drop:0);
        }
    }
    }
    Slot_EndShelf();
    if(!slot_partial_motion || drop)Slot_Chrome(slot_count,slot_index[slot_platform],drop?0:slot_title,ticks,
                slot_scan_error?"SD read error: restart to retry":slot_warning?"512 game limit: more files exist":
                slot_skipped?"Skipped long names: see guide":0);

}
static void Slot_ConfigSet(const char *name,unsigned value)
{
    if(!strcmp(name,"Skeuomorphic") && value<2)slot_skeuo=value;
    else if(!strcmp(name,"Theme") && value<3) slot_theme=value;
    else if(!strcmp(name,"Animations") && value<2) slot_animations=value;
    else if(!strcmp(name,"Wrap") && value<2) slot_wrap=value;
    else if(!strcmp(name,"Sounds") && value<2) launcher_sounds_enabled=value;
    else if(!strcmp(name,"Remember") && value<2) slot_remember=value;
    else if(!strcmp(name,"Addons") && value<2) slot_addon=value;
    else if(!strcmp(name,"Reset") && value<2) gl_reset_on=value;
    else if(!strcmp(name,"States") && value<2) gl_rts_on=value;
    else if(!strcmp(name,"Sleep") && value<2) gl_sleep_on=value;
    else if(!strcmp(name,"Platform") && value<3) slot_last_platform=value;
    else if(!strcmp(name,"GBA") && value<MAX_files) slot_index[0]=value;
    else if(!strcmp(name,"GB") && value<MAX_files) slot_index[1]=value;
    else if(!strcmp(name,"GBC") && value<MAX_files) slot_index[2]=value;
}
static const char *const slot_config_keys[]={"Theme","Animations","Wrap","Sounds","Remember","Addons","Reset","States","Sleep","Platform","GBA","GB","GBC","Skeuomorphic"};
static int Slot_ConfigKey(const char *name)
{
    unsigned i; for(i=0;i<14;i++) if(!strcmp(name,slot_config_keys[i])) return (int)i;
    return -1;
}
static unsigned Slot_ReadConfigFile(const char *path)
{
    unsigned seen=0,ended=0,valid=1,value,pending[14],i,legacy,version;
    if(f_open(&gfile,path,FA_READ)!=FR_OK) return 0;
    if(!f_gets(slot_line,sizeof(slot_line),&gfile)) { f_close(&gfile); return 0; }
    slot_line[strcspn(slot_line,"\r\n")]=0;
    legacy=!strcmp(slot_line,"SlotOmega=1");
    version=legacy?1:!strcmp(slot_line,"SlotOmega=2")?2:!strcmp(slot_line,"SlotOmega=3")?3:0;
    if(!version) { f_close(&gfile); return 0; }
    /* Validate a complete snapshot before changing live settings. A truncated
       primary file cannot poison values restored from the previous snapshot. */
    while(f_gets(slot_line,sizeof(slot_line),&gfile)) {
        char *equals=strchr(slot_line,'='); int k;
        if(!equals || slot_line[0]=='#' || slot_line[0]==';') continue;
        *equals++=0; k=Slot_ConfigKey(slot_line);
        if(k>=0) {
            unsigned limit=k==0 || k==9?3:k>=10 && k<=12?MAX_files:2;
            if(!Slot_ParseNatural(equals,&value) || value>=limit) { valid=0; break; }
            seen|=1u<<k;pending[k]=value;
        } else if(!strcmp(slot_line,"End")) {
            if(!Slot_ParseNatural(equals,&value) || value!=1) valid=0;
            ended=1;
        }
    }
    if(f_error(&gfile) || !ended || seen!=(version<3?8191:16383)) valid=0;
    if(f_close(&gfile)!=FR_OK) valid=0;
    if(valid) {
        for(i=0;i<(version<3?13:14);i++)Slot_ConfigSet(slot_config_keys[i],pending[i]);
        if(version<3){slot_skeuo=1;slot_config_dirty=1;}
        if(legacy){slot_addon=1;gl_reset_on=1;slot_config_dirty=1;}
    }
    return valid;
}
static void Slot_ReadConfig(void)
{
    if(!Slot_ReadConfigFile(SLOT_CONFIG)) Slot_ReadConfigFile(SLOT_CONFIG_BAK);
    slot_platform=slot_remember?slot_last_platform:0;
    if(!slot_remember) memset(slot_index,0,sizeof(slot_index));
}
static unsigned Slot_SaveConfig(void)
{
    unsigned length; UINT written; FRESULT result;
    if(!slot_config_dirty) return 1;
    f_mkdir("/SYSTEM"); f_mkdir(SLOT_DIR);
    length=snprintf(slot_line,sizeof(slot_line),
        "SlotOmega=3\nTheme=%u\nAnimations=%u\nWrap=%u\nSounds=%u\nRemember=%u\nAddons=%u\nReset=%u\nStates=%u\nSleep=%u\nPlatform=%u\nGBA=%u\nGB=%u\nGBC=%u\nSkeuomorphic=%u\nEnd=1\n",
        slot_theme,slot_animations,slot_wrap,launcher_sounds_enabled,slot_remember,slot_addon,
        gl_reset_on,gl_rts_on,gl_sleep_on,slot_platform,slot_index[0],slot_index[1],slot_index[2],slot_skeuo);
    if(length>=sizeof(slot_line) || f_open(&gfile,SLOT_CONFIG_TMP,FA_CREATE_ALWAYS|FA_WRITE)!=FR_OK) return 0;
    result=f_write(&gfile,slot_line,length,&written);
    if(f_close(&gfile)!=FR_OK || result!=FR_OK || written!=length) return 0;
    f_unlink(SLOT_CONFIG_BAK);
    result=f_rename(SLOT_CONFIG,SLOT_CONFIG_BAK);
    if(result!=FR_OK && result!=FR_NO_FILE) return 0;
    if(f_rename(SLOT_CONFIG_TMP,SLOT_CONFIG)!=FR_OK) { f_rename(SLOT_CONFIG_BAK,SLOT_CONFIG); return 0; }
    slot_config_dirty=0; return 1;
}
static void Slot_Message(const char *title,const char *line)
{
    Slot_Background(); Slot_Centred(title,62,Slot_Ink()); Slot_Centred(line,82,Slot_Ink()); Slot_Present();
}
static void Slot_SelectDirectory(void)
{
    const char *patterns[3]={"/Games/%s","/ROMS/%s","/%s"}; unsigned n;
    for(n=0;n<3;n++) {
        snprintf(slot_library[slot_platform],MAX_path_len,patterns[n],slot_platform_names[slot_platform]);
        if(f_opendir(&dir,slot_library[slot_platform])==FR_OK) { f_closedir(&dir); return; }
    }
    snprintf(slot_library[slot_platform],MAX_path_len,"/Games/%s",slot_platform_names[slot_platform]);
}
static void Slot_Scan(void)
{
    FRESULT result; const char *ext; unsigned ok;
    SetPSRampage(0); Slot_ResetCache(); Slot_SelectDirectory();
    slot_count=0; slot_warning=0; slot_skipped=0; slot_scan_error=0;
    Slot_BlankShelf(); Slot_Present();
    result=f_opendir(&dir,slot_library[slot_platform]);
    slot_scan_missing=result==FR_NO_PATH || result==FR_NO_FILE;
    if(result!=FR_OK) { slot_scan_error=!slot_scan_missing; goto done; }
    while(1) {
        result=f_readdir(&dir,&fileinfo);
        if(result!=FR_OK || !fileinfo.fname[0]) break;
        if(fileinfo.fattrib&(AM_DIR|AM_HID|AM_SYS) || fileinfo.fname[0]=='.') continue;
        ext=strrchr(fileinfo.fname,'.'); if(!ext) continue;
        ok=slot_platform==0 ? !strcasecmp(ext,".gba") || !strcasecmp(ext,".agb") :
           !strcasecmp(ext,slot_platform==1?".gb":".gbc");
        if(!ok) continue;
        if(strlen(fileinfo.fname)>=sizeof(pFilename_temp.filename)) { slot_skipped++; continue; }
        if(slot_count>=MAX_files) { slot_warning=1; continue; }
        Launcher_StoreFileRecord(slot_count++,fileinfo.fname,fileinfo.fsize);
    }
    if(result!=FR_OK) slot_scan_error=1;
    f_closedir(&dir);
    Sort_file(slot_count);
 done:
    slot_counts[slot_platform]=slot_count;
    if(slot_index[slot_platform]>=slot_count) slot_index[slot_platform]=0;
    folder_total=0; game_total_SD=slot_count; recents_view_active=0;
    snprintf(currentpath,sizeof(currentpath),"%s",slot_library[slot_platform]);
    f_chdir(currentpath);
    Slot_ReadTitle(); Slot_FillCache();
}
static unsigned Slot_ShelfAvailable(void)
{
    FRESULT result;const char *ext;
    Slot_SelectDirectory();result=f_opendir(&dir,slot_library[slot_platform]);
    if(result!=FR_OK)return result!=FR_NO_PATH && result!=FR_NO_FILE;
    while((result=f_readdir(&dir,&fileinfo))==FR_OK && fileinfo.fname[0]) {
        if(fileinfo.fattrib&(AM_DIR|AM_HID|AM_SYS) || fileinfo.fname[0]=='.' || strlen(fileinfo.fname)>=100)continue;
        ext=strrchr(fileinfo.fname,'.');if(!ext)continue;
        if(slot_platform==0?(!strcasecmp(ext,".gba") || !strcasecmp(ext,".agb")):!strcasecmp(ext,slot_platform==1?".gb":".gbc")) {f_closedir(&dir);return 1;}
    }
    f_closedir(&dir);return result!=FR_OK;
}
static unsigned Slot_ChangeShelf(int direction)
{
    unsigned original=slot_platform,n;
    Slot_ClearNav();
    for(n=1;n<3;n++) {
        slot_platform=(original+(direction>0?n:3-n))%3;
        if(Slot_ShelfAvailable()) {slot_config_dirty=1;Slot_Scan();return 1;}
    }
    slot_platform=original;return 0;
}
static void Slot_Play(void)
{
    unsigned frame,start,sounded=0;
    if(!slot_count) return;
    slot_config_dirty=1;
    if(!Slot_SaveConfig()) { Slot_Message("Settings could not be saved","B: return"); wait_btn(); }
    Slot_ArmReturn(pFilename_buffer[slot_index[slot_platform]].filename,slot_platform==0);
#ifdef __arm__
    Slot_SetAddonTheme(slot_theme);
#endif
    Slot_ClearNav();Slot_FillCache();
    if(slot_animations) {
        slot_partial_motion=1;start=slot_vblanks;
        do {
            unsigned elapsed=slot_vblanks-start+1;
            if(!sounded && elapsed>=SLOT_INSERT_FRAMES-6){UIAudio_PlaySfx(UI_SFX_LAUNCH);sounded=1;}
            frame=elapsed>=SLOT_INSERT_FRAMES?256:elapsed*256/SLOT_INSERT_FRAMES;
            Slot_DrawShelf(slot_index[slot_platform],0,frame,0);Slot_Present();UIAudio_Update();
        }while(frame<256);
        slot_partial_motion=0;
    } else {UIAudio_PlaySfx(UI_SFX_LAUNCH);Slot_DrawShelf(slot_index[slot_platform],0,256,0);Slot_Present();}
    /* Stock DS Style copies the chosen filename into EWRAM before game loading
       overwrites PSRAM. Keep its save, patch, emulator and hardware boot paths. */
    launcher_carousel_animations=0; launcher_active_page=SD_list;
    gl_boot_mode_pref=slot_addon?2:1;
    slot_loading_dark=slot_theme!=2;
    SD_list_MENU(slot_index[slot_platform],0,0xBB);
    Slot_ArmReturn("",0);
    Launcher_EndLaunchIndicator();
    slot_frame=(uint16_t*)pReadCache;
    SetMode(MODE_3|BG2_ENABLE); Slot_Scan();
}
static unsigned Slot_RefreshClock(void);
static void Slot_CartBack(void)
{
    unsigned dirty=1;
    if(slot_platform!=0 || !slot_count) return;
    Slot_ArmReturn(pFilename_buffer[slot_index[slot_platform]].filename,slot_platform==0);
#ifdef __arm__
    Slot_SetAddonTheme(slot_theme);
#endif
    while(1) {
        unsigned down;
        if(dirty) { Slot_Board(slot_board,71,slot_title,slot_addon,slot_platform); Slot_Present(); dirty=0; }
        VBlankIntrWait(); UIAudio_Update(); scanKeys(); down=keysDown();
        if(slot_vblanks%60==0 && Slot_RefreshClock())dirty=1;
        if(down&KEY_B) { UIAudio_PlayBack(); return; }
        if(slot_platform==0 && down&(KEY_LEFT|KEY_RIGHT)) { slot_addon^=1; slot_config_dirty=1; dirty=1; UIAudio_PlaySfx(UI_SFX_MOVE); }
        if(down&KEY_A) { Slot_Play(); return; }
    }
}
static void Slot_ShowAbout(void)
{
    unsigned armed=0;
    Slot_About();Slot_Present();
    while(1) {
        VBlankIntrWait();UIAudio_Update();scanKeys();
        if(Slot_DialogCanClose(&armed,keysHeld(),keysDown()))return;
    }
}
static void Slot_ShowHelp(void)
{
    unsigned armed=0,page=0,dirty=1;
    while(1) {
        unsigned down;
        if(dirty){Slot_Help(page);Slot_Present();dirty=0;}
        VBlankIntrWait();UIAudio_Update();scanKeys();down=keysDown();
        if(Slot_DialogCanClose(&armed,keysHeld(),down))return;
        if(armed && (down&(KEY_LEFT|KEY_RIGHT))){page^=1;dirty=1;}
    }
}
static void Slot_SettingsMenu(void)
{
    static const char *const labels[]={"Appearance","Skeuomorphic","Animation","Wrap carousel","UI sounds","Boot mode","Select hold","Save states","Sleep patch","Remember shelf","Help","About"};
    const char *values[12];unsigned selected=0,dirty=1;
    Slot_ClearNav();
    while(1) {
        unsigned down,repeat;
        if(dirty) {
            values[0]=slot_theme==0?"Wood":slot_theme==1?"Dark":"Light";
            values[1]=slot_skeuo?"On":"Off";values[2]=slot_animations?"On":"Off";values[3]=slot_wrap?"On":"Off";
            values[4]=launcher_sounds_enabled?"On":"Off";values[5]=slot_addon?"Addons":"Clean";
            values[6]=gl_reset_on?"On":"Off";values[7]=gl_rts_on?"On":"Off";values[8]=gl_sleep_on?"On":"Off";
            values[9]=slot_remember?"On":"Off";values[10]="";values[11]="";
            Slot_Settings(labels,values,12,selected);Slot_Present();dirty=0;
        }
        VBlankIntrWait();UIAudio_Update();scanKeys();down=keysDown();repeat=keysDownRepeat()|down;
        if(slot_vblanks%60==0 && Slot_RefreshClock())dirty=1;
        if(down&KEY_B) {
            if(!Slot_SaveConfig()){Slot_Message("Settings write failed","Check card space");wait_btn();}
            Slot_ResetCache();Slot_FillCache();Slot_ClearNav();return;
        }
        if(repeat&KEY_UP){selected=selected?selected-1:11;dirty=1;}
        if(repeat&KEY_DOWN){selected=(selected+1)%12;dirty=1;}
        if(down&(KEY_A|KEY_LEFT|KEY_RIGHT)) {
            slot_config_dirty=1;dirty=1;
            switch(selected) {
                case 0:slot_theme=(slot_theme+(down&KEY_LEFT?2:1))%3;break;
                case 1:slot_skeuo^=1;break;case 2:slot_animations^=1;break;case 3:slot_wrap^=1;break;
                case 4:launcher_sounds_enabled^=1;break;case 5:slot_addon^=1;break;
                case 6:gl_reset_on^=1;break;case 7:gl_rts_on^=1;break;case 8:gl_sleep_on^=1;break;
                case 9:slot_remember^=1;break;
                case 10:Slot_ShowHelp();break;
                case 11:Slot_ShowAbout();break;
            }
        }
    }
}
static unsigned Slot_RefreshClock(void)
{
    unsigned char time[3];char next[6];unsigned hh,mm;
    rtc_enable();rtc_gettime(time);rtc_disenable();
    hh=UNBCD(time[0]&0x3f);mm=UNBCD(time[1]&0x7f);
    if(hh>23 || mm>59)strcpy(next,"--:--");else snprintf(next,sizeof(next),"%02u:%02u",hh,mm);
    if(!strcmp(next,slot_time))return 0;
    memcpy(slot_time,next,6);return 1;
}
static void Slot_EjectReplay(void)
{
    unsigned start,elapsed,frame;
    Slot_ClearNav();slot_partial_motion=0;
    Slot_DrawShelf(slot_index[slot_platform],0,256,0);Slot_Present();
#ifdef __arm__
    Slot_FadeIn();
#endif
    if(slot_animations) {
        start=slot_vblanks;
        while(slot_vblanks-start<SLOT_EJECT_HOLD_FRAMES){VBlankIntrWait();UIAudio_Update();}
        UIAudio_PlaySfx(UI_SFX_EJECT);start=slot_vblanks;
        do {
            elapsed=slot_vblanks-start+1;
            frame=elapsed>=SLOT_EJECT_FRAMES?0:(SLOT_EJECT_FRAMES-elapsed)*256/SLOT_EJECT_FRAMES;
            Slot_DrawShelf(slot_index[slot_platform],0,frame,0);Slot_Present();UIAudio_Update();
        }while(frame);
    } else {UIAudio_PlaySfx(UI_SFX_EJECT);Slot_DrawShelf(slot_index[slot_platform],0,0,0);Slot_Present();}
    slot_return_pending=0;
    /* The Select that requested the reboot must not enter another UI action. */
    scanKeys();
}
static void Slot_Run(void)
{
    unsigned redraw=1,idle=0,nav_index,steps,ejected=0;int pose=0,last_pose=0,last_base=0;
    slot_frame=(uint16_t*)pReadCache;slot_platform=0;
#ifdef __arm__
    /* Offline-test diagnostic only, after the backend has recovered saves.
       A loader that clears EWRAM must fail closed rather than eject on boot. */
    if(slot_return_request_seen) {
        const char *message=slot_return_save_failed?"Eject skipped: game save write-back is still pending.\n":
            slot_return_pending?"ROM return accepted: warm cookie matched.\n":
            "Eject skipped: SRAM request found, but the powered-session cookie was missing or changed.\n";
        UINT written;
        f_mkdir("/SYSTEM");f_mkdir(SLOT_DIR);
        if(f_open(&gfile,SLOT_DIR "/RETURN.TXT",FA_CREATE_ALWAYS|FA_WRITE)==FR_OK){
            f_write(&gfile,message,strlen(message),&written);f_close(&gfile);
        }
    }
#endif
    gl_show_Thumbnail=LAUNCHER_VIEW_LIST;launcher_sounds_enabled=1;gl_reset_on=1;
    Slot_ReadUsername();Slot_ReadConfig();Slot_RefreshClock();
    if(slot_return_pending)slot_platform=0;
    Slot_Scan();setRepeat(10,4);
    if(slot_return_pending) {
        unsigned i;
        for(i=0;i<slot_count;i++)if(!strcasecmp(pFilename_buffer[i].filename,slot_return_filename))break;
        if(i<slot_count){slot_index[slot_platform]=i;Slot_ReadTitle();Slot_FillCache();}
        else slot_return_pending=0;
    }
    if(!slot_count)Slot_ChangeShelf(1);
    if(!slot_count)Slot_Empty(slot_library[slot_platform],slot_scan_error?"SD read error":0);
    else if(slot_return_pending){Slot_EjectReplay();ejected=1;}
    else Slot_DrawShelf(slot_index[slot_platform],0,0,0);
    Slot_Present();
#ifdef __arm__
    if(!ejected)Slot_FadeIn();
#endif
    redraw=0;last_base=(int)slot_index[slot_platform];
    Slot_ResumeNav(slot_count,slot_index[slot_platform],slot_wrap,slot_animations);
    while(1) {
        unsigned down,art_changed=slot_art_changed;int base,fraction,modal=0;
        slot_art_changed=0;
        UIAudio_Update();scanKeys();down=keysDown();
        Slot_NavSnapshot(&nav_index,&pose,&steps);
        if(slot_index[slot_platform]!=nav_index){slot_index[slot_platform]=nav_index;slot_config_dirty=1;Slot_ReadTitle();}
        if(slot_vblanks%60==0 && Slot_RefreshClock())redraw=1;
        if(down || pose)idle=0;else if(idle<90)idle++;
        if(idle==90 && slot_config_dirty){Slot_SaveConfig();idle++;}
        if(down&(KEY_L|KEY_R|KEY_UP|KEY_DOWN)) {
            Slot_CancelArt();Slot_ClearNav();Slot_ChangeShelf(down&(KEY_L|KEY_UP)?-1:1);modal=1;
        } else if(down&KEY_START) {
            Slot_CancelArt();Slot_ClearNav();Slot_SettingsMenu();modal=1;
        } else if((down&KEY_B) && slot_platform==0) {
            Slot_CancelArt();Slot_ClearNav();Slot_CartBack();Slot_SaveConfig();modal=1;
        } else if(down&KEY_A) {Slot_CancelArt();Slot_ClearNav();Slot_Play();modal=1;}
        if(modal){Slot_ResumeNav(slot_count,slot_index[slot_platform],slot_wrap,slot_animations);nav_index=slot_index[slot_platform];pose=0;redraw=1;}
        base=(int)nav_index+(pose>>8);fraction=pose-(pose>>8)*256;
        if(redraw || art_changed || pose!=last_pose || base!=last_base) {
            slot_partial_motion=!redraw;
            if(!slot_count)Slot_Empty(slot_library[slot_platform],slot_scan_error?"SD read error":0);
            else Slot_DrawShelf(base,fraction,0,0);
            Slot_Present();slot_partial_motion=0;redraw=0;last_pose=pose;last_base=base;
        } else VBlankIntrWait();
        /* Start the next off-screen image half a cart earlier while keeping
           all four potentially visible GB carts resident. */
        Slot_RequestArtAround(base+(fraction>=128));Slot_PumpArt();
    }
}
#endif
