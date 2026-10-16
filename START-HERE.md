# BEETLE 1.01 RC1.01 · Project Manager compatible

**SikkenWorks · Clean project release**

## Files to use

| File | Purpose |
|---|---|
| `WebInstaller.html` | Project Transfer-compatible FM1 installer. **This is the correct installer.** |
| `BEETLE-1.01-RC1.01-TRUE-DELETE-EXPERIMENTAL.fwsc` | The exact `.fwsc` used inside the web installer |
| `BEETLE-Project-Manager.html` | Project Manager V1.4 with 12-slot scan and connection diagnostics |
| `simulator.html` | Full RC1.01 simulator with native encrypted `.beetle` import/export |
| `source/` | Complete RC1.01 firmware source, tools, licences, references and tests |
| `PROJECT-MANAGER-README.md` | USB protocol, import/export, safeguards and compatibility |

Use Chrome/Edge and serve HTML via **HTTPS** (e.g. GitHub Pages) or localhost
for Web MIDI. The project's GitHub Pages links to `WebInstaller.html`,
`simulator.html` and `BEETLE-Project-Manager.html`.

## What was cleaned

- Removed the original RC1 WebInstaller that **did not** install Project Transfer.
- Removed the obsolete standalone non-transfer `.fwsc` and manifest.
- Removed alternate copies of the installer and simulator at repository root.
- Made `source/web/` copies identical to the live web tools.
- Corrected the Project Transfer package manifest (including the old stale SHA value).
- Kept **all firmware source files and tests**, including the native Project Transfer implementation and AES/project codec.

## Firmware integrity

- Firmware SHA-256: `185ec2c198199847325fce7240d94dd13575707336421979b9c67426ca96c541`
- Firmware size: `610086` bytes
- Installer embeds the **exact same bytes** as the `.fwsc` in this project.

## Important limitations

The BEETLE Project Manager communication path has been used by the maintainer,
but hardware-safe flash updates, interrupted project write behaviour and
independent device recovery have not been certified. Any destructive first-use
flash-storage provisioning can destroy ETCH/Felucca data. Keep verified backups.
`.beetle` files use a shared key to conceal data from ordinary text editors;
this is **not** confidential encryption against an attacker who inspects the app.

**No new firmware was flashed or changed during this cleanup**: the existing
transfer-enabled binary was retained byte-for-byte.

## True slot deletion (V2)

The updated `BEETLE-Project-Manager.html` has a DELETE command that erases only the selected slot’s two A/B journal banks; it then verifies the FM1 reports `[ EMPTY ]`. This requires the **new V2 firmware** in the updated `WebInstaller.html`. Project Transfer V1 still supports export/import, but cannot physically delete slots. The currently loaded project on the FM1 must be closed by choosing NEW or loading another project before DELETE. Export a verified backup first; do not remove USB/power during the erase. A power failure during deletion can make the slot unreadable. Real FM1 physical deletion has not been hardware-tested.
