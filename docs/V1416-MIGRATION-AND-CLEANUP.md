# BEETLE V1.4.16 — Source cleanup and safe migration preflight

**Status: offline source/review release. Not installable V1.4.16 firmware.**

## What is implemented

- `tools/storage_v2/migration_preflight.py`: authenticates the existing AES-GCM `.beetle` V2 export format or accepts individually CRC-validated TRK1 `.trk` fixture files; explicitly accounts for **all 12 project slots** (including confirmed empty slots). The program does not infer that a missing file means an empty project.
- Optional U01–U13 inputs must be *encoded and CRC-valid* BEETLE BD4S files. Original WAV files are not treated as flash-ready samples.
- The tool builds an **in-memory** 288 KiB Storage V2 NOR volume, checks four completely erased recovery sectors, simulates reboot and checks that **every project and sample is byte-identical** after reloading. Failure is non-destructive.
- Negative tests cover tampered encrypted backups, corrupted TRK1 checksums, missing/duplicate slots, path traversal, bad sample input, and the rejected overfull case (12 dense projects plus 20s sample).
- Original code, FM1 HAL, project formats, sample codec, immutable generation-pinned reader, playable host DPCM4 tests, and installer checks are retained.

## Run the migration preflight (PC only)

1. Make a separately verified backup of all 12 project slots using the current Project Manager. **Never assume an empty slot.**
2. Copy `docs/migration/MIGRATION-MANIFEST-TEMPLATE.json` to your local backup directory.
3. For each numbered slot, replace `UNCONFIRMED` with either `"status":"empty"` **only after verifying that slot is empty**, or `"status":"file","path":"slot-01.beetle"` (change the filename to the exported backup). For U slots optionally add `{"slot":1,"path":"u01.bd4"}` and other encoded BD4S sources to the samples array. Relative paths only.
4. Run `python tools/storage_v2/migration_preflight.py /path/to/MIGRATION-MANIFEST.json --report /path/to/LOCAL-REPORT.json`.
5. `OFFLINE_PREFLIGHT_ONLY` means your current *backups* fit the in-memory model. It **does not** mean the installed FM1 partition can be converted in place.

## Why the real flash migration remains disabled

The current firmware journals twelve projects in fixed 12 KiB A/B banks at `0x97000–0xDEFFF`. Storage V2 would reuse those very same 288 KiB for a completely different layout. There is no proven online atomic converter that guarantees all existing projects survive a power cut, and the full flash ownership, recovery route and scheduler/DMA timing have not been verified on physical FM1 hardware.

**Do not overwrite the installed project journal with an offline-generated Storage V2 image.** This tool intentionally **does not generate a firmware image or a writable flash dump**. Hardware integration requires explicit verified off-device backups, a physical recovery test, and a staged/verified restore procedure before enabling any flash write.

## Build and test validation

- Existing V1.4.14 read-only and experimental OTA/USB JieLi builds succeed; output application SHA stays unchanged from V1.4.15. Product identity remains `FM-1_2123` (V1.4.5 package family).
- V1.4.16 tests, V1.4.15 generation lease test, native 6-voice prefetch, BD4S codec, Storage V2 power-failure simulation, OTA loader tests and legacy music/REC regressions succeed in host tests.
- Firmware Hub switching to arbitrary firmware, realtime sample playback, OTA recovery, physical DMA stability and Flash V2 migration are **not validated on a real FM1**.

## Cleanup policy

The distributable ZIP removes generated `build/` binaries and caches, redundant obsolete root firmware versions, duplicated stale HTML names, root-level inventory/log clutter, and the privately designated `tests/private-fixtures/` project backup. It preserves source, current web tools, licencing/credits, target build scripts, test inputs necessary for active tests, current V1.4.14 candidate firmware, reference package identity, and documentation/history. Older firmware snapshots needed for historical regression testing are retained as test fixtures and explicitly not current release candidates.

**The historical root docs were moved into `docs/history` so the repository starts with a current truthful README.** The version of the embedded WebInstaller remains V1.4.14 because no new native sample firmware has been released.
