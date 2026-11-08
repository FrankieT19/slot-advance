# v1.1 validation — 8 November 2026

The maintainer reports the revised v1.1 build now works on the original Omega DE Rev A / Rev B. The published kernels are byte-for-byte identical to that offline revision. Original Omega hardware was not available in this run.

## Build limits

| Target | Kernel bytes | Headroom below 2 MiB | Static EWRAM | IWRAM headroom to user stack |
| --- | ---: | ---: | ---: | ---: |
| omega | 1,751,220 | 345,932 | 249,912 | 4,360 |
| omega-de | 1,751,236 | 345,916 | 249,912 | 3,944 |

Built with devkitARM GCC 16.1.0 for ARM7TDMI. The addon occupies 6,272 bytes of its 8 KiB patch window. The build checks ROM, EWRAM and IWRAM limits, including the reserved warm-return mailbox.

## Checks

- **2,518 host frontend checks:** real frontend with mocked SD/hardware, cache streaming, input, settings faults, and exact reversed insertion frames. The live carousel loop performs no background settings writes. Across 750 steady streaming frames, no visible cache misses occurred in the mock-card simulation.
- **1,006 addon emulator checks:** actual menu assembly, native font, selection, disabled state rows, cheats toggle and restored game video. VRAM sentinels confirm navigation leaves unrelated pixels untouched.
- **63 return-context checks:** one-shot consumption, retained SRAM after cold boot, damaged records, filename validation and bank restoration.
- **Four real-startup emulator checks:** the actual devkitARM CRT preserves the warm-return cookie while clearing ordinary work RAM; cold boot and emulator hard reset reject the cookie.
- **Five boot-header tests:** unchanged updater entry/header structure, valid GBA checksum and guarded startup hook.
- Both ZIPs pass integrity checks and contain only README.txt, Label Converter.exe, the model-specific kernel, empty USERNAME.TXT and empty SD-card folders. Kernel hashes match the hardware-tested offline revision.

The converter and its assets are unchanged from v1.0. Earlier converter, motion, audio and save-recovery results remain in the adjacent reports. Emulator and host tests do not reproduce the physical FPGA/SD bus or establish compatibility with every game's save-state, sleep and cheat behaviour.

## Kernel SHA-256

`ezkernel.bin`

`d9876e19cbf789853e894185506a3aa014b06bd1f788ee179922311ce154bd70`

`ezkernelnew.bin`

`89f66ef494a5860b13413976a5e66c7c67c41314a8af57338d10b95b65363332`

