# BEETLE V1.4.18 — Native Storage V2 reader & recovery-image preparation

**Scope: real new source code and reproducible C/flash-image tests. NOT an installed user-sample feature yet.**

## What's new

* `firmware/fm1-native/beetle_storage_v2_reader.[ch]` is an original native C reader for the persistent B2SB/B2RC volume. It recognizes the newest committed superblock, checks CRC and ownership-relative bounds, supports the twelve project records (including genuine BEETLE lossless unpacking) and reads U01–U13 sample data across 4KiB-sector/chunk boundaries. It rejects corrupt record headers and payloads and falls back to older confirmed metadata. It accepts a physical flash read callback, so the same reader runs against a host NOR model or an FM1 driver. It was compiled for `pi32v2` JieLi as well as native x86_64. **It is not yet connected to on-device project save, user-sample triggers or the audio ISR.**
* `tools/storage_v2/create_v2_recovery_image.py` accepts a private *complete authenticated 1 MiB legacy flash backup* plus an explicit matching twelve-slot manifest and optional BD4S files. It creates a *new* complete V2-format flash image with the original bootloader, app, OTA staging and all other bytes unchanged. It modifies only `0x97000..0xDFFFF`, with the last 32 bytes being a new V2 ownership marker. The tool refuses a missing/incorrect slot, malformed input, unsafe capacity and existing output paths. After creation it checks every project/sample byte after simulated reboot and the four-erased-sector reserve.
* Two tests assert exact project/sample contents, fragmented sector crossing, corrupt-record refusal, rejected backup mismatch and no modification outside the owned BEETLE volume.

## Why this is not yet a user-installable V2 release

The existing FM1 firmware still has `track_journal_*` wired into the legacy fixed A/B partition and the six-voice BD4S renderer remains outside the hardware audio path. **A V2-format recovery image cannot be installed while running V1.4.14/legacy firmware** and should not be flashed by itself. Implementing native V2 writes/USB transfers and replacing legacy project read/save atomically are still engineering requirements; a successful migration-image build or pi32v2 compile does not prove live sample playback.

The user's independently established recovery path is accepted as available; this document does not make recovery access a reason to block development. But recovery capability cannot substitute for the missing firmware code, nor guarantee that every project survives an incorrectly applied new flash layout.

## Local use / privacy

1. Keep the full original 1MiB dump private and preserve a second untouched backup.
2. Audit it using `tools/storage_v2/legacy_flash_audit.py` and export all twelve slots to a new private folder.
3. Add optional valid BD4S samples to the twelve-slot `MIGRATION-MANIFEST.json` (format described in `migration_preflight.py`).
4. Run `python tools/storage_v2/migration_preflight.py /path/to/MIGRATION-MANIFEST.json`.
5. To **create an offline recovery image for the matching future V2 firmware**, use `python tools/storage_v2/create_v2_recovery_image.py old-flash.bin /path/to/MIGRATION-MANIFEST.json v2-prepared.bin --acknowledge-v2-firmware-required`. Do **not** flash the resulting image into V1.4.14.
6. Re-run `bash tests/storage_v2/run_v1418.sh` after any modifications.

No tool in V1.4.18 writes to the physical FM1. Neither this generated image nor the standalone V2 reader is a firmware `.fwsc`.

## Safety / stability

All flash accesses are bounded to the former 288KiB BEETLE project volume. This new C reader performs CRC checks in main/prefetch context; full-chunk CRC verification is comparatively expensive and must not be done inside the DMA audio interrupt. A low-latency pre-verified, generation-pinned read path and native SPI NOR write manager must be integrated and stressed before release. The current 13 slots and 10–20 second capacity are **verified in specific offline scenarios**, not universally guaranteed under twelve maximum-sized projects.

## Continuity

Previous user requests retained: permanent global U01..U13 sample bank, twelve project slots, sample replacement and reclaim, no hidden polyphony or audio-quality reductions, package identity matching V1.4.5 (`FM-1_2123`), Firmware Hub cross-firmware compatibility testing, deferred advanced USB/DMA diagnostics, REC LED and Song Chain fixes. The previous firmware in the source is unchanged and **must not be represented as V1.4.18 playback firmware**.
