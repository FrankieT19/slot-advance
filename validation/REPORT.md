# v1.1 validation — 8 November 2026

The maintainer confirmed the final GB/GBC exit-fade fix on the original Omega DE Rev A / Rev B. The released kernels match the tested offline build exactly. Original Omega hardware was not available in this run.

## Build limits

| Target | Kernel bytes | Headroom below 2 MiB | Static EWRAM | IWRAM headroom to user stack |
| --- | ---: | ---: | ---: | ---: |
| omega | 1,752,452 | 344,700 | 249,912 | 4,352 |
| omega-de | 1,752,572 | 344,580 | 249,912 | 3,944 |

Built with devkitARM GCC 16.1.0 for ARM7TDMI. The native addon occupies 6,272 bytes of its 8 KiB patch window. Both kernels pass ROM, EWRAM and IWRAM guards. Embedded emulator IWRAM headroom is 400 bytes for Omega and 280 bytes for Omega DE; mapper helpers execute from EWRAM and the fade keeps normal display IRQ handling active until black.

## Checks

- **268 full-emulator assertions:** GB/GBC battery-save fixtures across both cartridge builds with the emulated LCD on and off; controls, save round trips, Restart, SGB border suppression, active scanline DMA and fade frames. Starting exit at scanline 80 reproduced the previous raster fault; the corrected code produces no striped frames and the menu fades through eight decreasing brightness levels.
- **Six reset/transition checks:** production copied reset assembly, BIOS calls and the real black-entry CRT, with captured fade-out, black handoff and fade-in frames.
- **2,632 host frontend checks:** actual frontend with SD/hardware mocks, shelf restoration, input and settings faults, cache streaming and exact reversed insertion frames for GBA, GB and GBC. No background settings writes occur in the live carousel loop; 750 steady streaming frames have no visible mock-card cache misses.
- **1,014 native addon checks:** actual menu/return-marker assembly, native font, dirty-row navigation, resume/video restoration and cheats toggle.
- **2,513 emulator-controls checks:** 180 physical VBlank hold threshold, release gates, menu priority, disabled hold, repeated fast-forward samples and clock wrap.
- **71 return-context checks:** one-shot consumption, correct systems, cold-boot rejection, damaged records/options, safe filenames and SRAM banking.
- **Four real-startup checks and five updater-header tests:** warm-cookie preservation through CRT RAM clearing, cold-reset rejection, unchanged updater header and guarded entry hooks.
- Both ZIPs pass integrity and minimal-content checks. Their kernels are byte-for-byte identical to the maintainer-tested offline revision. The converter is unchanged from v1.0.

See [GB/GBC validation](GB_GBC_TEST.md) for the raster regression and fixture limits. Host/emulator tests do not reproduce the physical FPGA/SD bus or establish every game's save-state, sleep and cheat compatibility. Earlier converter, motion, audio and save-recovery reports remain alongside these results.

## Kernel SHA-256

`ezkernel.bin`

`063ce4616a5058261073155f6ed2ece7aaadbb56b93ccd0f53e8d2a6d96042cc`

`ezkernelnew.bin`

`bf629a0c5faa12c60e460c3fec1460d7d2fb8b2ec2deec870d9310129a6ea668`
