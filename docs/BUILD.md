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
```

The Windows host harness uses TinyCC and a disposable mock SD card. The preview is a normal GBA ROM with fictional labels, built from the shared renderer. Neither substitutes for testing the physical flashcart, its SD bus, save write-back or game compatibility. Exact results and memory sizes are in [validation/REPORT.md](../validation/REPORT.md).

## Artwork sources and packaging

`tools/generate_assets.py` regenerates reduced built-in artwork. `tools/make_examples.py` regenerates fictional examples and preview labels. These use Pillow. Regenerating the original simple mould masks also requires the build-only `resvg-py` dependency and `tools/generate_simple_moulds.py`; the resulting masks are already supplied.

`tools/package.py` verifies the built kernel and converter hashes, then writes the two minimal installation ZIPs under `dist/`. Each contains `SD_CARD`, `Label Converter.exe` and `README.txt`. Generated demo artwork and emulator captures are local test output and are not committed or packaged.

The release version is `v1.0`, defined in [common/slot_version.h](../common/slot_version.h). Upstream sources are pinned in [UPSTREAM.json](../UPSTREAM.json); backend deltas are in `patches/`.

Use local Git for source commits and pushes, and GitHub CLI (`gh release upload` / `gh release edit`) for releases. Browser-editor uploads are unnecessary.
