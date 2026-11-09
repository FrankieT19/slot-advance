# Building slot. advance

The normal downloads contain everything an end user needs. Start with the [installation and artwork guide](../README.md).

## Kernel

Install devkitPro's `gba-dev` toolchain and Python 3.10 or newer. From the repository root:

```powershell
python tools/build.py --target all --devkitpro C:/devkitPro
```

On Linux, use `--devkitpro /opt/devkitpro`. The builder keeps the Omega and original Omega DE backends separate, preserves the compatible loader header, and checks the 2 MiB kernel envelope and RAM limits. Output is under `release/omega` and `release/omega-de`.

The hardware-required kernel names remain `ezkernel.bin` and `ezkernelnew.bin`. Internal `SlotOmega=3` settings and `/SYSTEM/SLOT` artwork paths remain compatible with earlier test builds.

## Artwork converter

On Windows, use the standalone application bundled in each model's release ZIP. To run the source on another platform:

```sh
python -m pip install -r requirements.txt
python tools/converter_app.py
```

The command-line converter also accepts a Cart Studio export directly:

```sh
python tools/convert_labels.py --help
```

`tools/build_converter_app.py` uses PyInstaller to bundle the Windows application, its Python/Tk/Pillow runtime, artwork and licences. Its `--help` documents optional bundler and Tcl paths. No Python installation is required by users of the frozen executable.

## Preview and checks

```sh
python tools/make_examples.py
python tools/build_preview.py
python tools/test_preview.py --core path/to/mgba_libretro.dll
python tools/test_motion.py --core path/to/mgba_libretro.dll
python tests/test_boot_entry.py
python tests/test_converter.py
python tools/test_frontend_host.py --tcc path/to/tcc
python tools/test_navigation.py --tcc path/to/tcc
python tools/test_startup_save.py --tcc path/to/tcc
python tools/test_return_io.py --tcc path/to/tcc
python tools/test_addon_menu.py --core path/to/mgba_libretro.dll
python tools/test_boot_return.py --core path/to/mgba_libretro.dll
python tools/test_emulator_controls.py --tcc path/to/tcc
```

The Windows host harness uses TinyCC and a disposable mock SD card. The preview is a normal GBA ROM with fictional labels, built from the shared renderer. Neither substitutes for testing the physical flashcart, its SD bus, save write-back or game compatibility. Exact results and memory sizes are in [validation/REPORT.md](../validation/REPORT.md).

## Artwork sources and packaging

`tools/generate_assets.py` regenerates reduced built-in artwork. `tools/make_examples.py` regenerates fictional examples and preview labels. These use Pillow. Regenerating the original simple mould masks also requires the build-only `resvg-py` dependency and `tools/generate_simple_moulds.py`; the resulting masks are already supplied.

`tools/package.py` verifies the built kernel and converter hashes, then writes the two minimal installation ZIPs under `dist/`. Each contains `SD_CARD`, `Label Converter.exe` and `README.txt`. Generated demo artwork and emulator captures are local test output and are not committed or packaged.

The displayed version is defined in [common/slot_version.h](../common/slot_version.h). Upstream sources are pinned in [UPSTREAM.json](../UPSTREAM.json); backend deltas are in `patches/`.

## Rebuild the GB/GBC emulator

The generated `goomba.h` in each target includes Jagoomba Color v0.5 with the slot. advance controls and return hook. Normal kernel builds use those supplied headers. To regenerate them, check out `EvilJagaGenius/jagoombacolor` at commit `2a77825e04b7fcef968c2e5f97cb238296a1ebb5`, then run:

```sh
python tools/build_emulator.py --source path/to/jagoombacolor
```

The tool applies the integration to a build-directory copy of the pinned source. The changes are explicit in `tools/build_emulator.py`; the new control and reset code is under `common/slot_emulator_*` and `common/slot_jagoomba.c`. Existing emulator CPU, mapper, menu and save handling remain upstream-based.

For full-emulator checks with self-authored GB/GBC save-test ROMs:

```sh
python tools/build_emulator.py --source path/to/jagoombacolor --target omega-de --test --build-root work/emulator-test
python tools/test_emulator.py --core path/to/mgba_libretro.dll
python tools/test_emulator.py --lcd-off --core path/to/mgba_libretro.dll
python tools/test_emulator_reset.py --core path/to/mgba_libretro.dll
```

Only cartridge SRAM bank access and the final reboot are substituted in that fixture. Physical-cartridge save write-back, return and cold-boot behaviour still require hardware testing.

The reset test separately executes the production reset assembly, BIOS calls and black-entry CRT, checking the captured fade-out/black/fade-in frames. GB/GBC border drawing is suppressed while SGB game palettes remain available; the obsolete border toggle is removed from the emulator menu.

The raster regression starts exit at scanline 80 with HBlank DMA running. It reproduced alternating black/white rows when the old fade disabled IRQs. The new path keeps normal VBlank/VCount display handling active until fully black, then stops DMA and resets. Reports include intermediate menu brightness and striped-frame counts.

Use local Git for source commits and pushes, and GitHub CLI (`gh release upload` / `gh release edit`) for releases. Browser-editor uploads are unnecessary.
