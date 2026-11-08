/* Four EWRAM drawing slots avoid PSRAM reads for every scaled pixel.
   Two occupy idle framebuffer scratch; two have dedicated banks. */
#ifndef SLOT_FAST_CACHE_H
#define SLOT_FAST_CACHE_H
#ifdef __arm__
#define SLOT_CACHE_RAM EWRAM_BSS
#else
#define SLOT_CACHE_RAM
#endif
#define SLOT_FAST_PIXELS (104*112)
static uint16_t slot_fast_extra[2][SLOT_FAST_PIXELS] SLOT_CACHE_RAM;
static const uint16_t *slot_fast_source[4];
static unsigned slot_fast_used[4];
static void Slot_InvalidateFast(void) { memset(slot_fast_source,0,sizeof(slot_fast_source)); }
static void Slot_InvalidateFastSource(const uint16_t *source) {unsigned i;for(i=0;i<4;i++)if(slot_fast_source[i]==source)slot_fast_source[i]=0;}
static void Slot_BeginFastFrame(void) { memset(slot_fast_used,0,sizeof(slot_fast_used)); }
static const uint16_t *Slot_FastPixels(const uint16_t *source,unsigned platform)
{
    unsigned i,n=4,pixels=slot_widths[platform]*slot_heights[platform];uint16_t *destination;
    for(i=0;i<4;i++)if(slot_fast_source[i]==source){n=i;break;}
    if(n==4)for(i=0;i<4;i++)if(!slot_fast_used[i]){n=i;break;}
    if(n==4)return source;
    destination=n>=2?slot_fast_extra[n-2]:slot_frame+240*160+n*SLOT_FAST_PIXELS;
    if(slot_fast_source[n]!=source){SLOT_COPY(destination,source,pixels*2);slot_fast_source[n]=source;Slot_RegisterOpaque(n,destination,slot_widths[platform],slot_heights[platform]);}
    slot_fast_used[n]=1;return destination;
}
#endif
