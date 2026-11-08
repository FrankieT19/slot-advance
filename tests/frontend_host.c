/* Host integration harness for the unmodified shared Slot frontend.
   Windows: tcc -Icommon tests/frontend_host.c -o frontend-host.exe
   Run with a disposable fixture-card directory as the only argument. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <direct.h>
#include <sys/stat.h>
#include <setjmp.h>
#ifndef _Static_assert
#define ASSERT_NAME2(a,b) a##b
#define ASSERT_NAME(a,b) ASSERT_NAME2(a,b)
#define _Static_assert(c,m) typedef char ASSERT_NAME(static_assertion_,__LINE__)[(c)?1:-1]
#endif
#define EWRAM_BSS
#define MAX_files 512
#define MAX_path_len 256
#define MAX_pReadCache_size 0x20000
#define LAUNCHER_FILE_LIST_PSRAM_ADDRESS 0x08FF0000
#define LAUNCHER_VIEW_LIST 0
#define SD_list 0
#define MODE_3 3
#define BG2_ENABLE 0x400
#define FA_READ 1
#define FA_WRITE 2
#define FA_CREATE_ALWAYS 8
#define AM_DIR 0x10
#define AM_HID 2
#define AM_SYS 4
#define FR_OK 0
#define FR_DISK_ERR 1
#define FR_NO_FILE 4
#define FR_NO_PATH 5
#define FR_EXIST 8
#define KEY_A 1
#define KEY_B 2
#define KEY_SELECT 4
#define KEY_START 8
#define KEY_RIGHT 16
#define KEY_LEFT 32
#define KEY_UP 64
#define KEY_DOWN 128
#define KEY_R 256
#define KEY_L 512
#define UI_SFX_MOVE 1
#define UI_SFX_TAB 2
#define UI_SFX_LAUNCH 3
#define UI_SFX_EJECT 4
#define strcasecmp _stricmp
#define dmaCopy(src,dst,n) memcpy((dst),(src),(n))
typedef unsigned short u16;typedef unsigned u32;typedef unsigned UINT;typedef int FRESULT;
typedef struct { FILE *file; unsigned size; int error; } FIL;
typedef struct { HANDLE handle; WIN32_FIND_DATAA found; unsigned first; } DIR;
typedef struct { char fname[256]; unsigned fsize,fattrib; } FILINFO;
typedef struct { char filename[100];unsigned filesize; } FM_FILE_FS;
#define pFilename_buffer ((FM_FILE_FS*)LAUNCHER_FILE_LIST_PSRAM_ADDRESS)
static unsigned char pReadCache[MAX_pReadCache_size];
static uint16_t mock_video[240*160];
#define VideoBuffer mock_video
static FM_FILE_FS pFilename_temp;
static char currentpath[256];
static unsigned folder_total,game_total_SD,recents_view_active,launcher_active_page;
static unsigned gl_show_Thumbnail,launcher_sounds_enabled,launcher_carousel_animations;
static unsigned slot_loading_dark;
static unsigned gl_reset_on,gl_rts_on,gl_sleep_on,gl_cheat_on,gl_boot_mode_pref;
static FIL gfile;static DIR dir;static FILINFO fileinfo;
static char card_root[1024],mock_cwd[256]="/";
static unsigned io_short_write,io_fail_read,io_read_calls,io_fail_rename,io_dir_error,io_dir_calls;
static unsigned io_max_read;
static unsigned io_short_read;
static unsigned mock_keys[128],mock_event,mock_key,mock_boots,mock_boot_index,mock_waits;
static void map_path(const char *name,char *out)
{
    if(name[0]=='/')snprintf(out,1400,"%s%s",card_root,name);
    else snprintf(out,1400,"%s%s/%s",card_root,mock_cwd,name);
}
static int f_open(FIL *f,const char *name,unsigned flags)
{
    char path[1400];map_path(name,path);f->error=0;
    f->file=fopen(path,flags&FA_WRITE?"wb":"rb");
    if(!f->file)return FR_NO_FILE;
    fseek(f->file,0,SEEK_END);f->size=(unsigned)ftell(f->file);rewind(f->file);return FR_OK;
}
static int f_close(FIL *f) { int r=fclose(f->file);f->file=0;return r?FR_DISK_ERR:FR_OK; }
static int f_read(FIL *f,void *p,unsigned n,UINT *got)
{
    io_read_calls++;if(n>io_max_read)io_max_read=n;
    if(io_fail_read && io_read_calls>=io_fail_read) { *got=0;f->error=1;return FR_DISK_ERR; }
    *got=(UINT)fread(p,1,io_short_read?n/2:n,f->file);return ferror(f->file)?FR_DISK_ERR:FR_OK;
}
static int f_write(FIL *f,const void *p,unsigned n,UINT *got)
{
    if(io_short_write) n/=2;
    *got=(UINT)fwrite(p,1,n,f->file);return ferror(f->file)?FR_DISK_ERR:FR_OK;
}
static char *f_gets(char *p,int n,FIL *f) { return fgets(p,n,f->file); }
static unsigned f_size(FIL *f) { return f->size; }
static int f_error(FIL *f) { return f->error || ferror(f->file); }
static int f_lseek(FIL *f,unsigned n) { return fseek(f->file,n,SEEK_SET)?FR_DISK_ERR:FR_OK; }
static int f_mkdir(const char *name) { char p[1400];map_path(name,p);return _mkdir(p)?FR_EXIST:FR_OK; }
static int f_unlink(const char *name) { char p[1400];map_path(name,p);return remove(p)?FR_NO_FILE:FR_OK; }
static int f_rename(const char *from,const char *to)
{
    char a[1400],b[1400];map_path(from,a);map_path(to,b);
    if(io_fail_rename && strstr(from,"SETTINGS.TMP"))return FR_DISK_ERR;
    return rename(a,b)?FR_NO_FILE:FR_OK;
}
static int f_opendir(DIR *d,const char *name)
{
    char p[1400];struct stat st;map_path(name,p);
    if(stat(p,&st) || !(st.st_mode&S_IFDIR))return FR_NO_PATH;
    strcat(p,"/*");d->handle=FindFirstFileA(p,&d->found);d->first=1;return FR_OK;
}
static int f_readdir(DIR *d,FILINFO *info)
{
    WIN32_FIND_DATAA *w=&d->found;unsigned have=d->first;d->first=0;io_dir_calls++;
    if(io_dir_error && io_dir_calls>=io_dir_error)return FR_DISK_ERR;
    if(!have)have=FindNextFileA(d->handle,w);
    if(d->handle==INVALID_HANDLE_VALUE || !have) { info->fname[0]=0;return FR_OK; }
    snprintf(info->fname,sizeof(info->fname),"%s",w->cFileName);info->fsize=w->nFileSizeLow;info->fattrib=0;
    if(w->dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)info->fattrib|=AM_DIR;
    if(w->dwFileAttributes&FILE_ATTRIBUTE_HIDDEN)info->fattrib|=AM_HID;
    if(w->dwFileAttributes&FILE_ATTRIBUTE_SYSTEM)info->fattrib|=AM_SYS;
    return FR_OK;
}
static int f_closedir(DIR *d) { if(d->handle!=INVALID_HANDLE_VALUE)FindClose(d->handle);return FR_OK; }
static int f_chdir(const char *p) { snprintf(mock_cwd,sizeof(mock_cwd),"%s",p);return FR_OK; }
static volatile unsigned slot_vblanks;
static unsigned capture_motion,capture_count,capture_hashes[128],eject_sounds;
static unsigned centre_hash(void)
{
    unsigned x,y,h=2166136261u;uint16_t *pixels=(uint16_t*)pReadCache;
    for(y=0;y<143;y++)for(x=80;x<160;x++)h=(h^pixels[y*240+x])*16777619u;
    return h;
}
static void VBlankIntrWait(void) {
    slot_vblanks++;
    if(capture_motion && capture_count<128)capture_hashes[capture_count++]=centre_hash();
}
static void SetPSRampage(unsigned page) { (void)page; }
static void SetMode(unsigned mode) { (void)mode; }
static void setRepeat(unsigned a,unsigned b) { (void)a;(void)b; }
static void UIAudio_Update(void) {}
static void UIAudio_PlaySfx(unsigned id) { if(id==UI_SFX_EJECT)eject_sounds++; }
static void UIAudio_PlayBack(void) {}
static void scanKeys(void) { mock_key=mock_keys[mock_event<127?mock_event++:127]; }
static unsigned keysHeld(void){return mock_key;}
static unsigned keysDown(void) { return mock_key; }
static unsigned keysDownRepeat(void) { return 0; } /* Assert first-press handling works. */
static void wait_btn(void) { mock_waits++; }
static void Launcher_EndLaunchIndicator(void) {}
static unsigned SD_list_MENU(unsigned index,unsigned select,unsigned recent)
{
    if(select || recent!=0xbb)abort();mock_boots++;mock_boot_index=index;
    /* Launch destroys shared scratch exactly as the actual backend does. */
    memset(pReadCache,0xdd,sizeof(pReadCache));return 0;
}
static void Launcher_StoreFileRecord(unsigned index,const char *name,unsigned size)
{
    memset(&pFilename_temp,0,sizeof(pFilename_temp));strcpy(pFilename_temp.filename,name);pFilename_temp.filesize=size;
    memcpy(&pFilename_buffer[index],&pFilename_temp,sizeof(pFilename_temp));
}
static int filecmp(const void *a,const void *b) { return _stricmp(((const FM_FILE_FS*)a)->filename,((const FM_FILE_FS*)b)->filename); }
static void Sort_file(unsigned count) { qsort(pFilename_buffer,count,sizeof(FM_FILE_FS),filecmp); }
#define UNBCD(x) (((x)&15)+(((x)>>4)*10))
static void rtc_enable(void){}
static void rtc_disenable(void){}
static void rtc_gettime(unsigned char *time){time[0]=0x14;time[1]=0x32;time[2]=0;}
#include "slot_frontend.h"
static unsigned checks,failed;
#define CHECK(x) do {checks++;if(!(x)){failed++;printf("FAIL line %d: %s\n",__LINE__,#x);}} while(0)
static void put_file(const char *name,const void *data,unsigned bytes)
{
    FIL f;UINT wrote;if(f_open(&f,name,FA_WRITE)!=FR_OK)abort();f_write(&f,data,bytes,&wrote);f_close(&f);
}
static unsigned read_file(const char *name,char *data,unsigned bytes)
{
    FIL f;UINT got=0;if(f_open(&f,name,FA_READ)!=FR_OK)return 0;f_read(&f,data,bytes,&got);f_close(&f);return got;
}
static void events(unsigned a,unsigned b,unsigned c,unsigned d,unsigned e)
{
    memset(mock_keys,0,sizeof(mock_keys));mock_keys[0]=a;mock_keys[1]=b;mock_keys[2]=c;mock_keys[3]=d;mock_keys[4]=e;mock_event=0;
}
static void config_tests(void)
{
    char before[512],after[512],crlf[700];unsigned a,b,i,j;
    slot_theme=2;slot_platform=2;slot_index[0]=3;slot_index[1]=4;slot_index[2]=5;slot_addon=1;
    gl_reset_on=1;gl_rts_on=1;slot_config_dirty=1;
    CHECK(Slot_SaveConfig());CHECK(!slot_config_dirty);
    slot_theme=0;slot_platform=0;slot_index[1]=0;gl_reset_on=0;
    Slot_ReadConfig();CHECK(slot_theme==2);CHECK(slot_platform==2);CHECK(slot_index[1]==4);CHECK(gl_reset_on==1);
    a=read_file(SLOT_CONFIG,before,sizeof(before));CHECK(a>100);
    for(i=0,j=0;i<a;i++){if(before[i]=='\n')crlf[j++]='\r';crlf[j++]=before[i];}
    put_file(SLOT_CONFIG,crlf,j);slot_theme=0;Slot_ReadConfig();CHECK(slot_theme==2);
    slot_theme=0;slot_config_dirty=1;CHECK(Slot_SaveConfig());
    slot_theme=1;slot_config_dirty=1;CHECK(Slot_SaveConfig()); /* Backup is now theme=0. */
    put_file(SLOT_CONFIG,"SlotOmega=1\nTheme=2\n",19);
    slot_theme=2;Slot_ReadConfig();CHECK(slot_theme==0);
    slot_theme=1;slot_config_dirty=1;CHECK(Slot_SaveConfig());
    a=read_file(SLOT_CONFIG,before,sizeof(before));
    slot_theme=2;slot_config_dirty=1;io_short_write=1;CHECK(!Slot_SaveConfig());io_short_write=0;
    b=read_file(SLOT_CONFIG,after,sizeof(after));CHECK(a==b && !memcmp(before,after,a));
    io_fail_rename=1;CHECK(!Slot_SaveConfig());io_fail_rename=0;
    b=read_file(SLOT_CONFIG,after,sizeof(after));CHECK(a==b && !memcmp(before,after,a));
    CHECK(Slot_SaveConfig());CHECK(!slot_config_dirty);
    put_file(SLOT_CONFIG,"SlotOmega=1\nTheme=4294967296\nEnd=1\n",34);
    slot_theme=2;Slot_ReadConfig();CHECK(slot_theme==1); /* Restore complete backup. */
    f_unlink(SLOT_CONFIG);f_unlink(SLOT_CONFIG_BAK);
    slot_theme=2;Slot_ReadConfig();CHECK(slot_theme==2);
}
static void image_tests(void)
{
    uint16_t *dst=(uint16_t*)SLOT_CACHE_BASE;unsigned char bad[54];char path[200];unsigned i;
    slot_platform=0;io_read_calls=0;
    CHECK(Slot_LoadBMP("/SYSTEM/SLOT/CARTS/GBA/ORBIT RUNNER.bmp",dst));
    CHECK(dst[0]==SLOT_KEY);CHECK(dst[35*120+60]!=SLOT_KEY);
    for(i=0;i<SLOT_CACHE_PIXELS;i++)dst[i]=0x5555;
    io_fail_read=2;io_read_calls=0;
    CHECK(!Slot_LoadBMP("/SYSTEM/SLOT/CARTS/GBA/ORBIT RUNNER.bmp",dst));io_fail_read=0;
    CHECK(dst[0]==0x5555 && dst[SLOT_CACHE_PIXELS-1]==0x5555);
    memset(bad,0,sizeof(bad));bad[0]='B';bad[1]='M';put_file("/SYSTEM/SLOT/bad.bmp",bad,54);
    CHECK(!Slot_LoadBMP("/SYSTEM/SLOT/bad.bmp",dst));CHECK(dst[0]==0x5555);
    CHECK(!Slot_LoadBMP("/SYSTEM/SLOT/missing.bmp",dst));
    slot_platform=1;CHECK(!Slot_LoadBMP("/SYSTEM/SLOT/CARTS/GBA/ORBIT RUNNER.bmp",dst));
}
static void scan_tests(void)
{
    unsigned i;char path[200];
    f_mkdir("/Games");f_mkdir("/Games/GBA");f_mkdir("/Games/GB");
    put_file("/Games/GBA/zulu.gba","Z",1);put_file("/Games/GBA/alpha.gba","A",1);
    put_file("/Games/GBA/Bravo.agb","B",1);put_file("/Games/GBA/ignore.txt","X",1);
    slot_platform=0;slot_index[0]=0;slot_wrap=1;Slot_Scan();
    CHECK(slot_count==3);CHECK(!strcmp(pFilename_buffer[0].filename,"alpha.gba"));
    CHECK(!strcmp(pFilename_buffer[1].filename,"Bravo.agb"));CHECK(!slot_scan_error);
    CHECK(game_total_SD==3 && folder_total==0);
    CHECK(!strcmp(currentpath,"/Games/GBA"));CHECK(Slot_GetCache(0));CHECK(Slot_GetCache(-1));
    CHECK(Slot_GetCache(0)->missing);
    Slot_DrawShelf(0,0,0,0);CHECK(slot_frame==(uint16_t*)pReadCache);
    slot_platform=1;Slot_Scan();CHECK(slot_count==0 && !slot_scan_error);
    Slot_Play();CHECK(mock_boots==0); /* Empty shelf cannot launch. */
    slot_platform=2;Slot_Scan();CHECK(slot_count==0 && slot_scan_missing && !slot_scan_error);
    slot_platform=0;io_dir_error=4;io_dir_calls=0;Slot_Scan();CHECK(slot_scan_error);io_dir_error=0;
    for(i=0;i<515;i++){snprintf(path,sizeof(path),"/Games/GBA/Game %03u.gba",i);put_file(path,"G",1);}
    Slot_Scan();CHECK(slot_count==512);CHECK(slot_warning);
    for(i=0;i<512;i++)CHECK(strlen(pFilename_buffer[i].filename)<100);
    slot_animations=0;slot_addon=0;slot_index[0]=0;
    Slot_Play();CHECK(mock_boots==1 && mock_boot_index==0);CHECK(gl_boot_mode_pref==1);
    CHECK(slot_frame==(uint16_t*)pReadCache && slot_count==512);
    slot_addon=1;Slot_Play();CHECK(mock_boots==2 && gl_boot_mode_pref==2);
}
static void username_tests(void)
{
    uint16_t *before=malloc(240*160*2);unsigned x,y;
    f_unlink("/USERNAME.TXT");strcpy(slot_username,"old");Slot_ReadUsername();CHECK(!slot_username[0]);
    put_file("/USERNAME.TXT","\xef\xbb\xbf  Frankie \r\nignored",23);Slot_ReadUsername();CHECK(!strcmp(slot_username,"Frankie"));
    put_file("/USERNAME.TXT","abcdefghijklmnop\n",17);Slot_ReadUsername();CHECK(!strcmp(slot_username,"abcdefghij"));
    slot_theme=2;slot_skeuo=1;slot_username[0]=0;Slot_BlankShelf();memcpy(before,slot_frame,240*160*2);
    Slot_ReadUsername();Slot_Clock();
    for(y=0;y<160;y++)for(x=0;x<240;x++)if(before[y*240+x]!=slot_frame[y*240+x])CHECK(x>=6 && x<68 && y>=145 && y<157);
    put_file("/USERNAME.TXT","J\xc3\xb6rg",5);Slot_ReadUsername();CHECK(!strcmp(slot_username,"J?rg"));
    put_file("/USERNAME.TXT","\nSECOND",7);Slot_ReadUsername();CHECK(!slot_username[0]);
    put_file("/USERNAME.TXT","\xff\xfeN\0",4);Slot_ReadUsername();CHECK(!slot_username[0]);
    io_fail_read=1;Slot_ReadUsername();CHECK(!slot_username[0] && !gfile.file);io_fail_read=0;
    f_unlink("/USERNAME.TXT");Slot_ReadUsername();free(before);
}
static void menu_tests(void)
{
    slot_theme=0;slot_animations=1;
    events(KEY_DOWN,KEY_A,KEY_B,0,0);Slot_SettingsMenu();
    CHECK(slot_skeuo==0);CHECK(mock_event==3);
    events(KEY_A,KEY_B,0,0,0);Slot_SettingsMenu();CHECK(slot_theme==1);
    slot_platform=0;slot_count=1;slot_addon=0;
    events(KEY_RIGHT,KEY_B,0,0,0);Slot_CartBack();CHECK(slot_addon==1);
    slot_platform=1;slot_addon=0;
    events(KEY_RIGHT,KEY_B,0,0,0);Slot_CartBack();CHECK(slot_addon==0 && mock_event==0);
    slot_platform=2;Slot_CartBack();CHECK(mock_event==0);
    events(KEY_A,KEY_A,0,KEY_B,0);Slot_ShowAbout();CHECK(mock_event==4);
    events(KEY_UP,KEY_A,KEY_A,0,KEY_B);mock_keys[5]=0;mock_keys[6]=KEY_B;
    Slot_SettingsMenu();CHECK(mock_event==7); /* About remains the last item after adding Help. */
    events(KEY_A,KEY_A,0,KEY_RIGHT,KEY_B);Slot_ShowHelp();CHECK(mock_event==5);
    events(KEY_UP,KEY_UP,KEY_A,KEY_A,0);mock_keys[5]=KEY_RIGHT;mock_keys[6]=KEY_B;mock_keys[7]=0;mock_keys[8]=KEY_B;
    Slot_SettingsMenu();CHECK(mock_event==9); /* Help row, page switch, return. */

}
static void refinement_tests(void)
{
    unsigned i;
    Slot_ResumeNav(512,0,1,1);
    SlotNav_Tick(&slot_navigation,KEY_RIGHT,0);CHECK(slot_navigation.index==1);
    SlotNav_Tick(&slot_navigation,0,17);
    SlotNav_Tick(&slot_navigation,KEY_RIGHT,34);CHECK(slot_navigation.index==2);
    SlotNav_Tick(&slot_navigation,KEY_LEFT,51);CHECK(slot_navigation.index==1);
    SlotNav_Tick(&slot_navigation,KEY_LEFT,450);CHECK(slot_navigation.index==1);
    SlotNav_Tick(&slot_navigation,KEY_LEFT,451);CHECK(slot_navigation.index==0);
    SlotNav_Tick(&slot_navigation,0,468);
    for(i=0;i<90;i++)SlotNav_Tick(&slot_navigation,0,485+i*17);
    CHECK(slot_navigation.index==0 && !slot_navigation.offset && !slot_navigation.direction);
    Slot_ClearNav();SlotNav_Tick(&slot_navigation,KEY_RIGHT,3000);
    Slot_ResumeNav(512,0,1,1);SlotNav_Tick(&slot_navigation,KEY_RIGHT,3017);CHECK(slot_navigation.index==0);
    slot_platform=0;slot_index[0]=0;slot_wrap=1;slot_skeuo=1;Slot_Scan();
    /* GBA populated; GB empty; GBC missing. No cycling through blank shelves. */
    CHECK(!Slot_ChangeShelf(1));CHECK(slot_platform==0);
    f_mkdir("/Games/GBC");put_file("/Games/GBC/Test.gbc","G",1);
    CHECK(Slot_ChangeShelf(1));CHECK(slot_platform==2 && slot_count==1);
    CHECK(Slot_ChangeShelf(-1));CHECK(slot_platform==0);
    slot_skeuo=0;slot_config_dirty=1;CHECK(Slot_SaveConfig());slot_skeuo=1;Slot_ReadConfig();CHECK(slot_skeuo==0);
    Slot_About();CHECK(slot_redraw_top==0 && slot_redraw_bottom==160);
    {
        uint16_t *reference=malloc(240*160*2);unsigned style,theme,step,platform;char path[100];
        for(platform=1;platform<3;platform++) {
            snprintf(path,sizeof(path),"/Games/%s",slot_platform_names[platform]);f_mkdir(path);
            for(i=0;i<5;i++){snprintf(path,sizeof(path),"/Games/%s/Mould %u.%s",slot_platform_names[platform],i,platform==1?"gb":"gbc");put_file(path,"G",1);}
        }
        for(platform=0;platform<3;platform++) {
        slot_platform=platform;slot_index[platform]=0;Slot_Scan();
        for(style=0;style<2;style++)for(theme=0;theme<3;theme++) {
            slot_skeuo=style;slot_theme=theme;Slot_ResetCache();Slot_FillCache();
            slot_partial_motion=0;Slot_DrawShelf(0,0,0,0);
            memcpy(reference,slot_frame,240*160*2);
            for(i=0;i<2;i++) {
                Slot_DrawShelf(0,i?1:-1,0,0);
                CHECK(!memcmp(reference,slot_frame,240*160*2));
            }
            for(step=1;step<=16;step++) {
                int progress=(step<=8?1:-1)*(int)Slot_Ease((step-1)%8+1,8);
                slot_partial_motion=1;Slot_DrawShelf(0,progress,0,0);memcpy(reference,slot_frame,240*160*2);
                slot_partial_motion=0;Slot_DrawShelf(0,progress,0,0);
                CHECK(!memcmp(reference,slot_frame,240*160*2));
            }
        }
        }
        free(reference);
    }
}
static unsigned stream_steady_misses,stream_steady_frames;
static void streaming_tests(void)
{
    const char *names[3]={"ORBIT RUNNER","MOON POST","PRISM QUEST"};
    uint16_t *reference=malloc(SLOT_CACHE_PIXELS*2),*frame=malloc(240*160*2);
    unsigned platform,style,pumps,i;char name[110],path[220];
    for(platform=0;platform<3;platform++)for(style=0;style<2;style++) {
        Slot_ResetCache();slot_platform=platform;slot_skeuo=style;slot_count=1;slot_wrap=1;slot_index[platform]=0;
        snprintf(name,sizeof(name),"%s.%s",names[platform],platform==0?"gba":platform==1?"gb":"gbc");
        Launcher_StoreFileRecord(0,name,1);
        snprintf(path,sizeof(path),"/SYSTEM/SLOT/%s/%s/%s.bmp",style?"CARTS":"CARTS_FLAT",slot_platform_names[platform],names[platform]);
        CHECK(Slot_LoadBMP(path,reference));
        io_max_read=0;Slot_RequestArtAround(0);
        CHECK(slot_art_active && !Slot_GetCache(0));CHECK(io_max_read==54);
        Slot_DrawShelf(0,0,0,0);memcpy(frame,slot_frame,240*160*2);
        pumps=0;
        while(slot_art_active && pumps<10) {
            io_max_read=0;Slot_PumpArt();pumps++;
            CHECK(io_max_read<=slot_widths[platform]*SLOT_ART_ROWS*2);
            Slot_DrawShelf(0,0,0,0);
            if(slot_art_active) {
                CHECK(!Slot_GetCache(0)); /* No partially decoded photo is drawn. */
                CHECK(!memcmp(frame,slot_frame,240*160*2));
            }
        }
        CHECK(pumps==(slot_heights[platform]+SLOT_ART_ROWS-1)/SLOT_ART_ROWS);
        CHECK(!slot_art_active && slot_art_file.file==0 && slot_art_changed);
        CHECK(Slot_GetCache(0) && !Slot_GetCache(0)->missing);
        CHECK(!memcmp(reference,Slot_CachedPixels(Slot_GetCache(0)),slot_widths[platform]*slot_heights[platform]*2));
        Slot_ResetCache();Slot_RequestArtAround(0);Slot_PumpArt();
        CHECK(slot_art_active);Slot_ResetCache();CHECK(!slot_art_active && !slot_art_file.file);
        Slot_RequestArtAround(0);pumps=0;
        while(slot_art_active && pumps++<10)Slot_PumpArt();
        CHECK(Slot_GetCache(0) && !Slot_GetCache(0)->missing);
        Slot_ResetCache();Slot_RequestArtAround(0);io_fail_read=1;Slot_PumpArt();io_fail_read=0;
        CHECK(!slot_art_active && !slot_art_file.file && Slot_GetCache(0)->missing);
        Slot_ResetCache();Slot_RequestArtAround(0);io_short_read=1;Slot_PumpArt();io_short_read=0;
        CHECK(!slot_art_active && !slot_art_file.file && Slot_GetCache(0)->missing);
    }
    /* A rapid jump cancels an irrelevant job and immediately services the new
       range. Native text fallback remains available for absent/malformed art. */
    Slot_ResetCache();slot_platform=0;slot_count=10;slot_skeuo=1;
    Launcher_StoreFileRecord(0,"ORBIT RUNNER.gba",1);
    for(i=1;i<10;i++){snprintf(name,sizeof(name),"Missing %u.gba",i);Launcher_StoreFileRecord(i,name,1);}
    Slot_RequestArtAround(0);CHECK(slot_art_active);Slot_RequestArtAround(5);
    CHECK(!slot_art_active && !slot_art_file.file && !Slot_GetCache(0));
    CHECK(Slot_GetCache(5) && Slot_GetCache(5)->missing);
    put_file("/SYSTEM/SLOT/CARTS/GBA/Missing 5.bmp","BM",2);
    Slot_ResetCache();Slot_RequestArtAround(5);
    CHECK(!slot_art_active && !slot_art_file.file && Slot_GetCache(5)->missing);
    /* A complete held-scroll/reversal run with the real cache, decoder and
       renderer. One chunk per displayed frame, as in the cartridge loop. */
    for(platform=0;platform<3;platform++)for(style=0;style<2;style++) {
        unsigned bytes,frame_number,ticks=0,misses=0;
        Slot_ResetCache();slot_platform=platform;slot_skeuo=style;slot_count=20;slot_wrap=1;slot_index[platform]=0;
        snprintf(path,sizeof(path),"/SYSTEM/SLOT/%s/%s/%s.bmp",style?"CARTS":"CARTS_FLAT",slot_platform_names[platform],names[platform]);
        bytes=read_file(path,(char*)reference,SLOT_CACHE_PIXELS*2);CHECK(bytes>54);
        for(i=0;i<20;i++) {
            snprintf(name,sizeof(name),"Stream %02u.%s",i,platform==0?"gba":platform==1?"gb":"gbc");
            Launcher_StoreFileRecord(i,name,1);
            snprintf(path,sizeof(path),"/SYSTEM/SLOT/%s/%s/Stream %02u.bmp",style?"CARTS":"CARTS_FLAT",slot_platform_names[platform],i);
            put_file(path,reference,bytes);
        }
        Slot_FillCache();Slot_ResumeNav(20,0,1,1);slot_navigation.previous=0;
        for(frame_number=0;frame_number<210;frame_number++) {
            unsigned selected,steps,held=frame_number<90?KEY_RIGHT:frame_number<180?KEY_LEFT:0;
            int pose,base,fraction,d;
            for(i=0;i<2;i++){ticks++;SlotNav_Tick(&slot_navigation,held,(unsigned)(((uint64_t)ticks*4389000)>>18));}
            Slot_NavSnapshot(&selected,&pose,&steps);slot_index[platform]=selected;
            base=(int)selected+(pose>>8);fraction=pose-(pose>>8)*256;
            if((frame_number>=20 && frame_number<85) || (frame_number>=115 && frame_number<175)) {
                for(d=-2;d<=2;d++) {
                    int relative=d*256-fraction,centre=Slot_CarouselCentre(platform,relative);
                    int half=Slot_ScaledExtent(slot_widths[platform],Slot_CarouselScale(relative))/2;
                    if(centre+half>0 && centre-half<240 && !Slot_GetCache(base+d))misses++;
                }
                stream_steady_frames++;
            }
            slot_partial_motion=frame_number!=0;Slot_DrawShelf(base,fraction,0,0);
            Slot_RequestArtAround(base+(fraction>=128));Slot_PumpArt();
            CHECK(slot_navigation.enabled); /* IO never controls navigation. */
        }
        stream_steady_misses+=misses;printf("Stream %u/%u: %u steady visible cache misses\n",platform,style,misses);
        CHECK(!misses);Slot_ResetCache();
    }
    free(reference);free(frame);
}
__declspec(dllexport) unsigned Slot_HostStreamMisses(void){return stream_steady_misses;}
static void eject_tests(void)
{
    unsigned insert[64],i,rest;
    slot_platform=0;slot_index[0]=0;slot_animations=1;slot_partial_motion=0;Slot_Scan();Slot_DrawShelf(0,0,0,0);rest=centre_hash();
    capture_count=0;capture_motion=1;Slot_Play();capture_motion=0;
    CHECK(capture_count==SLOT_INSERT_FRAMES+1); /* Mock launch returns, then scans once. */
    memcpy(insert,capture_hashes,sizeof(unsigned)*SLOT_INSERT_FRAMES);
    capture_count=0;capture_motion=1;slot_return_pending=1;eject_sounds=0;Slot_EjectReplay();capture_motion=0;
    CHECK(capture_count==1+SLOT_EJECT_HOLD_FRAMES+SLOT_EJECT_FRAMES);
    for(i=0;i<=SLOT_EJECT_HOLD_FRAMES;i++)CHECK(capture_hashes[i]==insert[SLOT_INSERT_FRAMES-1]);
    for(i=1;i<SLOT_EJECT_FRAMES;i++)CHECK(capture_hashes[SLOT_EJECT_HOLD_FRAMES+i]==insert[SLOT_INSERT_FRAMES-1-i]);
    CHECK(capture_hashes[capture_count-1]==rest);CHECK(!slot_return_pending);CHECK(eject_sounds==1);
    slot_animations=0;slot_return_pending=1;eject_sounds=0;Slot_EjectReplay();
    CHECK(centre_hash()==rest && !slot_return_pending && eject_sounds==1);
}
__declspec(dllexport) unsigned Slot_HostStreamFrames(void){return stream_steady_frames;}
__declspec(dllexport) unsigned Slot_HostCheckCount(void){return checks;}
__declspec(dllexport) int main(int argc,char **argv)
{
    void *memory;if(argc!=2)return 2;snprintf(card_root,sizeof(card_root),"%s",argv[1]);
    memory=VirtualAlloc((void*)SLOT_CACHE_BASE,0x30000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(memory!=(void*)SLOT_CACHE_BASE){printf("Cannot map mock PSRAM\n");return 2;}
    slot_frame=(uint16_t*)pReadCache;f_mkdir("/SYSTEM");f_mkdir(SLOT_DIR);
    config_tests();image_tests();scan_tests();username_tests();menu_tests();refinement_tests();streaming_tests();eject_tests();
    printf("Frontend host integration: %u checks, %u failed\n",checks,failed);
    VirtualFree(memory,0,MEM_RELEASE);return failed?1:0;
}

