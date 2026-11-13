# slot. advance

slot. advance is a port of [slot.](https://github.com/slot-cfw/slot) for the GBA, running on the **EZ-FLASH Omega** and the original **Omega Definitive Edition**, using [DS Style](https://github.com/FrankieT19/omega-de-ds-style-kernel) as its underlying framework.

Cartridge carousels for GBA, GB and GBC, with label artwork, cartridge colours, and Wood, Dark and Light themes.

## Install

Download **v1.2.3** for your cartridge.

| Cartridge | Download |
| --- | --- |
| Original Omega | [Download](https://github.com/FrankieT19/slot-advance/releases/download/v1.2.3/slot-advance-v1.2.3-omega.zip) |
| Original Omega DE, Rev A / Rev B | [Download](https://github.com/FrankieT19/slot-advance/releases/download/v1.2.3/slot-advance-v1.2.3-omega-de.zip) |

The newer DE B / EZODE-B is not supported.

1. Back up your SD card and saves.
2. Copy the **contents of `SD_CARD`** to the root of your card.
3. Put games in `Games/GBA`, `Games/GB` and `Games/GBC`.
4. Hold **R** while powering on and follow the kernel updater.

Keep the supplied kernel filename: `ezkernel.bin` for Omega, `ezkernelnew.bin` for Omega DE. When updating, keep your existing saves and `USERNAME.TXT`. Saves belong in `SYSTEM/SAVER`.

## Artwork

1. Open [slot. Cart Studio](https://studio.slot-cfw.fyi/) in Chrome or Edge. Click **Get Started**, then **Choose SD Card**. Select the root of the card, where `Games` and `ezkernel.bin` (Omega) or `ezkernelnew.bin` (Omega DE) are located. Allow file editing if prompted.
2. Let Cart Studio look up labels. Choose **Real Label** or **Logo Only** if prompted, and click any cart to change its label or shell colour.
3. Click **Write to card**. Cart Studio converts the artwork for slot. advance and saves it to `SYSTEM/SLOT`.
4. Safely eject the SD Card and place it back into your flashcart.

If you select **Download for slot. advance** instead, extract `labels.zip` and merge its `Labels` and `SYSTEM/SLOT` folders into the matching locations on your SD card.

**Fallback.** If Cart Studio doesn't detect slot. advance and gives you incompatible labels, leave the card connected, open the included **Label Converter.exe** on Windows, select the card, and click **Convert labels and shells**.

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

In **GBA Addon, GB and GBC games**, **Start + Select** opens the in-game menu by default. Hold **Select for three seconds** to return to the carousel and eject the cartridge. Use **Settings → In-game hotkeys** to change the return button or the two menu buttons. **Hold to return** toggles the hold shortcut, and **Save states** enables save/load in the GBA Addon menu. **Settings → Help** shows your current controls.

GBA Addon, GB and GBC games fade through black on return. GB/GBC games run without SGB borders, and **Exit** in the emulator menu also returns and ejects the cartridge.

To hide the footer clock, set **Settings → Show clock** to **Off**.

## Showcase

[Watch the v1.0 showcase on YouTube](https://www.youtube.com/watch?v=XQxO3N3EVzI).

Since v1.0, slot. advance has added cartridge ejection with animation and sound when returning to the shelf, a redesigned GBA in-game menu, matching in-game shortcuts across GBA Addon, GB and GBC, and the option to hide the clock.

## Discussion

[Discuss slot. advance on GBAtemp](https://gbatemp.net/threads/slot-advance-a-modern-frontend-for-the-game-boy-advance.686336/).

## Credits

slot. design by [Brandon Kowalski and contributors](https://github.com/slot-cfw/slot), with cartridge artwork by [Markus](https://github.com/MarkusMaribu). GBA port and DS Style by [frankiet19](https://github.com/FrankieT19), built on the EZ-FLASH kernels and SimpleLight.

[Source build instructions](docs/BUILD.md) · [Licence](LICENSE) · [Notices](NOTICE.md)
