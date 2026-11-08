# slot. advance v1.1 — offline test

- Hold Select in a GBA Addon game to return to its cartridge and play slot.'s original eject sound and reverse insertion motion.
- A one-shot SRAM request and checksum-bound volatile RAM cookie distinguish a ROM return from a hard start. Cold starts discard stale SRAM requests.
- The existing in-game menu uses the DS Style 6×12 font, theme colours and the kernel settings selection bar. Resume is first; unavailable save/load rows are greyed out; cheats use one toggle row.

This build stays offline. Test with a backed-up card. Use a GBA game launched from SD with Addons and Select hold enabled. Confirm both the warm return/eject and a hard power-off/on without eject, then test Resume, normal save recovery, save/load states and cheats. The cartridge's loader and physical SRAM/SD behaviour still need hardware verification.
