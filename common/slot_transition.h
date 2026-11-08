#ifndef SLOT_TRANSITION_H
#define SLOT_TRANSITION_H
#ifdef __arm__
static void Slot_FadeBlack(void)
{
    unsigned i;REG_BLDCNT=0x00ff;
    for(i=0;i<=16;i+=2){VBlankIntrWait();UIAudio_Update();REG_BLDY=i;}
}
static void Slot_FadeIn(void)
{
    int i;REG_BLDCNT=0x00ff;
    for(i=16;i>=0;i-=2){VBlankIntrWait();UIAudio_Update();REG_BLDY=i;}
    REG_BLDCNT=0;REG_BLDY=0;
}
static void Slot_BootSplash(void)
{
    unsigned i;REG_BLDCNT=0x00ff;REG_BLDY=16;REG_DISPCNT=MODE_3|BG2_ENABLE;
    dmaCopy(slot_splash,VideoBuffer,240*160*2);
    Slot_FadeIn();for(i=0;i<18;i++)VBlankIntrWait();
}
#endif
#endif
