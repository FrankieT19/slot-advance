/* DS Style FIFO/timer driver, with the original Slot insertion clip. */
#define REG_SOUNDCNT_L_UI   (*(volatile u16*)0x04000080)
#define REG_SOUNDCNT_H_UI   (*(volatile u16*)0x04000082)
#define REG_SOUNDCNT_X_UI   (*(volatile u16*)0x04000084)
#define REG_TM0CNT_L_UI     (*(volatile u16*)0x04000100)
#define REG_TM0CNT_H_UI     (*(volatile u16*)0x04000102)
#define REG_TM1CNT_L_UI     (*(volatile u16*)0x04000104)
#define REG_TM1CNT_H_UI     (*(volatile u16*)0x04000106)
#define REG_DMA1SAD_UI      (*(volatile u32*)0x040000BC)
#define REG_DMA1DAD_UI      (*(volatile u32*)0x040000C0)
#define REG_DMA1CNT_UI      (*(volatile u32*)0x040000C4)
#define REG_FIFO_A_UI       (*(volatile u32*)0x040000A0)
#define REG_SOUNDBIAS_UI    (*(volatile u16*)0x04000088)
#define REG_IF_UI           (*(volatile u16*)0x04000202)

#define UI_DMA_ENABLE       0x80000000
#define UI_DMA_TIMING_FIFO  0x30000000
#define UI_DMA_32BIT        0x04000000
#define UI_DMA_REPEAT       0x02000000
#define UI_DMA_DST_FIXED    0x00200000
#define UI_DMA_SRC_INC      0x00000000
#define UI_DMA_COUNT_4      0x00000004

#define UI_TIMER_ENABLE     0x0080
#define UI_TIMER_FREQ_1     0x0000
#define UI_TIMER_FREQ_1024  0x0003
#define UI_TIMER_COUNT_UP   0x0004
#define UI_TIMER_IRQ        0x0040

#define UI_SOUNDCNT_A_RIGHT 0x0100
#define UI_SOUNDCNT_A_LEFT  0x0200
#define UI_SOUNDCNT_A_TIMER0 0x0000
#define UI_SOUNDCNT_A_RESET 0x0800
#define UI_MASTER_ENABLE    0x0080
#define UI_AUDIO_BUFFER_SIZE 0x2000
#define UI_AUDIO_MAX_SAMPLES 65520

typedef enum
{
    UI_SFX_NONE = 0,
    UI_SFX_ACCEPT,
    UI_SFX_BACK,
    UI_SFX_MENU,
    UI_SFX_MOVE,
    UI_SFX_STARTUP,
    UI_SFX_TAB,
    UI_SFX_LAUNCH,
    UI_SFX_EJECT
} UI_SFX_ID;

static u16 UIAudio_TimerReload(u32 sample_rate)
{
    if(sample_rate == 0) sample_rate = 22050;
    return (u16)(65536 - (16777216 / sample_rate));
}

static volatile u8 g_ui_audio_initialised = 0;
static volatile u8 g_ui_audio_active = 0;
static volatile u8 g_ui_audio_uses_shared_buffer = 0;

static void UIAudio_Stop(void);
static void UIAudio_Timer1IRQ(void);

static void UIAudio_StopForSharedBufferUse(void)
{
    if(g_ui_audio_active && g_ui_audio_uses_shared_buffer)
        UIAudio_Stop();
}

static void UIAudio_Update(void);

static void UIAudio_WaitForCurrentClip(u32 max_frames)
{
    u32 frames = 0;

    while(g_ui_audio_active && (frames < max_frames))
    {
        VBlankIntrWait();
        UIAudio_Update();
        frames++;
    }
}

static s8 *UIAudio_GetBuffer(void)
{
    return g_ui_audio_buffer;
}

static s8 *UIAudio_GetSharedLongClipBuffer(void)
{
    return (s8*)pReadCache;
}

static u32 UIAudio_PrepareBuffer(const signed char *data, u32 len, u32 allow_shared_long_clip, s8 **out_buffer)
{
    u32 copy_len;
    u32 padded_len;
    u32 buffer_size;
    s8 *buffer;

    if(out_buffer)
        *out_buffer = 0;

    if(!data || !len)
        return 0;

    if(allow_shared_long_clip && (len > UI_AUDIO_BUFFER_SIZE))
    {
        copy_len = len;
        if(copy_len > UI_AUDIO_MAX_SAMPLES)
            copy_len = UI_AUDIO_MAX_SAMPLES;
        if(out_buffer)
            *out_buffer = (s8*)data;
        return copy_len;
    }

    buffer = UIAudio_GetBuffer();
    buffer_size = UI_AUDIO_BUFFER_SIZE;

    copy_len = len;
    if(copy_len > buffer_size)
        copy_len = buffer_size;
    if(copy_len > UI_AUDIO_MAX_SAMPLES)
        copy_len = UI_AUDIO_MAX_SAMPLES;

    padded_len = (copy_len + 15) & ~15;
    if(padded_len > buffer_size)
        padded_len = buffer_size;

    /* Clear the active transfer, including its final partial FIFO block. */
    if(data != buffer)
    {
        memset(buffer, 0, padded_len);
        memcpy(buffer, data, copy_len);
    }

    if(out_buffer)
        *out_buffer = buffer;

    return padded_len;
}


static void UIAudio_StopHardware(void)
{
    REG_DMA1CNT_UI = 0;
    REG_TM0CNT_H_UI = 0;
    REG_TM1CNT_H_UI = 0;
    REG_DMA1SAD_UI = 0;
    REG_DMA1DAD_UI = 0;
    REG_SOUNDCNT_H_UI |= UI_SOUNDCNT_A_RESET;
    REG_IF_UI = IRQ_TIMER1;
    g_ui_audio_active = 0;
    g_ui_audio_uses_shared_buffer = 0;
}

static void UIAudio_Stop(void)
{
    u16 old_ime = REG_IME;
    REG_IME = 0;
    UIAudio_StopHardware();
    REG_IME = old_ime;
    delay(8);
}

static void UIAudio_StopFromTimer(void)
{
    UIAudio_StopHardware();
}

static void UIAudio_Timer1IRQ(void)
{
    UIAudio_StopFromTimer();
}

static void UIAudio_Update(void)
{
    (void)g_ui_audio_active;
}

void UIAudio_Init(void)
{
    u16 old_ime = REG_IME;
    REG_IME = 0;
    REG_DMA1CNT_UI = 0;
    REG_TM0CNT_H_UI = 0;
    REG_TM1CNT_H_UI = 0;
    REG_IF_UI = IRQ_TIMER1;
    REG_SOUNDCNT_X_UI = 0;
    REG_SOUNDCNT_L_UI = 0;
    REG_SOUNDCNT_H_UI = 0;
    REG_SOUNDBIAS_UI = 0x0200;
    delay(256);
    REG_SOUNDCNT_X_UI = UI_MASTER_ENABLE;
    delay(64);
    REG_SOUNDCNT_H_UI =
        UI_SOUNDCNT_A_RIGHT |
        UI_SOUNDCNT_A_LEFT  |
        UI_SOUNDCNT_A_TIMER0 |
        UI_SOUNDCNT_A_RESET |
        (1 << 2);  // 100% volume

    g_ui_audio_active = 0;
    g_ui_audio_uses_shared_buffer = 0;

    irqSet(IRQ_TIMER1, UIAudio_Timer1IRQ);
    irqEnable(IRQ_TIMER1);

    g_ui_audio_initialised = 1;
    REG_IME = old_ime;
}

static void UIAudio_PlayRaw(const signed char *data, u32 len, u32 sample_rate, u32 allow_shared_long_clip)
{
    u32 sample_count;
    u32 copy_len;
    u32 padded_len;
    s8 *play_buffer;
    u16 old_ime;

    if(!g_ui_audio_initialised || !data || !len)
        return;

    /* Stop the previous FIFO stream before touching the bounce buffer.
       Otherwise DMA can read the buffer while it is being memset/memcpy'd,
       which is the classic source of random clipped UI sounds. */
    UIAudio_Update();
    if(g_ui_audio_active)
        UIAudio_Stop();

    copy_len = UIAudio_PrepareBuffer(data, len, allow_shared_long_clip, &play_buffer);
    if(copy_len == 0 || !play_buffer)
        return;

    if(allow_shared_long_clip && (play_buffer == (s8*)data))
        padded_len = copy_len;
    else
        padded_len = (copy_len + 15) & ~15;
    sample_count = padded_len;
    if(sample_count == 0)
        return;
    if(sample_count > 65535)
        sample_count = 65535;

    old_ime = REG_IME;
    REG_IME = 0;
    UIAudio_StopHardware();
    REG_SOUNDBIAS_UI = 0x0200;
    REG_DMA1SAD_UI = (u32)play_buffer;
    REG_DMA1DAD_UI = (u32)&REG_FIFO_A_UI;
    REG_DMA1CNT_UI = UI_DMA_COUNT_4 | UI_DMA_ENABLE | UI_DMA_TIMING_FIFO | UI_DMA_32BIT | UI_DMA_REPEAT | UI_DMA_DST_FIXED | UI_DMA_SRC_INC;

    REG_TM0CNT_L_UI = UIAudio_TimerReload(sample_rate);
    REG_TM1CNT_H_UI = 0;
    REG_TM1CNT_L_UI = (u16)(0x10000 - sample_count);
    REG_IF_UI = IRQ_TIMER1;
    g_ui_audio_active = 1;
    g_ui_audio_uses_shared_buffer = allow_shared_long_clip && (play_buffer == UIAudio_GetSharedLongClipBuffer());
    /* Timer 1 counts Timer 0 sample ticks, so playback stops on an exact sample
       boundary rather than an independently approximated wall-clock duration. */
    REG_TM1CNT_H_UI = UI_TIMER_ENABLE | UI_TIMER_COUNT_UP | UI_TIMER_IRQ;
    REG_TM0CNT_H_UI = UI_TIMER_ENABLE | UI_TIMER_FREQ_1;
    REG_IME = old_ime;
}

/* The legacy PogoShell LZ77UnCompWram symbol is not a libgba decoder.
   Use the WRAM BIOS service directly without changing plugin/reset code. */
#include "slot_sounds.h"
static void UIAudio_PlaySfx(UI_SFX_ID id)
{
    if(!launcher_sounds_enabled)return;
    /* The original Slot insert/eject clips; no navigation/menu beeps. */
    if(id==UI_SFX_LAUNCH)UIAudio_PlayRaw(slot_insert_pcm,SLOT_INSERT_SAMPLES,SLOT_SOUND_HZ,0);
    else if(id==UI_SFX_EJECT)UIAudio_PlayRaw(slot_eject_pcm,SLOT_EJECT_SAMPLES,SLOT_SOUND_HZ,0);
}
