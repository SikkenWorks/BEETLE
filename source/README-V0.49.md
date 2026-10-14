# BEETLE V0.49 · Fast Project Workflow

Based on the **complete V0.48 source**. All previous audio, tracker, animated boot, cursor and six-lane indicator functionality remains included.

## Changes

1. **LOAD → unsaved confirmation → DISCARD → full project picker**. DISCARD creates a clean session and opens the 12-slot browser. It never loads the currently highlighted slot implicitly. Selecting a slot in the browser then loads it with full project CRC validation.
2. **Faster project browser**. Only the two 32-byte A/B bank headers and at most 36 bytes from each selected bank are read for a *provisional* title preview (up to 136 bytes per slot, rather than scanning ~24 KiB plus complete decode). One selected slot and one other are processed per poll. Full reads/CRC/ownership checks are still used for actual LOAD and SAVE. No flash writes are allowed from a preview.
3. **SAVE and SAVE AS respond visibly before flash operations** with a reading/writing status screen. Flash operations are synchronous for integrity; interruptions/power-off during a write are still unsafe.
4. **Successful save returns directly to the tracker** with an approximately 1.3-second `PROJECT SAVED` confirmation, without needing SEQ. Errors still show an explicit failure page. First-time storage provisioning retains the five-second hardware confirmation and returns to tracker only if the save succeeds.
5. **Saving the currently opened project** skips the redundant overwrite dialog and unnecessary full project decode for checking slot occupancy. SAVE AS to another occupied slot still requires confirmation; the full journal safety checks remain unchanged.
6. Browser scans now reuse the boot-time flash ownership audit instead of reading the marker sector repeatedly.

## Tests and limitations

- Native builds: read-only diagnostic, OTA pilot, experimental BEETLE storage.
- Host tests: prior V0.46-through-V0.35 regression paths, native V0.48 UI checks, V0.49 provisional preview A/B tests and static navigation-path checks.
- OTA installer: simulated routes + cryptographic hash and package checks. **No hardware validation**.
- Project title preview is deliberately unverified. A slot is only trusted after a full CRC-checked load; journal writes still inspect banks and reject foreign data.
- Physical flash erase/program may still take noticeable time. The status screen gives immediate feedback, but this does not imply interruptible/background flash writes.
- **WARNING**: experimental storage requires explicit first-use destructive initialization, may overwrite older ETCH/Felucca user-storage data, and firmware flashing has nonzero brick risk. An independent rescue route remains unproven.

GPL-3.0-only; existing licenses retained.
