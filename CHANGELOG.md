# v1.1 — offline test

- Add warm-return cartridge ejection using slot.'s original sound and reversed insertion path.
- Keep hard restarts on the normal boot path using a volatile return cookie.
- Restyle the GBA Addon menu with native DS Style text and settings-style selection.
- Separate the addon patch cache and track its theme so existing patched games receive the new menu.

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
