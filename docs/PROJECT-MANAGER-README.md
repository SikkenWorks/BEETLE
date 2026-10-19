# BEETLE Project Manager · Project Transfer V2 (V1 import/export backward compatible) (experimental)

This addition is based on **BEETLE 1.01 RC1**. It provides a standalone BEETLE-themed browser interface with twelve project slots and adds a separate, bounded USB-MIDI SysEx protocol to the firmware source.

## Release status

**Software tested, NOT physically validated on an FM1.** The supplied experimental package is a different build from the original RC1. The existing RC1 firmware does not support direct project transfer. This cleaned release contains **only** the Project Transfer build. `WebInstaller.html` is the transfer-enabled installer and embeds the exact same firmware package as the included `.fwsc` (SHA-256 verified). Hardware write safety and recovery remain unproven. Do not deploy to an FM1 containing important data without an independently verified complete backup and recovery route. This tool cannot initialize or format user flash.

## Browser workflow

1. Host `BEETLE-Project-Manager.html` on **HTTPS** (e.g. GitHub Pages) or `localhost`. Desktop Chrome/Edge supports Web MIDI SysEx; grant the requested permission. Plug the FM1 in using a USB data cable.
2. The FM1 must be running a hardware-validated project-transfer-enabled firmware. The current build is an **experimental candidate** pending that validation. Playback must be stopped. If writing into the currently loaded project slot with unsaved edits, first save or discard those edits on the device.
3. Click CONNECT FM1; choose MIDI INPUT and OUTPUT; select a slot after the 12-slot scan.
4. Click EXPORT .BEETLE. The device sends a CRC-verified project; the browser encrypts it automatically (no password) and starts the download. Confirm the file was actually saved.
5. To IMPORT: select target slot, choose IMPORT and open the file. New V2 files open automatically without a password (older V1 files still require the original password). The app decrypts it locally, validates TRK1 magic, schema version, length, slot and CRCs. Empty slots require approval. Existing slots require explicit overwrite approval; the **EXPORT FIRST** option downloads the current project and asks you to confirm you have saved the backup before overwriting.
6. A successful import is acknowledged **only after** the FM1's existing two-bank journal reports its completed verified commit. If communication fails during commit, refresh the slots and verify before repeating. **Never assume a timed-out import failed or succeeded.**

## Encryption and file format

`.beetle` is a binary encrypted container, not renamed plain JSON or a compressed file. Current V2 structure:

- 8 ASCII bytes `BEETLE01`
- 1 byte format version `0x02`
- 12-byte random AES-GCM nonce
- AES-256-GCM ciphertext plus 16-byte authentication tag, protecting the native **TRK1** project record

V2 uses an embedded fixed key shared by the public browser tools. This removes the password prompt and makes the file opaque in text editors, **but it is not secret against someone who inspects the public key**. Old V1 `.beetle` files remain importable with their original password. See `BEETLE-PROJECT-FORMAT-V2.md` for current details. The on-device A/B journal stores ordinary TRK1 records and **is not encrypted**.

## USB project protocol (non-OTA)

Uses the existing Felucca/JieLi SysEx framing (`F0 pack7(00 59 cmd len24 body check) F7`) with a request sequence byte echoed in replies. It never invokes the OTA loader. All commands are bound to a project slot, not a flash offset:

| Command | Action | Reply |
|---|---|---|
| `50` | Protocol/version capabilities | `60` |
| `51` | Read slot title/occupancy preview | `61` |
| `52` | Open verified project export, return size/CRC | `62` |
| `53` | Read up to 24 verified project bytes by offset | `63` |
| `54` | Finish transfer, discard staging | `64` |
| `55` | Begin import to a target slot; validate occupied status | `64` |
| `56` | Stage 24-byte project chunks at exact offsets | `64` |
| `57` | Verify complete CRC + decode, save via A/B journal | `65` |
| `58` | Abort transfer, discard staging | `64` |
| any | Failure | `6F` + original command/error code |

Write safety:
- No browser-provided flash addresses or arbitrary flash commands.
- Requires BEETLE ownership marker; refuses unknown, incomplete and foreign banks.
- Upload is fully staged in RAM; no flash write until the CRC and full decoder validation succeed.
- Uploads cannot bypass native 12 KiB project budget or original journal bank ownership checks.
- The existing bank CRC, two-bank transactional commit, read-back and read-only probes remain in place.
- The imported project's original slot is reassigned to the chosen target; the firmware encoder generates new internal headers and CRCs.
- Transfer operations require STOP and are blocked during boot recovery, audio faults and OTA; an active transfer prevents physical UI edits from racing its shared codec workspace. Staging expires after 120 seconds of inactivity.
- The browser confirms before overwriting an occupied slot; direct USB command clients can explicitly request overwriting without a browser confirmation. As with the firmware's existing project SAVE functionality, this is **not an authenticated control protocol**. Anyone with authorized Web MIDI SysEx access to the connected FM1 can request overwrite. USB physical access/browser permission should be treated as the trust boundary.

## Validation performed

- Three JieLi variants compiled successfully: read-only, OTA pilot, and persistent-storage project-transfer lab.
- Full existing RC1 host regression suite passed.
- Browser UI rendered in Chromium at desktop/mobile widths; no JavaScript errors or horizontal overflow.
- Simulated Web MIDI handshake, listing and slot selection passed.
- Browser encryption tests cover password-free AES-GCM V2 round-trip, tamper detection, V1 legacy compatibility, and 7-bit SysEx frame checksums; see `source/tests/project-manager/test_v2.cjs` and the project-transfer test suite.
- Experimental `.fwsc` built with pinned ETCH 1.37 reference; exact first protected 16 KiB, inherited loader, compiled app and package checks passed.
- **Physical FM1 read/write and destructive rollback tests were NOT performed.** Real USB rate, peripheral behavior, app reliability and recovery are unproven.

GPL-3.0-only BEETLE/Felucca/ETCH lineage remains intact; see source/CREDITS-AND-LICENSES.md.

## V1.1: Password-free V2 and simulator compatibility

- New `.beetle` backups use AES-256-GCM with a random nonce, but need **no password**. They are unreadable in text editors, not secret against someone inspecting BEETLE's public bundled key.
- Legacy password-protected V1 files can still be opened using their original password.
- RC1.0.11 WebSimulator can import compatible historical TRK1 projects, edit them in browser memory and export native V19 encrypted `.beetle` files with SONG CHAINS.
- In the simulator, preexisting JSON backups remain importable. Project Manager accepts native binary projects, not raw JSON.
- Firmware binaries and `.fwsc` are unchanged from the experimental Project Transfer release. No new hardware validation has been performed.

Full wire protocol and format notes: `BEETLE-PROJECT-FORMAT-V2.md`.


## V1.2 · USB diagnostics and clear firmware identification (no flash changes)

This version fixes the *generic* `FM1 did not respond` error screen. The standalone manager first sends the read-only Project Transfer HELLO (`0x50`). If there is no reply, it sends the existing **read-only OTA identity request** (`0x11`) through the selected USB-MIDI ports.

- **PROJECT TRANSFER READY / CONNECTED**: protocol is present and all 12 slots were scanned.
- **FIRMWARE WITHOUT PROJECT TRANSFER**: the FM1 returned a firmware identity, but not a Project Transfer HELLO. This is consistent with original/older BEETLE 1.01 RC1 firmware; USB project transfers require a separately built firmware supporting these commands.
- **NO USB REPLY**: neither request received a reply. Causes can include incorrect MIDI input/output port, USB cable, inaccessible device, or firmware with no applicable support. This cannot identify the installed firmware conclusively.
- **STORAGE DISABLED / UNSUPPORTED FIRMWARE**: the firmware replied but reported incompatible protocol/capability.

Connection diagnostics show the selected MIDI port names, frame counts and a copyable troubleshooting report. Both diagnostic queries are read-only: **no project import, erase, firmware installation or flash writes**.

**Important:** Changing the HTML cannot make stock/standard RC1.01 firmware transfer projects over USB. The existing experimental firmware bundle is **unchanged**; actual FM1 project read/write and device recovery have **not** been hardware-tested here. Do not flash the experimental build merely to dismiss an error without independent full backup and a working recovery route.

Tests: `python source/tests/project_transfer/test_connection_diagnostics.py`, `python source/tests/project_transfer/test_web_browser.py` and `node source/tests/project-manager/test_v2.cjs`. The diagnostics tests emulate healthy, legacy-identity and silent USB-MIDI devices. Physical hardware behaviour is unknown.

## True slot deletion (V2)

The updated `BEETLE-Project-Manager.html` has a DELETE command that erases only the selected slot’s two A/B journal banks; it then verifies the FM1 reports `[ EMPTY ]`. This requires the **new V2 firmware** in the updated `WebInstaller.html`. Project Transfer V1 still supports export/import, but cannot physically delete slots. The currently loaded project on the FM1 must be closed by choosing NEW or loading another project before DELETE. Export a verified backup first; do not remove USB/power during the erase. A power failure during deletion can make the slot unreadable. Real FM1 physical deletion has not been hardware-tested.
