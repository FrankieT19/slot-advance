/* slot. advance, GPL-3.0-or-later. Shared by the kernels and the emulator preview. */
#ifndef SLOT_RENDER_H
#define SLOT_RENDER_H
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "slot_motion.h"
#include "slot_version.h"
#include "slot_username.h"
#include "slot_hotkeys.h"
#include "slot_assets.h"
#ifdef __arm__
#include <gba_base.h>
#include <gba_dma.h>
#define SLOT_FAST IWRAM_CODE ARM_CODE __attribute__((noinline,optimize("O2")))
#define SLOT_COPY(dst,src,n) dmaCopy((src),(dst),(n))
#define SLOT_RENDER_RAM EWRAM_BSS
#else
#define SLOT_FAST
#define SLOT_COPY(dst,src,n) memcpy((dst),(src),(n))
#define SLOT_RENDER_RAM
#endif
#define SLOT_RGB(r,g,b) ((uint16_t)(((r)>>3)|(((g)>>3)<<5)|(((b)>>3)<<10)))
#define SLOT_KEY 0x8000
#define SLOT_W 240
#define SLOT_H 160
#ifdef __arm__
extern void Slot_BlitOpaqueRow(const unsigned char *row,uint16_t *destination,const int *offsets,int count);
extern void Slot_DecodeBMP555(uint16_t *pixels,unsigned count);
#else
static void Slot_BlitOpaqueRow(const unsigned char *row,uint16_t *destination,const int *offsets,int count)
{while(count--)*destination++=*(const uint16_t*)(row+*offsets++);}
static void Slot_DecodeBMP555(uint16_t *pixels,unsigned count)
{while(count--){unsigned c=*pixels;*pixels++=c==0x7c1f?SLOT_KEY:((c&31)<<10)|(c&0x3e0)|((c>>10)&31);}}
#endif
static uint16_t *slot_frame;
static unsigned slot_theme; /* 0 wood, 1 dark, 2 light */
static unsigned slot_platform;
static unsigned slot_skeuo=1;
static unsigned slot_clock_visible=1;
static char slot_time[6]="--:--";
static char slot_username[SLOT_USERNAME_MAX+1];
static int slot_redraw_top=0,slot_redraw_bottom=160;
static unsigned slot_partial_motion=0;
typedef struct {const uint16_t *pixels;unsigned h;unsigned char left[112],right[112];} SlotOpaque;
static SlotOpaque slot_opaque[4] SLOT_RENDER_RAM;
static void SLOT_FAST Slot_RegisterOpaque(unsigned bank,const uint16_t *pixels,unsigned w,unsigned h)
{
    unsigned x,y;SlotOpaque *o=&slot_opaque[bank];o->pixels=pixels;o->h=h;
    for(y=0;y<h;y++) {
        const uint16_t *p=pixels+y*w;unsigned l=0,r=w;
        while(l<r && p[l]==SLOT_KEY)l++;
        while(r>l && p[r-1]==SLOT_KEY)r--;
        o->left[y]=l;o->right[y]=r;
        for(x=l;x<r;x++)if(p[x]==SLOT_KEY){o->left[y]=255;break;}
    }
}
static const SlotOpaque *Slot_OpaqueInfo(const uint16_t *pixels)
{
    unsigned i;for(i=0;i<4;i++)if(slot_opaque[i].pixels==pixels)return &slot_opaque[i];return 0;
}
typedef struct {unsigned char count;unsigned char left[4],right[4];} SlotCoverage;
static SlotCoverage slot_coverage_banks[2][160] SLOT_RENDER_RAM;
static SlotCoverage *slot_covered=slot_coverage_banks[0],*slot_next_covered=slot_coverage_banks[1];
static unsigned slot_layout_only,slot_difference,slot_coverage_safe,slot_old_coverage_safe;
static const char *const slot_platform_names[3] = {"GBA", "GB", "GBC"};
static const uint16_t *const slot_fallbacks[3] = {slot_fallback_gba,slot_fallback_gb,slot_fallback_gbc};
static const uint16_t *const slot_flat_fallbacks[3] = {slot_flat_gba,slot_flat_gb,slot_flat_gbc};
static const uint16_t *Slot_Fallback(unsigned platform) {return slot_skeuo?slot_fallbacks[platform]:slot_flat_fallbacks[platform];}
static const unsigned slot_widths[3] = {120,104,104};
static const unsigned slot_heights[3] = {71,112,112};
static uint16_t Slot_Ink(void) { return slot_theme == 2 ? SLOT_RGB(40,40,48) : SLOT_RGB(244,239,226); }
static uint16_t Slot_Ground(void) {return slot_theme==1?0:SLOT_RGB(235,235,231);}
static void Slot_Pixel(int x,int y,uint16_t colour)
{
    if((unsigned)x<240 && (unsigned)y<160) slot_frame[y*240+x]=colour;
}
static void SLOT_FAST Slot_Rect(int x,int y,int w,int h,uint16_t colour)
{
    int x1=x<0?0:x,y1=y<0?0:y,x2=x+w>240?240:x+w,y2=y+h>160?160:y+h;
    int xx,yy;
    for(yy=y1;yy<y2;yy++) for(xx=x1;xx<x2;xx++) slot_frame[yy*240+xx]=colour;
}
static void SLOT_FAST Slot_Text(const char *text,int x,int y,uint16_t colour)
{
    int row,col;
    unsigned char c;
    while((c=(unsigned char)*text++)) {
        if(c<32 || c>126) c='?';
        for(row=0;row<12;row++) for(col=0;col<8;col++)
            if((slot_font[c][row]&(128>>col)) && (unsigned)(x+col)<240 && (unsigned)(y+row)<160) slot_frame[(y+row)*240+x+col]=colour;
        x+=6;
        if(x>=240) break;
    }
}
static void Slot_Centred(const char *text,int y,uint16_t colour)
{
    Slot_Text(text,(240-(int)strlen(text)*6)/2,y,colour);
}
static inline void Slot_MapScaledEdges(unsigned char *inverse,unsigned sw,unsigned w,unsigned step)
{
    unsigned sx,dx=0,position=0;
    for(sx=0;sx<=sw;sx++) {
        while(dx<w && (position>>16)<sx){dx++;position+=step;}
        inverse[sx]=dx;
    }
}
static void SLOT_FAST Slot_Image(const uint16_t *src,int sw,int sh,int x,int y,int w,int h)
{
    int xx,yy,x1=x<0?-x:0,y1=y<0?-y:0,x2=w,y2=h;
    int map[120]; unsigned step_x,step_y;
    unsigned char inverse[121];
    const SlotOpaque *opaque=Slot_OpaqueInfo(src);
    if(x+w>240) x2=240-x;
    if(y+h>160) y2=160-y;
    if(w<=0 || w>120 || h<=0 || x1>=x2 || y1>=y2) return;
    if(w==sw && h==sh && !opaque) {
        for(yy=y1;yy<y2;yy++) {
            const uint16_t *s=src+yy*sw+x1;uint16_t *d=slot_frame+(y+yy)*240+x+x1;
            for(xx=x1;xx<x2;xx++){uint16_t c=*s++;if(c!=SLOT_KEY)*d=c;d++;}
        }
        return;
    }
    step_x=((unsigned)sw<<16)/(unsigned)w;step_y=((unsigned)sh<<16)/(unsigned)h;
    if(opaque)Slot_MapScaledEdges(inverse,sw,w,step_x);
    /* Byte offsets let ARM7 load a scaled pixel directly from its row base.
       Avoid an addition and shift inside every pixel iteration. */
    for(xx=x1;xx<x2;xx++) map[xx]=((xx*step_x)>>16)*2;
    for(yy=y1;yy<y2;yy++) {
        unsigned sy=((unsigned)yy*step_y)>>16;int first=x1,last=x2;
        const unsigned char *s=(const unsigned char*)(src+sy*sw);
        uint16_t *d;const int *offset;int remaining;
        if(opaque && opaque->left[sy]!=255) {
            int left=inverse[opaque->left[sy]],right=inverse[opaque->right[sy]];
            if(first<left)first=left;
            if(last>right)last=right;
            if(first>=last)continue;
            d=slot_frame+(y+yy)*240+x+first;offset=map+first;remaining=last-first;
            if(w==sw && h==sh)SLOT_COPY(d,s+first*2,remaining*2);
            else Slot_BlitOpaqueRow(s,d,offset,remaining);
        } else {
            d=slot_frame+(y+yy)*240+x+x1;offset=map+x1;remaining=x2-x1;
            while(remaining--) {uint16_t c=*(const uint16_t*)(s+*offset++);if(c!=SLOT_KEY)*d=c;d++;}
        }
    }
}
static void SLOT_FAST Slot_Band(const uint16_t *src,int y)
{
    int xx,yy;
    for(yy=0;yy<19;yy++) for(xx=0;xx<240;xx++) {
        uint16_t c=src[yy*240+xx];
        if(c!=SLOT_KEY && (unsigned)(y+yy)<160) slot_frame[(y+yy)*240+xx]=c;
    }
}
static void Slot_BackgroundRange(int top,int bottom)
{
    slot_redraw_top=top;slot_redraw_bottom=bottom;
    if(slot_theme==0) SLOT_COPY(slot_frame+top*240,slot_background+top*240,(bottom-top)*240*2);
    else Slot_Rect(0,top,240,bottom-top,Slot_Ground());
    if(bottom>141) Slot_Band(slot_skeuo?(slot_theme==2?slot_back_light:slot_back_dark):(slot_theme==2?slot_back_flat_light:slot_back_flat_dark),141);
}
static void Slot_Front(void) {Slot_Band(slot_skeuo?(slot_theme==2?slot_front_light:slot_front_dark):(slot_theme==2?slot_front_flat_light:slot_front_flat_dark),141);}
static void Slot_Background(void) { Slot_BackgroundRange(0,160); }
static inline void Slot_RecordSpan(int y,int left,int right)
{
    SlotCoverage *row;
    if((unsigned)y>=160 || right<=0 || left>=240)return;
    if(left<0)left=0;
    if(right>240)right=240;
    if(left>=right)return;
    row=&slot_next_covered[y];
    if(row->count>=4){slot_coverage_safe=0;return;}
    row->left[row->count]=left;row->right[row->count]=right;row->count++;
}
static void SLOT_FAST Slot_RecordCart(const uint16_t *pixels,unsigned sw,unsigned sh,int x,int y,int w,int h)
{
    const SlotOpaque *o=Slot_OpaqueInfo(pixels);unsigned step_x,step_y;int yy;unsigned char inverse[121];
    if(!o){slot_coverage_safe=0;return;}
    step_x=(sw<<16)/w;step_y=(sh<<16)/h;
    Slot_MapScaledEdges(inverse,sw,w,step_x);
    for(yy=0;yy<h && y+yy<160;yy++)if(y+yy>=0) {
        unsigned sy=(yy*step_y)>>16;
        if(o->left[sy]==255){slot_coverage_safe=0;return;}
        Slot_RecordSpan(y+yy,x+inverse[o->left[sy]],x+inverse[o->right[sy]]);
    }
}
static void SLOT_FAST Slot_BeginShelf(unsigned partial,int top,int bottom)
{
    int y;partial=partial && slot_old_coverage_safe;
    slot_redraw_top=top;slot_redraw_bottom=bottom;slot_difference=partial;slot_coverage_safe=1;
    for(y=0;y<160;y++)slot_next_covered[y].count=0;
    slot_layout_only=1;
    if(!partial)Slot_BackgroundRange(top,bottom);
}
static void SLOT_FAST Slot_BeginShelfPaint(void)
{
    int y;slot_layout_only=0;
    if(!slot_difference)return;
    if(!slot_coverage_safe){Slot_BackgroundRange(slot_redraw_top,slot_redraw_bottom);return;}
    for(y=slot_redraw_top;y<slot_redraw_bottom;y++) {
        unsigned i,j;SlotCoverage *old=&slot_covered[y],*next=&slot_next_covered[y];
        j=0;
        for(i=0;i<old->count;i++) {
            int cursor=old->left[i],right=old->right[i];
            while(cursor<right) {
                int gap=right;
                while(j<next->count && next->right[j]<=cursor)j++;
                if(j<next->count && next->left[j]<=cursor)cursor=next->right[j]<right?next->right[j]:right;
                else {
                    if(j<next->count && next->left[j]<gap)gap=next->left[j];
                    if(slot_theme==0)SLOT_COPY(slot_frame+y*240+cursor,slot_background+y*240+cursor,(gap-cursor)*2);
                    else Slot_Rect(cursor,y,gap-cursor,1,Slot_Ground());
                    cursor=gap;
                }
            }
        }
    }
}
static void SLOT_FAST Slot_EndShelf(void){SlotCoverage *previous=slot_covered;slot_covered=slot_next_covered;slot_next_covered=previous;slot_old_coverage_safe=slot_coverage_safe;}
static void Slot_Clock(void)
{
    /* DS Style glyphs are 6 pixels wide and 12 high, with no resampling. */
    Slot_Text(slot_username,6,145,Slot_Ink());
    if(slot_clock_visible)Slot_Text(slot_time,204,145,Slot_Ink());
}
static void Slot_BlankShelf(void)
{
    Slot_Background();Slot_Front();Slot_Clock();
}
static void Slot_Chrome(unsigned count,unsigned index,const char *title,unsigned scroll,const char *notice)
{
    (void)count; (void)index; (void)title; (void)scroll;
    if(notice && *notice) {
        Slot_Rect(0,126,240,14,SLOT_RGB(40,29,28));
        Slot_Centred(notice,127,SLOT_RGB(255,211,143));
    }
    Slot_Front();
    Slot_Clock();
}
static void Slot_Empty(const char *directory,const char *error)
{
    Slot_Background();
    Slot_Centred(error&&*error?error:"Your shelf is empty",55,Slot_Ink());
    Slot_Centred("Put unzipped ROMs in",73,Slot_Ink());
    Slot_Centred(directory,85,Slot_Ink());
    Slot_Chrome(0,0,"L/R: change system",0,0);
}
static int Slot_CarouselScale(int relative)
{int dist=relative<0?-relative:relative;return 25600-(dist>256?256:dist)*22;}
static int Slot_CarouselCentre(unsigned platform,int relative)
{
    int position=relative*(int)slot_widths[platform];
    return 120+((position+128-(position<0))>>8);
}
static int Slot_ScaledExtent(unsigned extent,int scale)
{return (extent*scale+12800)/25600;}
static void SLOT_FAST Slot_CartQ8(const uint16_t *pixels,unsigned platform,int centre,int scale,int drop)
{
    /* Round once, in output pixels. Whole-percent scale and truncation used to
       leave a final one-percent size jump when the spring reached rest. */
    int w=Slot_ScaledExtent(slot_widths[platform],scale),h=Slot_ScaledExtent(slot_heights[platform],scale);
    int y=platform==0 ? 116-h : 126-h;
    if(drop>0) {
        int target_w=80,target_h=platform==0?47:86;
        int foot=y+h,seated=143+target_h,catch=(141-foot)*256/(seated-foot),travel,k;
        if(drop>256)drop=256;
        /* Slot's travel/catch/creep/push path: recede to slot width first. */
        if(drop<108)travel=catch*(int)Slot_Smooth(drop*256/108)/256;
        else if(drop<159)travel=catch+8*(drop-108)/51;
        else travel=catch+8+(256-catch-8)*(int)Slot_Smooth((drop-159)*256/97)/256;
        k=travel<catch?travel*256/catch:256;
        w=w+(target_w-w)*k/256;h=h+(target_h-h)*k/256;
        y=foot+(seated-foot)*travel/256-h;
    }
    /* A small shadow anchors the selected cartridge to the desk. */
    if(slot_layout_only) {
        Slot_RecordCart(pixels,slot_widths[platform],slot_heights[platform],centre-w/2,y,w,h);
        if(slot_skeuo && scale>=95*256 && !drop){Slot_RecordSpan(y+h+2,centre-w/2+5,centre-w/2+w-5);Slot_RecordSpan(y+h+3,centre-w/2+5,centre-w/2+w-5);}
        return;
    }
    if(slot_skeuo && scale>=95*256 && drop==0) Slot_Rect(centre-w/2+5,y+h+2,w-10,2,slot_theme==2?SLOT_RGB(200,200,196):slot_theme==0?SLOT_RGB(24,21,21):0);
    Slot_Image(pixels,slot_widths[platform],slot_heights[platform],centre-w/2,y,w,h);
}
static void Slot_Cart(const uint16_t *pixels,unsigned platform,int centre,int scale,int drop)
{Slot_CartQ8(pixels,platform,centre,scale*256,drop);}
static void Slot_SettingsPage(const char *title,const char *const *labels,const char *const *values,unsigned total,unsigned selected)
{
    unsigned top=selected>4?selected-4:0,i; char row[48];
    Slot_Background();
    Slot_Rect(18,17,204,124,slot_theme==1?SLOT_RGB(54,54,59):SLOT_RGB(245,245,241));
    Slot_Text(title,30,24,slot_theme==1?0x7fff:SLOT_RGB(43,41,39));
    for(i=0;i<6 && top+i<total;i++) {
        unsigned n=top+i; int y=47+i*14;
        uint16_t ink=slot_theme==1?0x7fff:SLOT_RGB(43,41,39);
        if(n==selected) { Slot_Rect(25,y-2,190,12,SLOT_RGB(74,64,59)); ink=0x7fff; }
        Slot_Text(labels[n],31,y-1,ink);
        snprintf(row,sizeof(row),"%s",values[n]);
        Slot_Text(row,207-(int)strlen(row)*6,y-1,ink);
    }
    Slot_Clock();
}
static void Slot_Settings(const char *const *labels,const char *const *values,unsigned total,unsigned selected)
{Slot_SettingsPage("Settings",labels,values,total,selected);}
static void Slot_About(void)
{
    uint16_t ink=Slot_Ink();Slot_BlankShelf();
    Slot_Centred(SLOT_DISPLAY_NAME,27,ink);
    Slot_Centred(SLOT_DISPLAY_VERSION,41,ink);
    Slot_Centred("Art and design",59,ink);
    Slot_Centred("Brandon Kowalski / Markus",72,ink);
    Slot_Centred("GBA port by frankiet19",97,ink);
    Slot_Centred("Built on DS Style",111,ink);
    Slot_Clock();
}
static const char *Slot_KeyName(unsigned key)
{
    static const char *const names[]={"A","B","Select","Start","Right","Left","Up","Down","R","L"};
    unsigned i=0;
    while(key>1){key>>=1;i++;}
    return names[i];
}
static void Slot_Help(unsigned page,unsigned hold,unsigned chord)
{
    static const char *const menu[]={
        "Left/Right: choose game",
        "L/R or Up/Down: change shelf",
        "A: play   Start: settings",
        "B: GBA boot options / back",
        "Settings: Up/Down to select",
        "A/Left/Right: change option"};
    char hold_line[36],menu_line[36];
    unsigned first=chord&-chord;
    const char *const ingame[]={"GBA Addons and GB/GBC:",hold_line,menu_line,
        "Enable Hold to return in Settings.",
        "Save states On enables save/load.",
        "GB/GBC: B resumes the game."};
    unsigned i;uint16_t ink=Slot_Ink();Slot_BlankShelf();
    Slot_Text(page?"Help 2/2 - In-game":"Help 1/2 - Menu",18,14,ink);
    if(page){
        snprintf(hold_line,sizeof(hold_line),"Hold %s 3s: return and eject",Slot_KeyName(hold));
        snprintf(menu_line,sizeof(menu_line),"%s+%s: in-game menu",Slot_KeyName(chord^first),Slot_KeyName(first));
    }
    for(i=0;i<6;i++)Slot_Text((page?ingame:menu)[i],18,34+i*13,ink);
    Slot_Text("Left/Right: pages   B: back",18,128,ink);
    Slot_Clock();
}
static void Slot_Board(const uint16_t *board,int board_h,const char *title,unsigned addon,unsigned platform)
{
    Slot_Background();
    Slot_Image(board,120,board_h,60,25,120,board_h);
    Slot_Rect(28,101,184,36,SLOT_RGB(36,36,41));
    Slot_Centred(platform==0?(addon?"BOOT WITH ADDONS":"CLEAN BOOT"):"DS STYLE GB/GBC EMULATOR",105,0x7fff);
    Slot_Centred(platform==0?"LEFT/RIGHT: choose boot mode":"Emulator selected by kernel",118,SLOT_RGB(181,183,177));
    Slot_Clock();
    (void)title;
}
#endif

