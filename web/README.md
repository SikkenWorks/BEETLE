> **RC1.02 update:** Reverse key/scale search, center-out startup LED ripple, Project Transfer V3 recovery for corrupted BEETLE-owned slots. See [RC1.02-README.md](../../RC1.02-README.md). Experimental hardware operation is not independently validated.

# BEETLE 1.01 RC1.01 web tools

The complete web tools are also available at the GitHub Pages repository root:

- `WebInstaller.html`: **Project Transfer-compatible** firmware installer (not stock RC1)
- `simulator.html`: RC1.01 simulator with encrypted `.beetle` project import/export
- `BEETLE-Project-Manager.html`: twelve-slot USB-MIDI Project Manager V1.4

These files are synchronized with the corresponding copies in `source/web/`.

For installation, security, storage risks and the protocol, read the top-level
`PROJECT-MANAGER-README.md` and `START-HERE.md`. Web MIDI needs HTTPS or localhost.

## True slot deletion (V2)

The updated `BEETLE-Project-Manager.html` has a DELETE command that erases only the selected slot’s two A/B journal banks; it then verifies the FM1 reports `[ EMPTY ]`. This requires the **new V2 firmware** in the updated `WebInstaller.html`. Project Transfer V1 still supports export/import, but cannot physically delete slots. The currently loaded project on the FM1 must be closed by choosing NEW or loading another project before DELETE. Export a verified backup first; do not remove USB/power during the erase. A power failure during deletion can make the slot unreadable. Real FM1 physical deletion has not been hardware-tested.
