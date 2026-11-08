# Offline GB/GBC return test

Published v1.1 is unchanged. This local revision adds Start+Select for the GB/GBC emulator menu, a three-second Select-hold exit, and matching GB/GBC cartridge ejection after returning to the kernel. The Select hold setting applies to both native GBA Addons and GB/GBC games. Emulator Restart remains inside the emulator.

Validation passed:

- 44 full-emulator assertions per cartridge build (88 total), using self-authored GB/GBC battery-save ROMs: menu/release controls, short Select presses, hold/Exit, saved-data round trips, Restart, return markers and IWRAM canaries.
- 2,513 controls checks, including the physical-VBlank threshold, fast-forward repeated samples, chord priority and clock wrap.
- 2,632 actual frontend host checks, including GB/GBC shelf restoration and exact reversed insertion frames for all three systems.
- 71 one-shot return IO checks and 1,014 native addon checks.
- Five updater-header checks and four real-CRT warm/cold startup checks.
- Both kernels and embedded emulators pass build-size and memory guards. Packages pass integrity and minimal-content checks; updater headers and the converter match published v1.1.

The full-emulator fixture replaces cartridge SRAM banking and the final reboot. Physical Omega/Omega DE return, SD save write-back, sound and cold-start behaviour still require testing on a backed-up card. Save states, cheats and game compatibility need per-game hardware coverage beyond these battery-save fixtures.
