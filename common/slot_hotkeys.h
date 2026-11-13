#ifndef SLOT_HOTKEYS_H
#define SLOT_HOTKEYS_H

/* Active-high GBA KEYINPUT bits, shared by the kernel and the GB emulator. */
#define SLOT_KEY_MASK 0x3ffu
#define SLOT_DEFAULT_RETURN_KEY 4u
#define SLOT_DEFAULT_MENU_KEYS 12u

static inline unsigned Slot_SingleKey(unsigned key)
{
    return key && key<=SLOT_KEY_MASK && !(key&(key-1));
}

static inline unsigned Slot_MenuKeysValid(unsigned keys)
{
    unsigned first=keys&-keys;
    /* A stock GBA D-pad cannot press opposite directions together. */
    return first && first<=SLOT_KEY_MASK && Slot_SingleKey(keys^first) && keys!=0x30 && keys!=0xc0;
}

#endif
