# slot. advance

slot. advance is a port of [slot.](https://github.com/slot-cfw/slot) for the GBA, running on the **EZ-FLASH Omega** and the original **Omega Definitive Edition**, using [DS Style](https://github.com/FrankieT19/omega-de-ds-style-kernel) as its underlying framework.

Cartridge carousels for GBA, GB and GBC, with label artwork, cartridge colours, and Wood, Dark and Light themes.

## Install

Download **v1.2.2** for your cartridge.

| Cartridge | Download |
| --- | --- |
| Original Omega | [Download](https://github.com/FrankieT19/slot-advance/releases/download/v1.2.2/slot-advance-v1.2.2-omega.zip) |
| Original Omega DE, Rev A / Rev B | [Download](https://github.com/FrankieT19/slot-advance/releases/download/v1.2.2/slot-advance-v1.2.2-omega-de.zip) |

The newer DE B / EZODE-B is not supported.

1. Back up your SD card and saves.
2. Copy the **contents of `SD_CARD`** to the root of your card.
3. Put games in `Games/GBA`, `Games/GB` and `Games/GBC`.
4. Hold **R** while powering on and follow the kernel updater.

Keep the supplied kernel filename: `ezkernel.bin` for Omega, `ezkernelnew.bin` for Omega DE. When updating, keep your existing saves and `USERNAME.TXT`. Saves belong in `SYSTEM/SAVER`.

## Artwork

Scrape labels and choose cartridge colours with [slot. Cart Studio](https://studio.slot-cfw.fyi/). Then run the included **Label Converter.exe** on your PC, select your SD card and convert.

You can leave the SD card connected after Cart Studio, even when it tells you to eject it. Run the converter next.

## Your name

Edit the included **`USERNAME.TXT`** on your card. Its first line displays up to **10 characters** opposite the clock. Leave it empty for a blank footer.

## Controls

| Button | Action |
| --- | --- |
| Left / Right | Choose a game; hold to scroll |
| L / R | Change system |
| A | Play |
| B | GBA boot options / return |
| Start | Settings |

Open **Settings → Help** for all controls. In **GBA Addon, GB and GBC games**, press **Start + Select** for the in-game menu, or hold **Select for three seconds** to return to the carousel and eject the cartridge. Enable **Select hold** in Settings; **Save states** enables save/load in the GBA Addon menu.

GBA Addon, GB and GBC games fade through black on return. GB/GBC games run without SGB borders, and **Exit** in the emulator menu also returns and ejects the cartridge.

To hide the footer clock, set **Settings → Show clock** to **Off**.

## Credits

slot. design by [Brandon Kowalski and contributors](https://github.com/slot-cfw/slot), with cartridge artwork by [Markus](https://github.com/MarkusMaribu). GBA port and DS Style by [frankiet19](https://github.com/FrankieT19), built on the EZ-FLASH kernels and SimpleLight.

[Source build instructions](docs/BUILD.md) · [Licence](LICENSE) · [Notices](NOTICE.md)
