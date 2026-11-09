/* An ordinary GBA ROM exercising the real shared renderer, without flashcart IO. */
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <gba_dma.h>
#include <gba_base.h>
#include "slot_render.h"
#include "slot_format.h"
#include "slot_fast_cache.h"
#include "slot_timing.h"
#include "demo_assets.h"
static struct { unsigned guard0; uint16_t pixels[65536]; unsigned guard1; } canvas EWRAM_BSS;
#define VideoBuffer ((uint16_t*)0x06000000)
#define pReadCache ((unsigned char*)canvas.pixels)
static signed char g_ui_audio_buffer[0x2000] EWRAM_BSS;
static unsigned short launcher_sounds_enabled=1;
static void delay(unsigned n){volatile unsigned i;for(i=0;i<n;i++){};}
#include "slot_audio_driver.h"
#include "slot_transition.h"
#define RGB(r,g,b) ((r)|((g)<<5)|((b)<<10))
static unsigned launcher_launch_screen_active;
static unsigned short slot_loading_dark=1;
static void Launcher_FadeOut(void){Slot_FadeBlack();}
#include "../kernels/omega/source/launch_indicator.h"
#include "../kernels/omega/source/launch_indicator_anchor.h"
extern int Slot_TestHoldStep(unsigned keys,unsigned flags);
extern int Slot_TestHoldDisabled(unsigned keys,unsigned flags);
extern void Slot_TestHoldClear(void);
#define RESULT ((volatile unsigned*)0x0203F000)
static unsigned tests,failures;
static void check(unsigned ok) { tests++;if(!ok) failures++; }
static void le16(unsigned char *p,unsigned n) { p[0]=n;p[1]=n>>8; }
static void le32(unsigned char *p,unsigned n) { p[0]=n;p[1]=n>>8;p[2]=n>>16;p[3]=n>>24; }
static void logic_tests(void)
{
    unsigned count,i,out;int n;unsigned char h[54]={0};
    check(Slot_IndexForCount(0,0,1)==-1);
    for(count=1;count<=512;count*=2) {
        for(n=-513;n<=513;n+=17) {
            int v=Slot_IndexForCount(n,count,1);
            check(v>=0 && (unsigned)v<count);
            check(((v-n)%(int)count)==0);
            check(Slot_IndexForCount(n,count,0)==(n<0 || (unsigned)n>=count?-1:n));
        }
    }
    check(Slot_ParseNatural("0",&out)&&out==0);
    check(Slot_ParseNatural(" 512\r\n",&out)&&out==512);
    check(Slot_ParseNatural("4294967295",&out)&&out==0xffffffff);
    check(!Slot_ParseNatural("4294967296",&out));check(!Slot_ParseNatural("-1",&out));
    check(!Slot_ParseNatural("1garbage",&out));check(!Slot_ParseNatural("",&out));
    check(!Slot_ParseNatural(" \t",&out));check(!Slot_ParseNatural("+1",&out));
    h[0]='B';h[1]='M';le32(h+2,54+120*71*2);le32(h+10,54);le32(h+14,40);
    le32(h+18,120);le32(h+22,71);le16(h+26,1);le16(h+28,16);
    check(Slot_BMPValid(h,54+120*71*2,120,71));
    check(!Slot_BMPValid(h,54+120*71*2-1,120,71));
    check(!Slot_BMPValid(h,54+120*71*2,104,112));
    for(i=0;i<54;i++) {
        unsigned char original=h[i];
        if(i==0 || i==1 || (i>=2 && i<6) || (i>=10 && i<34)) {
            h[i]^=0x80;check(!Slot_BMPValid(h,54+120*71*2,120,71));h[i]=original;
        }
    }
    le32(h+18,0xffffffff);check(!Slot_BMPValid(h,54+120*71*2,120,71));
    le32(h+18,120);le32(h+22,0xffffffb9);check(!Slot_BMPValid(h,54+120*71*2,120,71));
    le32(h+18,104);le32(h+22,112);le32(h+2,54+104*112*2);
    check(Slot_BMPValid(h,54+104*112*2,104,112));
    canvas.guard0=0x1234abcd;canvas.guard1=0x5678ef01;
    Slot_Rect(-100,-100,200,200,0x7fff);Slot_Rect(239,159,99,99,0x7fff);
    Slot_Image(slot_fallback_gba,120,71,-60,-30,120,71);
    Slot_Image(slot_fallback_gbc,104,112,230,150,104,112);
    Slot_Image(slot_fallback_gba,120,71,-999,0,120,71);
    Slot_Text("Clipped edges",-30,158,0x7fff);
    check(canvas.guard0==0x1234abcd && canvas.guard1==0x5678ef01);
    Slot_TestHoldClear();
    for(i=0;i<179;i++)check(!Slot_TestHoldStep(0x3fb,0x10001));
    check(Slot_TestHoldStep(0x3fb,0x10001)==1);
    Slot_TestHoldClear();for(i=0;i<100;i++)Slot_TestHoldStep(0x3fb,0x10001);
    check(!Slot_TestHoldStep(0x3ff,0x10001));
    for(i=0;i<179;i++)check(!Slot_TestHoldStep(0x3fb,0x10001));
    check(Slot_TestHoldStep(0x3fb,0x10001)==1);
    Slot_TestHoldClear();for(i=0;i<200;i++)check(!Slot_TestHoldStep(0x3f3,0x10001));
    Slot_TestHoldClear();for(i=0;i<200;i++)check(!Slot_TestHoldDisabled(0x3fb,0x10001));
    Slot_TestHoldClear();for(i=0;i<200;i++)check(!Slot_TestHoldStep(0x3fb,0x10001000));
    check(!Slot_TestHoldStep(0x3fb,0x10001));
    Slot_TestHoldClear();for(i=0;i<200;i++)check(!Slot_TestHoldStep(0x3fb,0x10000));
    check(Slot_Ease(0,12)==0);check(Slot_Ease(12,12)==256);check(Slot_Ease(20,12)==256);
    for(i=1;i<=12;i++)check(Slot_Ease(i,12)>=Slot_Ease(i-1,12));
    {
        unsigned short source[32],destination[20];int offsets[18];unsigned length,j;
        for(j=0;j<32;j++)source[j]=j*37+19;
        for(j=0;j<18;j++)offsets[j]=((j*5)%32)*2;
        for(length=0;length<=17;length++) {
            for(j=0;j<20;j++)destination[j]=0x7777;
            Slot_BlitOpaqueRow((const unsigned char*)source,destination+1,offsets,length);
            check(destination[0]==0x7777);check(destination[length+1]==0x7777);
            for(j=0;j<length;j++)check(destination[j+1]==source[(j*5)%32]);
        }
    }
    {
        unsigned short data[260] __attribute__((aligned(4)));unsigned block,j,bad=0;
        for(block=0;block<256;block++) {
            data[0]=0x1234;data[1]=0x5678;data[258]=0x9abc;data[259]=0xdef0;
            for(j=0;j<256;j++)data[j+2]=block*256+j;
            Slot_DecodeBMP555(data+2,256);
            for(j=0;j<256;j++) {
                unsigned c=block*256+j;
                unsigned expected=c==0x7c1f?SLOT_KEY:((c&31)<<10)|(c&0x3e0)|((c>>10)&31);
                if(data[j+2]!=expected)bad++;
            }
            check(data[0]==0x1234 && data[1]==0x5678 && data[258]==0x9abc && data[259]==0xdef0);
        }
        check(!bad);
        for(j=0;j<18;j++) {
            unsigned k;
            for(k=0;k<20;k++)data[k]=0x7c1f;
            Slot_DecodeBMP555(data,j);
            for(k=0;k<20;k++)check(data[k]==(k<j?SLOT_KEY:0x7c1f));
        }
    }
    RESULT[0]=0x534c4f54;RESULT[1]=tests;RESULT[2]=failures;
}
static void present(void) { VBlankIntrWait();dmaCopy(slot_frame+slot_redraw_top*240,VideoBuffer+slot_redraw_top*240,(slot_redraw_bottom-slot_redraw_top)*240*2); }
static void shelf(int index,int progress,int drop)
{
    const uint16_t *sources[5];
    int i;
    for(i=0;i<5;i++)sources[i]=(slot_skeuo?demo_real:demo_flat)[slot_platform][Slot_IndexForCount(index+i-2,3,1)];
    Slot_DrawShelfCarts(sources,progress,drop);
    if(!slot_partial_motion || drop)Slot_Chrome(3,index,drop?0:demo_titles[slot_platform][Slot_IndexForCount(index,3,1)],0,0);
}
static void profile_start(void) {
    volatile unsigned short *timers=(volatile unsigned short*)0x04000108;
    timers[1]=0;timers[3]=0;timers[0]=0;timers[2]=0;timers[3]=0x84;timers[1]=0x80;
}
static unsigned profile_stop(void) {
    volatile unsigned short *timers=(volatile unsigned short*)0x04000108;
    timers[1]=0;timers[3]=0;return timers[0]|((unsigned)timers[2]<<16);
}
static void __attribute__((noinline,optimize("Os"))) legacy_bmp_conversion(uint16_t *p,unsigned count)
{while(count--){unsigned c=*p;*p++=c==0x7c1f?SLOT_KEY:((c&31)<<10)|(c&0x3e0)|((c>>10)&31);}}
static void decoder_benchmark(void)
{
    unsigned i;
    for(i=0;i<104*112;i++)canvas.pixels[i]=i%17?(i*31)&0x7fff:0x7c1f;
    profile_start();legacy_bmp_conversion(canvas.pixels,104*112);RESULT[21]=profile_stop();
    for(i=0;i<104*112;i++)canvas.pixels[i]=i%17?(i*31)&0x7fff:0x7c1f;
    profile_start();Slot_DecodeBMP555(canvas.pixels,104*112);RESULT[20]=profile_stop();
}
int main(void)
{
    unsigned indices[3]={0},screen=0,selected=0,redraw=1,animations=1,wrap=1,addon=1,frame,moves=0;
    unsigned nav_index,nav_steps=0,seen_steps=0,about_armed=0,help_page=0;int pose=0,last_pose=0,last_base=0;
    const char *const labels[]={"Appearance","Skeuomorphic","Animation","Wrap carousel","UI sounds","Boot mode","Select hold","Save states","Sleep patch","Remember shelf","Show clock","Help","About"};
    enum { total=sizeof(labels)/sizeof(*labels) };
    const char *values[]={"Wood","On","On","On","On","Addons","On","Off","Off","On","On","",""};
    irqInit();irqSet(IRQ_VBLANK,Slot_VBlank);irqEnable(IRQ_VBLANK);SetMode(MODE_3|BG2_ENABLE);setRepeat(10,4);UIAudio_Init();
    slot_frame=canvas.pixels;strcpy(slot_time,"14:32");strcpy(slot_username,"frankiet19");logic_tests();decoder_benchmark();
    Slot_BootSplash();Slot_FadeBlack();shelf(0,0,0);present();Slot_FadeIn();Slot_ResumeNav(3,0,wrap,animations);
    while(1) {
        unsigned down,repeat;int base,fraction;
        Slot_NavSnapshot(&nav_index,&pose,&nav_steps);
        if(slot_navigation.enabled){indices[slot_platform]=nav_index;moves+=nav_steps-seen_steps;seen_steps=nav_steps;}
        RESULT[3]=slot_platform;RESULT[4]=indices[slot_platform];RESULT[5]=screen;RESULT[6]=slot_theme;
        RESULT[7]=moves;RESULT[8]=slot_skeuo;RESULT[9]=pose!=0;RESULT[17]=slot_navigation.offset;RESULT[18]=slot_navigation.velocity;RESULT[10]=slot_vblanks;
        if(RESULT[16]){if(RESULT[16]&4){screen=4;Slot_ClearNav();}if(RESULT[16]&1)Slot_InvalidateFast();redraw=1;RESULT[16]=0;}
        UIAudio_Update();scanKeys();down=keysDown();repeat=keysDownRepeat()|down;
        if(screen==1) {
            if(down&KEY_B){screen=0;Slot_ClearNav();redraw=1;}
            if(repeat&KEY_DOWN){selected=(selected+1)%total;redraw=1;}
            if(repeat&KEY_UP){selected=selected?selected-1:total-1;redraw=1;}
            if(down&(KEY_A|KEY_LEFT|KEY_RIGHT)) {
                if(selected==0)slot_theme=(slot_theme+1)%3;
                if(selected==1)slot_skeuo^=1;
                if(selected==2)animations^=1;
                if(selected==3)wrap^=1;
                if(selected==5)addon^=1;
                if(selected==10)slot_clock_visible^=1;
                if(selected==11){screen=6;help_page=0;about_armed=0;}
                if(selected==12){screen=5;about_armed=0;}
                redraw=1;
            }
        } else if(screen==3 || screen==4 || screen==5 || screen==6) {
            Slot_ClearNav();
            if((screen==5 || screen==6)?Slot_DialogCanClose(&about_armed,keysHeld(),down):!!(down&KEY_B)){if(screen==3)Launcher_EndLaunchIndicator();screen=(screen==5 || screen==6)?1:0;redraw=1;}
            if(screen==6 && about_armed && (down&(KEY_LEFT|KEY_RIGHT))){help_page^=1;redraw=1;}
        } else {
            if(down&KEY_START){Slot_ClearNav();selected=0;screen=1;redraw=1;}
            else if((down&KEY_B) && slot_platform==0){Slot_ClearNav();screen=screen==2?0:2;redraw=1;}
            else if(down&KEY_A) {
                unsigned sounded=0;Slot_ClearNav();
                if(animations) {
                    unsigned start=slot_vblanks;slot_partial_motion=1;
                    do {
                        unsigned elapsed=slot_vblanks-start+1;
                        if(!sounded && elapsed>=SLOT_INSERT_FRAMES-6){UIAudio_PlaySfx(UI_SFX_LAUNCH);sounded=1;}
                        frame=elapsed>=SLOT_INSERT_FRAMES?256:elapsed*256/SLOT_INSERT_FRAMES;
                        shelf(indices[slot_platform],0,frame);present();
                    }while(frame<256);
                    slot_partial_motion=0;
                } else {UIAudio_PlaySfx(UI_SFX_LAUNCH);shelf(indices[slot_platform],0,256);present();}
                slot_loading_dark=slot_theme!=2;Launcher_BeginLaunchScreen();screen=3;redraw=0;
            } else if(screen==2) {
                if(down&(KEY_LEFT|KEY_RIGHT)){addon^=1;redraw=1;}
                Slot_ClearNav();
            } else if(down&(KEY_L|KEY_R|KEY_UP|KEY_DOWN)) {
                Slot_ClearNav();slot_platform=(slot_platform+(down&(KEY_L|KEY_UP)?2:1))%3;redraw=1;
            }
        }
        if(screen!=0)Slot_ClearNav();
        else if(!slot_navigation.enabled){Slot_ResumeNav(3,indices[slot_platform],wrap,animations);seen_steps=0;pose=0;redraw=1;}
        base=(int)indices[slot_platform]+(pose>>8);fraction=pose-(pose>>8)*256;
        if(screen==0 && !redraw && (pose!=last_pose || base!=last_base)) {
            slot_partial_motion=1;
            profile_start();shelf(base,fraction,0);RESULT[11]=profile_stop();present();slot_partial_motion=0;
            last_pose=pose;last_base=base;
        } else if(redraw) {
            if(screen==0){shelf(base,fraction,0);last_pose=pose;last_base=base;}
            if(screen==1) {
                values[0]=slot_theme==0?"Wood":slot_theme==1?"Dark":"Light";values[1]=slot_skeuo?"On":"Off";
                values[2]=animations?"On":"Off";values[3]=wrap?"On":"Off";values[5]=addon?"Addons":"Clean";
                values[10]=slot_clock_visible?"On":"Off";
                Slot_Settings(labels,values,total,selected);
            }
            if(screen==2)Slot_Board(slot_board,71,demo_titles[slot_platform][indices[slot_platform]],addon,slot_platform);
            if(screen==3)shelf(indices[slot_platform],0,256);
            if(screen==4)Slot_Empty("/Games/GBA",0);
            if(screen==5)Slot_About();
            if(screen==6)Slot_Help(help_page);
            present();redraw=0;
        } else VBlankIntrWait();
    }
}
