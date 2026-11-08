# slot. advance v1.0

A port of slot.'s cartridge frontend for EZ-FLASH Omega and the original Omega Definitive Edition, built on DS Style.

Choose the model ZIP for your cartridge. Each includes the kernel/SD-card folders under `SD_CARD`, the standalone Windows artwork converter under `Tools`, and a setup guide. Copy the contents of `SD_CARD` to the card root; keep your saves and the required kernel filename.

Use slot. Cart Studio for labels and cartridge colours, then run the bundled converter on the card. The README walks through the complete workflow. No Python installation is needed for the Windows tool.

- Separate GBA, GB and GBC carousels, cartridge labels/colours and insertion animation.
- Wood, black Dark and Light themes; realistic or simple cartridge designs.
- Native pixel text, clock, optional ten-character username in `/USERNAME.TXT`, and two-page Help.
- Native GBA Addon shortcuts: Select held about three seconds returns to the kernel; Start+Select opens the in-game menu.
- Continuous carousel motion with streamed/prefetched artwork and no loading gate.

**Compatibility:** original Omega or original Omega DE Rev A / Rev B only. The newer blue-PCB DE B / EZODE-B is not supported. Back up the card before updating. Automated/preview checks pass; physical SD timing, saves and game compatibility still require verification on each hardware revision. This pre-release awaits that hardware verification.

No ROMs, personal saves or scraped commercial artwork are bundled. GPL-3.0-or-later with original component notices retained. Source and SHA-256 sums accompany the model ZIPs.
