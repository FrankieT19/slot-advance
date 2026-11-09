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
static void Slot_ReserveFastPixels(const uint16_t *source)
{
    unsigned i;
    for(i=0;i<4;i++)if(slot_fast_source[i]==source)slot_fast_used[i]=1;
}
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

static void Slot_DrawShelfCarts(const uint16_t *const sources[5],int progress,int drop)
{
    const uint16_t *pixels[5]={0};
    int centres[5],scales[5],i,pass;
    memset(slot_fast_used,0,sizeof(slot_fast_used));
    for(i=0;i<5;i++)if(sources[i] && (!drop || i==2)) {
        int relative=(i-2)*256-progress;
        int centre=Slot_CarouselCentre(slot_platform,relative);
        int scale=Slot_CarouselScale(relative);
        int width=Slot_ScaledExtent(slot_widths[slot_platform],scale);
        int left=centre-width/2;
        if(left+width>0 && left<240) {
            centres[i]=centre;scales[i]=scale;pixels[i]=sources[i];
            /* Keep every visible hit before an entering cart can take a bank. */
            Slot_ReserveFastPixels(sources[i]);
        }
    }
    for(i=0;i<5;i++)if(pixels[i])pixels[i]=Slot_FastPixels(pixels[i],slot_platform);
    Slot_BeginShelf(slot_partial_motion && !drop,slot_partial_motion?(slot_platform==0?44:13):0,
                    slot_partial_motion?(drop?160:(slot_platform==0?122:131)):160);
    for(pass=0;pass<2;pass++) {
        if(pass)Slot_BeginShelfPaint();
        for(i=0;i<5;i++)if(pixels[i])Slot_CartQ8(pixels[i],slot_platform,centres[i],scales[i],i==2?drop:0);
    }
    Slot_EndShelf();
}
#endif
