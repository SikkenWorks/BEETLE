# BEETLE V1.4.25 Project Manager — sample reconnect hotfix

## Root cause
The old Project Manager displayed only `entries[]`, a PC-side array initialized to thirteen empty WAV entries. Connect scanned the twelve **project** slots but never requested the thirteen **global sample** statuses from the FM1. Its old upload implementation interpreted every empty PC entry as a request to issue native command `0x75 DELETE`, deleting occupied device samples when a user uploaded a different WAV.

## Correction (browser tools only)
- On connection, the Manager requests native command `0x70 SAMPLE INFO` for U01–U13 and displays device occupancy, compressed byte size, CRC and record generation independently of PC data. New samples can be uploaded only after all thirteen reads succeed.
- An empty PC WAV slot **never** triggers a delete. No mass-delete path exists in normal upload. Upload plans explicitly list new and replaced device slots, requiring confirmation for replacements.
- Every upload rechecks each planned slot's device metadata before starting and verifies **all** samples after completion. If a scan fails or an observed generation changes, the operation refuses to write.
- Deletion is separate, per-device-slot, double confirmed, compares slot metadata again, sends the original supported `0x75` command and checks that INFO reports empty. Deleting the PC WAV doesn't touch FM1 flash.
- A dedicated button refreshes all thirteen device statuses. Unknown statuses are visually marked as UNKNOWN, never EMPTY.

## Important native-protocol limitation
`0x70` returns only existence, compressed byte length, CRC32 and storage generation. Firmware V1.4.25 **does not have a sample readback command**. This browser-only fix cannot recover original WAV bytes or read back a custom 3-character FM1 sample label from the device. Such information is still kept in the separate `.beetlebank` and requires original user backups. Do not imply that an FM1-only sample has been downloaded or reconstructed.

## Tests
`python tests/web/test_v1425_sample_inventory_safesync.py` in an environment with Playwright and Chromium, using a mocked MIDI/FM1 device. Verifies reconnect inventory, no implicit deletes, upload U03 while preserving U01/U02, replacement confirmation/cancellation, device-only deletion, incomplete-scan write blocking. All such tests simulate firmware responses; no physical hardware interaction is claimed.

## Deployment
Replace `BEETLE-Project-Manager.html` on GitHub Pages or use the rebuilt `BEETLE-MANAGER.html`. The current V1.4.25 firmware is unchanged; no device flash or storage migration is needed.
