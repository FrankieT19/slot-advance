/* Small format checks shared by firmware, preview and regression tests. GPL-3.0-or-later. */
#ifndef SLOT_FORMAT_H
#define SLOT_FORMAT_H
#include <stdint.h>
#include <limits.h>
static uint16_t Slot_Le16(const unsigned char *p) { return p[0]|(uint16_t)p[1]<<8; }
static uint32_t Slot_Le32(const unsigned char *p) { return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static int Slot_IndexForCount(int index,unsigned count,unsigned wrap)
{
    if(!count) return -1;
    if(wrap) { index%=(int)count; if(index<0) index+=(int)count; return index; }
    return index<0 || index>=(int)count?-1:index;
}
static unsigned Slot_ParseNatural(const char *text,unsigned *out)
{
    unsigned value=0,digit,found=0;
    while(*text==' ' || *text=='\t') text++;
    while(*text>='0' && *text<='9') {
        digit=(unsigned)(*text++-'0');
        if(value>(UINT_MAX-digit)/10) return 0;
        value=value*10+digit; found=1;
    }
    while(*text==' ' || *text=='\t' || *text=='\r' || *text=='\n') text++;
    if(!found || *text) return 0;
    *out=value; return 1;
}
static unsigned Slot_BMPValid(const unsigned char *h,unsigned bytes,unsigned expected_w,unsigned expected_h)
{
    unsigned w=Slot_Le32(h+18),height=Slot_Le32(h+22);
    /* Only the exact even-width converter format is accepted; no allocation or
       file-controlled dimensions reach the image cache. */
    return h[0]=='B' && h[1]=='M' && Slot_Le32(h+14)==40 && Slot_Le32(h+10)==54 &&
           Slot_Le16(h+26)==1 && Slot_Le16(h+28)==16 && Slot_Le32(h+30)==0 &&
           expected_w>0 && expected_h>0 && expected_w<=120 && expected_h<=112 && !(expected_w&1) &&
           w==expected_w && height==expected_h && bytes==54+w*height*2 && Slot_Le32(h+2)==bytes;
}
#endif

