# Repository workflow

- Use local Git and GitHub CLI for commits, pushes, repository checks and release uploads. Do not use browser or computer-control automation for GitHub work.
- Keep the end-user README focused on installation, artwork and controls. Put development instructions in `docs/BUILD.md`.
- Public releases use the displayed version, currently **v1.0**. Do not put private test-build numbers in release titles, tags, filenames or user-facing documentation.
- Preserve the cartridge updater filenames `ezkernel.bin` and `ezkernelnew.bin`, existing save paths, and `/SYSTEM/SLOT` compatibility.
- Keep generated work directories, local credentials, build caches and compiler outputs out of Git. Retain required licenses and notices.
- Verify the remote commit and release assets after pushing. Use normal fast-forward pushes; do not rewrite published history as part of routine updates.
- In the maintainer's local workspace, `../work/slot-advance-cli.ps1` may provide configured Git and GitHub CLI commands. Use it when present; other checkouts can use their installed `git` and `gh` directly.
