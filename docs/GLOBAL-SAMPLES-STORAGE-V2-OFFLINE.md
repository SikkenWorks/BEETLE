# BEETLE Project Storage V2 — offline prototype

**Development stage: source + host simulator only. No V2 NOR writes are wired to FM1 firmware, no device migration, no new installable firmware.** The included firmware/WebInstaller are the existing V1.4.5 baseline. Existing V1.4.5 JieLi read-only and experimental OTA/USB app builds remain byte-for-byte unchanged.

## Implemented in this stage

- Original C lossless pack/unpack `beetle_project_pack_v2.[ch]`. A new `BPK1` container has byte count, codec version/mode, header CRC16 and uncompressed-data CRC32. Literal, zero runs, repeated bytes and bounded 4 KiB backreferences use a 4 KiB caller-provided hash scratch. An incompressible source automatically falls back to **raw bytes**. Supports any opaque record up to 12,256 bytes, and never strips unknown project fields. **Not yet part of the FM1's save/load path.** Old V23 per-project samples are NOT migrated by this tool.
- Original Python NOR model `tools/storage_v2/storage_v2_sim.py`. Models 288 KiB with two 4 KiB metadata sectors, a protected reserve of four wholly erased 4 KiB sectors, 256-byte NOR program pages, immutable CRC-checked project/sample records, 12 project slots and 13 device-global sample slots.
- A/B alternating superblock commits with a LAST-page `B2OK` commit marker; snapshots authenticate both generation/header and payload. The system checks all record headers and payload CRCs when booting, and rejects unknown sectors, bad metadata and incompatible ownership.
- Two stored generations for each occupied project slot. Sample replacement writes new immutable chunks first and atomically publishes new pointers; until the new superblock commits, previous metadata still references the original chunks. All non-metadata programming is read back before index commit. Multiple generations of a global sample are retained while older valid metadata still references them.
- Fail-closed handling for full storage or a missing 4-sector reserve. No project/save/sample update is committed when there is insufficient safe space. Sector-only garbage collection never erases sectors referenced by either verified metadata generation. An explicit checkpoint retires an older index and can release whole obsolete sectors; **intra-sector garbage collection/relocation is not implemented yet**.
- Python fault injection immediately after every NOR page program and erase action for representative project update and sample replacement/deletion, plus a torn commit-marker case, CRC corruption and full-storage refusal. These are **modeled boundaries**; an actual brownout can interrupt *inside* a flash page or erase.

## Tested on included BEETLE TRK1 fixtures

| Fixture | Original | BEETLE packer |
|---|---:|---:|
| BEETLE V19 | 1,637 B | 487 B |
| BEETLE V21 | 1,295 B | 230 B |
| NUGGIE, V22 | 1,724 B | 572 B |
| MOUNTAINKING, V20 | 8,779 B | 3,754 B |

Byte-for-byte restore confirmed. Only the available V19–V22 fixtures have been checked against actual BEETLE records. This is **not** proof of full V1–V23 migration coverage; additional format fixtures and old-device recovery tests are needed.

## Capacity proof points (simulated only)

The sample data used for 10s/20s tests is a **byte sequence of the planned DPCM4 data size**, not playable DPCM4 audio. Sample codec, quality tests, streaming, prefetch and CPU/DMA measurements remain to be implemented.

- 12 project slots holding one MOUNTAINKING-sized and eleven NUGGIE-sized encoded project records: **10 seconds' worth of sample bytes** and **20 seconds' worth of sample bytes** can be stored in the model with the 4-sector guard intact.
- Twelve maximum-size high-entropy project payloads remain readable, while an overcapacity 20-second sample upload is **rejected before NOR mutation**.
- These are scenarios, not a promise that all possible real project populations fit 20 seconds. Their results also depend on record generations, fragmentation and free sectors.
- Deleting or replacing samples can reclaim whole obsolete sectors after an explicit safe metadata checkpoint; mixed live/dead sectors are not yet compacted. A full-volume allocator may therefore reject an operation even if the **theoretical** number of reusable bytes looks sufficient.

## Still NOT implemented (release blockers)

1. Verifying exact physical flash-chip model, partition ownership, vendor OTA and independent hardware recovery. The Python `authorized=True` switch **does not establish real hardware ownership**.
2. Non-destructive migration of the existing 24 fixed-size project generations and legacy V23 embedded one-shot data. Migration must preserve both old and new formats through interrupted upgrades, provide verified off-device backups and refuse incompatibilities.
3. Independently authored BEETLE DPCM4 encoder/decoder, resampling and audible quality tests.
4. Device driver/XIP arbitration, DMA-safe prefetch and worst-case execution time with 8 synthvoices, 6 drumvoices, MASTER FX, USB stems and LCD updates. **Never read or program flash inside the DMA callback.**
5. Physical power-loss/torn-write qualification at subpage resolution, wear leveling, full compaction, partial erase behavior, hardware endurance and firmware downgrade compatibility.
6. Versioned USB-MIDI capability/transfer protocol, real device transfer and Project Manager wiring.
7. Project serialization of stable U01–U13 references and bank generations; missing-sample handling across all 12 projects.
8. Safe hardware testing; only then may new `.fwsc` flashing be considered.

**Do not flash anything from this prototype expecting global sample support.** This milestone does not increase the FM1's real sample capacity. Old V1.4.5 firmware, existing WAV bank manager and existing credits are preserved.

## Reproduce

Linux/WSL + C compiler + Python 3.10+:

```sh
python tests/storage_v2/run_tests.py
python tools/storage_v2/capacity_model_v1.py
node tests/global_bank/test_cross_language.cjs
```

Test script compiles the C packer with compiler warnings and UndefinedBehaviorSanitizer; compiles **host code only**. Native JieLi firmware recompilation is optional and uses the separately installed toolchain.

### Engineering caution

The host simulator is a **verification model**, not firmware: its Python object model, metadata layout and allocator still require a separately reviewed native implementation. It models mutation-level power loss but does not prove atomic flash page programming or independent hardware rescue. Sector-only reclaim has limited reuse under fragmentation. No copy of another firmware's sample/allocator implementation was used for this stage, although BEETLE's existing upstream-derived base and legal credits remain unchanged.
