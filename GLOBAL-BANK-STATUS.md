# BEETLE — V1.4.2 Global One-Shot Library: safe first stage

**STATUS: desktop Project Manager implementation only. Not an installable firmware release.**

This branch deliberately starts from V1.4.1 pre-reSID firmware, retaining the original 14 built-in drum mappings, existing 13 user slots, 12 project slots, licensing and credits. No `reSID` or `ymfm` has been added.

## User experience implemented now

The new Project Manager section **GLOBAL SAMPLE LIBRARY** stores thirteen independent WAV assignments (U01–U13, physical drum keys 14–26) in a single portable `.beetlebank` file that has **no `.beetle` project dependency**. Each slot has IMPORT / REPLACE / PLAY / DELETE. Deleting a sample compacts the export and returns all its bytes to the available library capacity. A fresh `.beetlebank` can be saved, reopened and edited without loss of original WAV audio data. The source contains a CRC-32-protected explicit file index and validates WAV markers, offsets, section lengths, entry checksums and overall checksum.

The *PC file format* has a deliberate 32 MiB safety limit, not an FM1 flash capacity claim. WAV files are preserved byte for byte (no sample-rate conversion or data loss); the machine's actual usable memory is not inferred from this figure. Library state exists in the browser only until it is exported: save the `.beetlebank` before closing the Manager.

## NOT yet working on the physical device

A `.beetlebank` currently **cannot be transferred or played from global on-device storage**. Transfer is visibly disabled in the Project Manager, not simulated. BEETLE V1.4.1 hardware still uses the legacy *per-project* 6,144-byte V23 audio extension; the legacy editor is hidden in the new Manager to avoid offering it as if it satisfied the global-bank request. The V1.4.1 firmware included in this archive is **byte-for-byte unchanged**. This avoids pretending that an arbitrary large host file is FM1 sample capacity.

## The critical flash-memory finding

| Region | Address | Assessed role | May claim for global samples? |
|---|---|---|---|
| Package / program and boot | `0x00000..0x92FFF` | Firmware/loader; depends on installation | **No** |
| Gap to project volume | `0x93000..0x96FFF` | Firmware packaging/OTA/recovery margin: ownership not established | **No** |
| BEETLE project journal | `0x97000..0xDEFFF` | 12 projects x two 12 KiB journal banks, already owned | **No** |
| Storage marker | `0xDF000..0xDFFFF` | BEETLE project volume ownership | **No** |
| OTA staging | `0xE0000..0xE4FFF` | OTA loader | **No** |
| Felucca autosave | `0xE5000..0xE6FFF` | Two A/B storage sectors | **No** |
| FM6 voice bank | `0xE7000..0xE7FFF` | Felucca v1.4.1; not proven unused across firmware | **No** |
| Stock OTA VM backup | `0xE8000..0xFBFFF` | Explicitly excluded from normal Felucca writable regions; reserved by stock OTA | **No** |
| Felucca globals | `0xFC000..0xFEFFF` | Global settings / superblocks | **No** |
| Last sector | `0xFF000..0xFFFFF` | Unknown/reserved; not claimed | **No** |

Source: `Felucca-main/firmware/hal/fm1_flash.h` and loader docs (`ldr_core.c`), and BEETLE V1.4.1 `beetle_flash_layout.h`. The table describes known code reservations, **not** hardware-verified free storage.

### FM1 audio memory

The existing µ-law sample bank is 6 KiB split across two preallocated RAM sections. Increasing project size or adding a new flash partition would not automatically make larger samples playable. A future implementation needs an ISR-safe streaming/prefetch architecture; all flash operations must be bounded, nonblocking relative to the audio DMA callback, and checked against storage/recovery ownership.

## Required next phase for native global storage

1. Acquire verified SPI NOR memory mapping and independent bootloader/OTA-recovery information **from the target FM1 hardware**. Backup full flash and all projects first.
2. Identify a dedicated partition *actually unowned* by boot, OTA, VM backup, globals, and project storage, or test an external-storage hardware option. Do not borrow sectors based only on observing `0xFF` bytes; erased bytes do **not** establish ownership.
3. Design a versioned A/B global bank and index with transactional writes, CRC, power-loss fallback, and deletion/compaction or a free-space allocator.
4. Design DMA-safe sample playback and prefetch without reducing existing 8-voice synth polyphony, existing 6 drum voices, 6-track USB or the display's accuracy.
5. Extend the USB-MIDI device protocol with an explicit global-bank capability query, versioned authenticated/CRC-checked multi-packet transfer, read-back verification, and a nonblocking commit outside the audio ISR. Do not reuse project transfer commands for arbitrary flash writes.
6. Make project serialization store only note mappings/references and keep its exact old 12 slots. Define migration for old V23 project-owned sample sets and safe V22 rollback.
7. Test continuous multi-minute chords+drums+USB with worst-case audio deadline, boot/restart, 13 independent samples, and power loss during bank commit. Only ship a `.fwsc` when these pass a hardware review.

**Do not claim this desktop library has increased the device's global sample capacity.** This phase implements cross-project bank management, deletion and an integrity-protected interchange file; the hardware storage and streaming phase is open.
