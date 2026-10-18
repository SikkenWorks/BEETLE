# BEETLE 1.01 RC1.0.5 - Project Storage Reset

**This release contains an experimental, unverified-on-hardware destructive flash operation.**

## On the FM1

1. Stop playback. Open **PROJECTS** from the tracker with **HOME**.
2. Turn **K1** to **WIPE ALL STORAGE** and press **SEL**.
3. Dialog 1, **ARE YOU SURE?**: select YES using K1 and press SEL. Default is NO.
4. Dialog 2, **REALLY?**: select YES independently and press SEL. Default is NO.
5. Keep the FM1 powered until erase, verification and marker initialization finish.
6. All **12 project slots** should read **[EMPTY]**. A new blank unsaved session is loaded. No empty project is written to flash.

HOME cancels either dialog. Choosing NO at either stage cancels without writing flash.

## Precisely what gets erased

Only the BEETLE-owned user-project partition on the 1 MiB external NOR: `0x97000–0xDFFFF` inclusive. It contains 24 A/B banks, 12 KiB each (288 KiB), plus the 4 KiB BEETLE marker sector at `0xDF000`.

The app image, bootloader/protected header, OTA staging, unrelated global/autosave sectors and other regions are untouched. Project files in that partition, including any historic ETCH/Felucca data already overwritten when the partition was first provisioned, are destroyed.

**After two independent on-device YES confirmations, BEETLE can explicitly reinitialize its fixed project range even when the BEETLE marker is damaged or unknown.** This intentionally also destroys any previous ETCH/Felucca user data that occupy that range. Unlike normal project deletion, this is destructive takeover, not ownership repair. The physical menu alone sets a dedicated short-lived write authorization flag. Physical read faults, safe mode, update mode and active playback are still rejected. No other flash region is ever addressed. If erasing or verifying a sector fails, the operation stops and may already have destroyed some projects. A full-flash backup and independent recovery path are essential.

Erases proceed bank sector by bank sector, marker last; each sector is read back in full before proceeding. The new marker is programmed only after successful erases. Finally the marker and every slot's empty state are verified. A power loss can result in incomplete storage. This does not establish an independent recovery mechanism.

## What has not changed

Project format and Project Transfer wire version V4 remain unchanged; importing `.beetle`, Project Manager, corrupted-slot recovery, KEYLOCK and Reverse Search, synths/drums, song mode, and LED animation are retained. The compatibility-improved WebInstaller keeps its FM1 CHECK + OTA-loader requirements and normal package hash checks.

## Validation

- JieLi storage + updater, read-only hardware-test, and OTA pilot builds compiled.
- Host NOR-flash fixture tested 12 slots erased, protected boundaries, without physical YES authorization refused, explicitly consented invalid-marker reinitialization succeeds, write lock denied, interrupted erase rejected and retry succeeds.
- Existing project deletion, AES container integrity, and LED regressions passed.
- Chromium simulator smoke test displayed both confirmation dialogs and produced no JavaScript errors.
- `BEETLE-1.01-RC1.0.5-EXPERIMENTAL.fwsc` reproduced byte-for-byte from the audited ETCH V1.37 baseline; both boot header and OTA loader match RC1.0.4.
- Exact embedded installer package SHA-256 matches the standalone `.fwsc`.

**Not verified:** physical FM1 erasure, interrupted-power recovery, complete regression suite, and hardware installation from arbitrary third-party custom firmware. Make a verified full-flash backup and establish independent recovery first. The WebInstaller is not a full-flash backup.
