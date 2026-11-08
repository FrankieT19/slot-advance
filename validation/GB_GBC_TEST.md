# GB/GBC return and fade validation

Version 1.1 includes Start+Select for the GB/GBC emulator menu, a three-second Select-hold exit, and matching GB/GBC cartridge ejection after returning to the kernel. The Select hold setting applies to both native GBA Addons and GB/GBC games. Emulator Restart remains inside the emulator. The maintainer confirmed the final fade fix on Omega DE hardware; original Omega hardware was not available in this run.

This revision disables SGB borders on every launch, removes the unused border toggle and masks border flags restored by older save states. Game palettes remain supported. Exits fade the existing display to black and restore black immediately after the register-only BIOS reset, during VBlank, before entering the kernel's existing fade-in/eject path.

The previous fade disabled IRQs while HBlank DMA could still be active. Without the VCount stop and VBlank rewind, DMA consumed data beyond its frame buffers and wrote alternating forced-blank rows. An active-LCD fixture starting the fade at scanline 80 reproduced the exact black/white pattern in the previous code. The corrected fade keeps the display IRQ pipeline alive, latches brightness at VBlank and stops DMA only after reaching full black.

Validation passed:

- 67 full-emulator assertions per cartridge/LCD-mode combination (268 total), using self-authored GB/GBC battery-save ROMs with LCD enabled and disabled: menu/release controls, short Select presses, hold/Exit, saved-data round trips, Restart, return markers, IWRAM canaries, SGB packets, stale border masks, active raster DMA and intermediate fade frames. Both builds produce zero striped frames, and the visible menu fades through eight decreasing brightness levels.
- Six reset/transition checks execute the production copied reset stub, BIOS calls and CRT; captured frames fade out, remain black during handoff and fade in without a white frame in mGBA.
- 2,513 controls checks, including the physical-VBlank threshold, fast-forward repeated samples, chord priority and clock wrap.
- 2,632 actual frontend host checks, including GB/GBC shelf restoration and exact reversed insertion frames for all three systems.
- 71 one-shot return IO checks and 1,014 native addon checks.
- Five updater-header checks and four real-CRT warm/cold startup checks.
- Both kernels and embedded emulators pass build-size and memory guards. Packages pass integrity and minimal-content checks; updater headers and the converter match published v1.1.

The full-emulator fixture replaces cartridge SRAM banking and the final reboot. Physical Omega/Omega DE return, SD save write-back, sound and cold-start behaviour still require testing on a backed-up card. Save states, cheats and game compatibility need per-game hardware coverage beyond these battery-save fixtures.
