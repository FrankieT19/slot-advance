# Notices

This combined derivative uses GPL-3.0-or-later to accommodate the Slot artwork and adapted rendering model. Original files retain their existing notices and licences; this does not replace the Apache-2.0 notices in the DS Style/EZ-FLASH sources.

- DS Style: FrankieT19. Original cartridge backends: EZ-FLASH, with SimpleLight work by Sterophonick and the contributors credited in `kernels/*/UPSTREAM_README.md`.
- Slot: Brandon Kowalski and contributors, GPL-3.0-or-later. Cartridge artwork: Markus (MarkusMaribu). Slot’s original boot wordmark and insertion PCM are included; the insertion clip is resampled by `tools/convert_sounds.py`. The original eject clip is also resampled for the v1.1 return animation. Copied visual assets live in `assets/slot/`; reduced assets are generated into `common/slot_assets.h`.
- The offline converter's shell palette, header exceptions, overlay model and format interpretation are adapted from Slot's `cart.rs`, `shell.rs`, `bitmaps.rs` and `cart_shell.rs`. Slot copyright and GPL obligations apply to that adaptation.
- Label font: licence in `assets/slot/label.ttf.OFL.txt`.
- DejaVu font: licence in `assets/slot/DejaVuSansMono.ttf.LICENSE.txt`.
- FatFs: ChaN; see notices in the inherited source.
- The test tools use separately obtained devkitPro, mGBA and TinyCC. Those tools are not bundled in this package.

Modified DS Style backend files in each target: `source/ezkernelnew.c`, `source/GBApatch.c`, `source/GBApatch.h`, `source/gba_rts_patch.s`, `source/launch_indicator.h`, `source/launch_indicator_anchor.h`, `source/menu_fade.h`, and `Makefile`; additionally `source/Ezcard_OP.c` in the DE target. Corresponding patches are included. The launcher loop is replaced by the shared frontend. Selected-game loading, save handling, hardware operations and emulators remain based on upstream code. Addon changes add Select-hold reset, fix the menu chord to Start + Select, and namespace the patch cache. The new black entry trampoline and shared animation/audio helpers are included as source. The exact DS Style 6 × 12 bitmap font is reused in the kernel and generated labels.

The standalone Windows converter also contains Python, Tcl/Tk, Pillow, and the PyInstaller bootloader. Their licences and the PyInstaller bootloader redistribution exception are included in `app/licenses/`. PyInstaller itself and development-only tools are not required on an end user’s computer.

All source needed to build the supplied experimental kernels is included. Original DS Style readmes, Apache licence texts, this notice and GPL licence text must remain with redistributed source. Scraped labels are user-supplied data and are not relicensed by conversion; follow the source artwork terms when sharing them.

The 0.3 refinement adapts Slot’s `slot_chrome.rs` travel/catch/creep geometry and untextured slot bands, and copies its `gba_cart.png`, `gb_cart.png` and `gbc_cart.png` silhouettes for the simple design. Realistic artwork remains by Markus. `house_label.rs` documents the `slot-label` PNG metadata used to identify text labels for native-font rendering. The new ARM row blitter and coverage renderer are included as corresponding source under `common/`; each kernel has a make-build forwarding file `source/slot_blit_kernel.s`. DE pending-save presentation is changed while its underlying save writer and failed-write retry policy remain in use.

The 0.4 navigation adapts the exact spring and hold schedule in `crates/slot-ui/src/shelf.rs` from slot-cfw/slot commit `0fe8f5cfa63161bdec5dd4680775d5a7ad0b463c` (5 November 2026). The plain cartridge design ports `shell_face`, `mould_detail`, `recess_label` and the silhouette/lettering masks from commit `57ea55f9763c01eb4e3ac12e8b0240ec682dba3f`, before the handmade-art replacement. Original SVGs, lettering PNGs and reference Rust source are in `assets/slot/simple-source/`; `tools/generate_simple_moulds.py` rasterises their masks using the build-only resvg-py dependency. End-user conversion needs only the bundled Pillow runtime. Current label placement and rounded masks are retained at GBA resolution. These procedural designs are by Brandon Kowalski/Slot contributors; realistic artwork remains by Markus.

Version 0.5 retains Slot’s spring equations with a GBA-specific steady 200 ms held repeat. No loading or presentation gate is used. The row-streaming loader, fine-scale pixel rounding and ARM BGR555 decoder are supplied as corresponding source in `common/`. Insertion PCM is resampled and smoothly limited before signed-8 quantization by `tools/convert_sounds.py`.
