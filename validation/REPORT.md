# Validation — 8 November 2026

Status: **hardware-test candidate**. No physical cartridge was available to this run.

## Builds

| Target | Kernel bytes | 2 MiB envelope headroom | Static EWRAM | Static IWRAM | IWRAM headroom to user stack |
| --- | ---: | ---: | ---: | ---: | ---: |
| omega | 1,741,276 | 355,876 | 249,912 | 26,328 | 6,184 |
| omega-de | 1,741,172 | 355,980 | 249,912 | 26,736 | 5,776 |

Both builds use devkitARM GCC 16.1.0, ARM7TDMI/Thumb, -Os and section collection. The build enforces the 2 MiB envelope, EWRAM capacity, and an IWRAM stack reserve. The per-game artwork stays on SD and does not change the kernel size.

Compiler output contains three warnings per target in retained legacy DS Style UI functions (two indentation warnings and one bounded snprintf truncation warning). Those source locations were inherited; no warning originated in the new shared frontend.

## Automated checks

- **41 converter integration tests pass.** Actual Cart Studio path/metadata layout; BOM/NFC; case matching; header-driven colours and shapes; transparent/clear/glitter assets; RGB555 channels, padding, transparency key; malformed inputs; original-file preservation; shared GB/GBC save-stem warning, inset one-pixel label corners, existing-BMP migration, repeat in-place GUI conversion and preservation of source data.
- **2459 host frontend checks pass.** The actual shared frontend header is compiled against fault-injected hardware/SD mocks and real converted BMPs. Checks include settings snapshots/backup recovery, short writes, failed rename/read, BMP caching, directory sorting, empty/missing/error states, 512-entry bounds, clean/addon launch delegation and menu input. New checks cover skipping empty shelves, immediate targets, release gates, style persistence, bounded username loading/placement and 36 exact resting-endpoint comparisons plus 288 full-frame comparisons between incremental and complete redraws across three platforms, both designs, both directions and all three backgrounds.
- **3,860 compiled GBA checks pass** in mGBA 0.11-212-7a12d6d: navigation boundaries/wrapping, natural-number overflow, strict BMP header validation, rendering bounds guards, and the actual ARM hold-reset macro (threshold, release, Start exclusion, enabled IRQ flags and flag-register preservation), plus the ARM row blitter and exhaustive BGR555 conversion of all 65,536 inputs, odd lengths and destination guards.
- **29 interactive emulator assertions pass** for all three shelves, wrapping, independent remembered selections, themes, settings, cartridge-back, insertion and empty states. Screenshots come from the compiled GBA renderer, not an HTML mock-up.
- **Standalone Windows app passes.** Frozen executable converts ten labels into thirty BMPs twice with only Windows on PATH. Tagged text has only ink/paper/transparency colours. Originals are verified unchanged. The folder-picker/worker flow was previously exercised through Windows Computer Use; v1.0 converter, one-pixel label corners and original procedural mould assets are verified in the frozen executable.
- **Insert sound verified in the emulator**, peak 24384 in captured PCM. Only the insertion clip is used. Smooth limiting raises offline waveform RMS by 4.75 dB over 0.4; duration remains 240 ms.
- **Motion/input checks pass:** eight rapid taps, direction reversals, steady 200 ms repeats, release behavior, simple-style toggle, About v1.0, both Help pages (without the unused Select line), slot. advance branding and removal of Rescan. The target changes before settling, without a transition queue. A tap including its subpixel spring tail settles in 32 emulated GBA frames (~536 ms); input remains responsive throughout. The unchanged spring, with the slower hold schedule, matches its independent float reference across 84,000 VBlanks, maximum position difference 0.023 GBA pixels. Held navigation shows about 30 changed display images per second on all three shelves in mGBA. Physical SD latency is not represented.
- **Streaming cache checks pass:** 750 steady held-scroll frames across all platforms and both styles have 0 visible cache misses in the mock-card simulation. Partial images stay hidden; cancellation/retry, short/failed reads, malformed and missing art are covered. Real SD latency is not simulated. The ARM colour-conversion benchmark is 4.82x faster than the prior Thumb loop. Scrolling never waits on a renderer acknowledgement.
- **Startup save:** five fault-injected checks compile the actual DE save-recovery function and verify success, absent flag, failed write, failed path and retry. Failed writes keep the pending marker. The normal write does not load artwork or draw the old popup.
- **0.2.1 boot-header regression checks pass.** Four unit tests, valid GBA checksum, and the first 224 bytes of both release kernels matching the working 0.1 headers exactly. The early ARM trampoline executes successfully in mGBA and preserves black startup. This addresses the suspected header cause of the reported no-kernel-found error; physical loader acceptance still needs confirmation.

The preview and host tests do not emulate the actual Omega FPGA or physical SD bus. The host launch stub validates handoff parameters, not successful cartridge boot. Memory headroom is a linker measurement, not a runtime stack high-water measurement.

## Hardware verification still required

- [ ] Boot on physical Omega and Omega DE Rev A / Rev B
- [ ] Physical SD IO and cartridge PSRAM cache behaviour
- [ ] Native GBA launch and GB/GBC emulator launch
- [ ] Save write-back across full power cycles
- [ ] Native GBA addon reset on 180 VBlanks, Start+Select menu, states/sleep and game compatibility
- [ ] Physical LCD animation, sound and input timing; startup/reset display before kernel entry

Black entry applies from the first instruction in the supplied kernel. A firmware loader displayed before it runs remains outside kernel control. Select-hold is a native GBA addon patch, not a clean-boot or GB/GBC emulator hotkey. Games without the expected VBlank IRQ path require separate compatibility testing.

Recommended first pass: use a backed-up card and a disposable/homebrew ROM. Test clean boot, a save/reboot round trip, GB and GBC saves, then addon paths. Confirm the same sequence independently on both hardware families. Check empty shelves, an absent or malformed BMP, long filenames, 512+ files, and animation off/on. A failed or interrupted update must be recovered using the correct known-working kernel for that model.

## SHA-256

`omega/ezkernel.bin`

df4fa396f7bbbc08e86348ddc5eeb9b9ce794b35d2d2ee370465a6b299368c18

`omega-de/ezkernelnew.bin`

7d96b7cd02bb985fb844d802c48ea60a21a589e7fd851f72e83bc73e7f5de10c

`preview/slot-preview.gba`

b123a2777daa7bed69d965ab6dc50dbc0167b5833d080beeb6822eb5b0a3b487
