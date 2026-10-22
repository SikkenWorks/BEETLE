# V1.4.22 — Native global U01–U13 sample playback (EXPERIMENTAL)

- Native conditional Storage V2 holds 12 separate project slots and 13 independent sample slots within the original BEETLE partition.
- BEETLE Project Manager converts WAV to original 22.05 kHz mono BD4S and transfers it in acknowledged USB-MIDI chunks. Full checksum validation and transaction commit precede publication.
- On a correctly migrated V2 volume, CH6 user drum notes U01–U13 use six existing drumvoices and the native `beetle_bd4_live` prefetch/decoder. The audio ISR reads only predecoded PCM windows, no flash access.
- Host tests: sample data crossing V2 flashsector boundaries, exact 40,000-frame reference PCM, six parallel 44,000-frame streams, no underrun with scheduled prefetch, no flash reads during audio render; source compiles with JieLi.
- Shared scratch memory requires pausing native audio for INFO, transfer, delete, project operations; background prefetch resumes after operations. Hard real-time margin and minimum trigger latency are not established on physical FM1.
- **V2 migration remains explicit via verified complete backup and offline recovery-image tool.** This release does not automatically rearrange a legacy 288 KiB project volume during boot or installation. No actual FM1 hardware result is claimed.
- Capacity objective 10–20 seconds shared at 22,050 Hz depends on free sectors after storing twelve projects. Four full erase sectors remain reserved. FULL refuses without modifying committed records.
- Fixed package identity `FM-1_2123` from V1.4.5. V1.4.22 `.fwsc` retains the reference protected header and OTA-loader exactly.

---

# V1.4.18 — Native Storage V2 readback and offline recovery-image builder

- Original native C V2 flash reader handles 12 packed projects and 13 sample slots, with CRC/bounds checks and multichunk reads. Built for JieLi pi32v2 and tested on host.
- Image generator migrates verified 1 MiB legacy backup and optional BD4S to V2, preserving all non-BEETLE bytes. It does not access physical flash and requires matching V2 firmware.
- Current native firmware and audio engine still do not activate V2 storage or U01–U13 playback. This is a **source-only** iteration.
- Established recovery path acknowledged; remaining implementation work is native V2 writes, project routing, USB transfer and DMA-safe sample mixer integration.

# Latest: V1.4.15 — offline Storage V2 lease safety

- Target: 13 shared U01–U13 slots, **10–20 seconds combined where actual project occupancy permits**.
- Desktop `.beetlebank` import/export: operational; its 32 MiB *host-file* cap is **not** FM1 capacity.
- Offline BD4S codec, six-voice prefetch and transactional 288 KiB NOR model: operational in tests, **not running on FM1**.
- **New:** whole-sector scratch reserve and immutable sample-generation leases, tested across replace/delete/checkpoint while two old/new generations of one slot play simultaneously.
- FM1 on-device global sample bank, migration, safe upload and real-time sample replay: **not implemented / not hardware-verified**.
- Native firmware/WebInstaller: unchanged V1.4.14 candidate with identity `FM-1_2123`; there is deliberately **no new V1.4.15 `.fwsc`**.

See `docs/V1415-GLOBAL-SAMPLE-GENERATION-LEASES.md` for detailed tests and unresolved hardware gates.

---

## V1.4.14 storage-to-stream integration (host simulation)

- Validated data flow: transactionally committed project+sample NOR simulator -> CRC-verified byte-range reader -> existing native DPCM4 prefetch -> six-voice RAM audio renderer.
- Test runs confirm all 13 global slots, 12 project slots, replacement, deletion, simulated restart and corrupted sample record detection.
- Render callback performs zero storage reads. PCM output matches the independent decoder (44.1 kHz interpolation), under timely prefetch. The bank is not yet in physical FM1 storage; audio/USB drivers and actual DMA remain untouched.
- No user sample capacity claim for an actual FM1. 10–20 seconds is still a target reached only in selected offline flash simulations; 12 full projects can force a non-destructive refusal.
- Physical migration, loader/recovery test, atomic voice generation pinning, concurrent ISR/main handoff, and heavy-load hardware measurements are blockers before enabling hardware sample transfer/playback.

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

## Stage 2 update — 2026-10-21 (PC tools, native read-only lab)

The portable `.beetlebank` v1 format remains unchanged. The current Project Manager
shows exact WAV metadata, rejects malformed `fmt `/`data` structures, permits
individual WAV exports and stops previews when replacing/deleting samples.
A storage-agnostic, read-only C bank parser has been added for host integration
testing. Compatibility was checked against the original empty bank fixture and
multi-slot PCM/float WAV banks. The supplied 62.4-second Kokiri WAV was also
roundtripped byte-identically in JavaScript and independently validated by the C reader.

**No on-device global sample storage is now available.** The V1.4.5 `.fwsc`
remains unchanged; this stage must never be described as a V1.4.6 firmware.
See `docs/GLOBAL-SAMPLE-BANK-STAGE2.md` for the native hardware safety gates.


## V1.4.9 — Host-only prefetch prototype

A new bounded six-voice PCM streaming module was added with storage reads in a
main-loop prefetch callback only and audio rendering from a fixed PCM cache.
See `docs/V149-PREFETCH-AUDIO-LAB.md`. This does **not** grant any additional
physical sample storage or enable sample transfer/playback on the FM1.


### V1.4.10 mono/song update
The mono SID EDIT note-drop and Song Chain empty-lane presentation are changed in the native source. No new on-device global sample playback; V1.4.9 sample prefetch remains a host lab. Physical FM1 audio/UI results have not yet been verified. Identity: `FM-1_2123`.


## V1.4.13 mapping audit

User sample note routing for U01–U13 (physical drum IDs 14–26) has a standalone native-compatible header and host unit test. The installed firmware does not yet persist or stream a global bank. The prefetch module is unconnected to physical flash and to the audio ISR.
