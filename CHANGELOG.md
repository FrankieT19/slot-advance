# v1.1

- Reworked the GBA in-game menu with the DS Style font and a cleaner layout.
- Consistent Start + Select menu and three-second Select-hold return controls across GBA Addon, GB and GBC games.
- Cartridge ejection with slot.'s animation and sound when returning from a game.
- GB/GBC games run without SGB borders and fade smoothly through black when exiting.
- Removed background settings writes that could stall carousel navigation.

# v1.0

First public release of **slot. advance**: a port of slot.'s cartridge frontend for EZ-FLASH Omega and the original Omega Definitive Edition, built on DS Style.

- Separate GBA, GB and GBC carousels with cartridge labels, shell colours and insertion animation.
- Wood, Dark and Light themes, with realistic or simple cartridge designs.
- Continuous carousel navigation, streamed artwork, native pixel text and a clock.
- Settings, About and Help, including the native GBA Addon reset and in-game menu controls.
- Optional ten-character username from `/USERNAME.TXT`, placed opposite the clock.
- Standalone Windows artwork converter compatible with slot. Cart Studio labels and cartridge colours.
- Model-specific installation ZIPs containing the kernel, SD-card folder layout and converter.

The original Omega and original Omega DE Rev A / Rev B are supported targets. The newer DE B / EZODE-B is not supported. See the README and validation report for setup and remaining physical-hardware verification.
