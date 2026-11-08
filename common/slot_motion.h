#ifndef SLOT_MOTION_H
#define SLOT_MOTION_H
/* Slot's quintic ease, fixed-point 0..256. Used once per insertion frame. */
static unsigned Slot_Smooth(unsigned t)
{
    unsigned long long u=t>256?256:t;
    return (unsigned)(u*u*u*(655360ULL-3840ULL*u+6ULL*u*u)/4294967296ULL);
}
#endif
