# v1.2.1 validation

The v1.2.1 ZIPs contain the locally tested clock toggle. The host harness checks settings persistence and compatibility with older settings files. In mGBA, the clock disappeared and returned without changing the username. The GBA return and launch changes were checked for v1.2. The maintainer previously confirmed the GB/GBC exit fade on original Omega DE hardware for v1.1.

## Build limits

| Target | Kernel bytes | Headroom below 2 MiB | Static EWRAM | IWRAM headroom to user stack |
| --- | ---: | ---: | ---: | ---: |
| omega | 1,752,724 | 344,428 | 249,912 | 10,624 |
| omega-de | 1,752,844 | 344,308 | 249,912 | 10,216 |

Built with devkitARM GCC 16.1.0 for ARM7TDMI. The native addon occupies 6,432 bytes of its 8 KiB patch window. Both kernels pass ROM, EWRAM and IWRAM guards. Embedded emulator IWRAM headroom is 400 bytes for Omega and 280 bytes for Omega DE. The emulator fade keeps normal display IRQ handling active until black.

## Checks

- **268 full-emulator assertions:** GB/GBC battery-save fixtures across both cartridge builds with the emulated LCD on and off; controls, save round trips, Restart, SGB border suppression, active scanline DMA and fade frames. Starting exit at scanline 80 reproduced the previous raster fault; the corrected code produces no striped frames and the menu fades through eight decreasing brightness levels.
- **Six reset/transition checks:** production copied reset assembly, BIOS calls and the real black-entry CRT, with captured fade-out, black handoff and fade-in frames.
- **2,671 host frontend checks** cover the clock toggle, shelf restoration, settings faults, cache streaming, reversed insertion frames, and the handoff from insertion to the loading icon before settings writes.
- **1,024 native addon checks** cover the in-game menu, warm return marker, and fade to and from black through both return controls. The Select-hold test uses a display window and active scanline DMA.
- Frame captures verify that the loading icon appears on the ninth VBlank after activation on all three shelves.
- **2,513 emulator-controls checks:** 180 physical VBlank hold threshold, release gates, menu priority, disabled hold, repeated fast-forward samples and clock wrap.
- **71 return-context checks:** one-shot consumption, correct systems, cold-boot rejection, damaged records/options, safe filenames and SRAM banking.
- **Four real-startup checks and five updater-header tests:** warm-cookie preservation through CRT RAM clearing, cold-reset rejection, unchanged updater header and guarded entry hooks.
- Both ZIPs pass integrity and minimal-content checks. The converter is unchanged from v1.0.

See [GB/GBC validation](GB_GBC_TEST.md) for the raster regression and fixture limits. Host/emulator tests do not reproduce the physical FPGA/SD bus or establish every game's save-state, sleep and cheat compatibility. Earlier converter, motion, audio and save-recovery reports remain alongside these results.

## Kernel SHA-256

`ezkernel.bin`

`7fb27d58fca1923604b9df00dfad4a0c3a1f65c8295e496fc41595e327338060`

`ezkernelnew.bin`

`a61c45d189d0b6c30db2ba3eaca147fda5018bbbdaa9b59d4c5dd689b888de24`
