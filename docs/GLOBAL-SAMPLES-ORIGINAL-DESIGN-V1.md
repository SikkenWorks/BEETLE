# BEETLE Global User Samples — Original Architecture V1

**Status:** standalone design and host-side capacity model. **NOT implemented on the FM1**, **NO flash migration**, and **NO new `.fwsc`**.

## Non-negotiable constraints

1. Thirteen shared user-note assignments **U01–U13**, across twelve independent `.beetle` project slots.
2. Preserve the original 14 ROM drums, eight synthvoices, six existing drumvoices, live and Song Mode, MASTER/Punch-In FX, USB multitrack and the current smooth/non-ghosting playhead.
3. Never overwrite bootloader, firmware/OTA staging, stock VM backups, Felucca globals, or unknown flash sectors. All new persistent data must be inside demonstrably BEETLE-owned storage. Retain independent project backups and a proven restore path before migration.
4. A project references a **global slot and bank generation**, not an embedded copy of the WAV. Old V23 per-project samples must be migrated without discarding the only recoverable copy.
5. Project Manager keeps the original WAV data in `.beetlebank`. On-device playback is encoded and will **not** be bit-perfect identical to the original WAV. Audition the encoded quality before transfer.
6. No sample flash reads, allocation, erases, USB transactions, decompression of whole projects, or other unbounded operations inside audio DMA callbacks.
7. New sample-storage, sample-decoder, allocator and transport implementations must be written independently for BEETLE; no source copied from other firmware projects. Preserve existing GPL and upstream credits for the firmware we already use.

## Ground truth from the current BEETLE V1.4.6 Stage 2 tree

- `beetle_flash_layout.h`: project region `0x97000` through `0xDEFFF`, 288 KiB = **72 erase sectors** of 4 KiB. `0xDF000` is an ownership marker and is **not included** in the budget.
- `track_journal.h`: 12 slots × two fixed 12 KiB generations = 288 KiB. Journal headers use 32 bytes and each payload can be up to 12,256 bytes.
- `track_project_store.c`: V23 can embed up to 6,144 bytes of project-local µ-law one-shots. Such records must be migrated to a global bank **before** project references change.
- Fixture project sizes: `MOUNTAINKING` 8,779 bytes, `NUGGIE` 1,724 bytes (decoded TRK1 payloads).
- Exploratory *desktop zlib* level 1 on those original TRK1 fixtures produces 2,670 and 442 bytes. This demonstrates redundancy, not guaranteed savings or a firmware codec implementation. Production BEETLE's own lossless project packer is still to be designed and validated.
- The `.beetlebank` host limit of 32 MiB says nothing about available flash on the FM1.

## Proposed hardware flash model

Replace **only after safe, explicit migration** the old fixed-size per-slot A/B layout with a **log-structured shared sector pool** in that same 288 KiB. The twelve logical project slots remain intact. Records are packed into 256-byte program-page units so multiple small project generations can share one erase sector. The index is transactional; old data is retained until a new generation is fully written and verified.

- Reserve **two 4 KiB sectors** for alternate metadata/superblocks (generation number, CRC, slot-to-record pointers, sample assignments and free-space state).
- Reserve **at least four 4 KiB sectors** for background garbage-collection/migration slack. A live device may need MORE depending on the largest pending transaction; a 16 KiB floor does **not** guarantee that an arbitrarily long sample can always be replaced at full capacity.
- Store two independently CRC-verified project generations per occupied logical slot, and immutable sample chunks with content CRC and slot/generation linkage. Never erase the only verified copy of any project or sample.
- New sample assignment/removal/replacement commits index metadata only after all changed sample chunks have been read back and verified. Unreferenced chunks become reclaimable **after commit**; sector reclamation occurs only when all live records in that sector have been moved safely.
- A free-space preflight decides whether a project save or sample change is safe. If it cannot preserve rollback and scratch space, **reject the operation with a clear error** rather than overwriting something.
- Do not blindly repurpose erased (`0xFF`) flash. Initial layout ownership, migration and recovery must be independently proven on hardware.

### Why space savings are plausible, but not guaranteed

A compressed project can be a few hundred bytes yet currently occupies two entire 12 KiB banks (24 KiB). Packing small A/B records together can reclaim this sector slack. However, if all 12 projects contain 12,256 incompressible bytes, even a perfect allocator cannot fit them plus 10 seconds of samples into this same 288 KiB. **At least one condition must give:** sampletime adapts downward, project capacity is restricted, or additional proven hardware storage is added. We will never silently restrict existing project data or corrupt it.

## Sample codec: own BEETLE DPCM4 (proposal)

- Source WAV imported and preserved in the PC library; offline encoder outputs a dedicated *BEETLE sample format* for the FM1.
- 22,050 Hz mono, 4 bits per sample with **independent 256-frame blocks**. Proposed per-block overhead: four-byte predictor/step/flags header and 128 encoded bytes = **132 bytes per 256 samples**. Exact predictor, step adaptation, clip control, rounding, dithering and error tolerance are to be engineered and tested, not borrowed.
- Estimated rate: **~11,370 bytes/sec**, before per-slot indexes and other metadata. Ten seconds ~113,784 bytes (28 sectors); twenty seconds ~227,436 bytes (56 sectors).
- A 44.1 kHz output is generated with rate-correct interpolation, supporting normal one-shot speed and pitch rules. Measure the native decode cost against the actual DMA deadline. Keep ROM drum behavior unchanged.
- Sample voices use the existing separate six-drum-voice pool, never synthvoices. If all six drumvoices are already sounding, existing documented drum-voice allocation behavior applies. No secret reduction of supported simultaneous voices.
- Per-voice state: immutable sample-generation reference, read cursor, decoded block state, envelope, and a bounded prefetch buffer. All I/O and chunk acquisition outside ISR; the DMA callback only consumes ready buffers and updates voices with O(voices) bounded work.
- During flash erase/program, **do not assume XIP sample playback remains safe**. Default to allowing sample-bank writes only when transport is stopped, and explicitly validate whether USB/LCD/OTA or execute-in-place memory reads contend with flash operations.

## Measured planning scenarios (not device results)

`capacity_model.py` simulates **two** project generations per slot, 256-byte program-page record packing, **two** metadata sectors and **four** transaction-slack sectors, without yet accounting for live-fragmentation or every sample index byte. All numbers are theoretical upper estimates:

| Twelve projects | Potential combined sample capacity |
|---|---:|
| One MOUNTAINKING + eleven NUGGIE-sized, current payloads | ~18 s |
| Same project mix, using desktop compression *measurements* | ~21 s |
| Twelve MOUNTAINKING-sized, current payloads | ~4 s |
| Twelve MOUNTAINKING-sized, desktop compression *measurements* | ~17 s |
| Twelve 12,256-byte incompressible projects | **0 s guaranteed** |

This is why **10–20 seconds is a target, not an unconditional guarantee**. A shipping implementation should show `Used`, `Available`, `Reserved for safety` and the actual usable sample seconds, and refuse transfers that would consume protected reserves.

## Native protocol and on-device use

1. Separate USB-MIDI SysEx capability query `GLOBAL_BANK_V2_SUPPORTED`; old firmware reports unsupported. Never reuse project opcodes for raw flash writes.
2. Host encodes WAV to BEETLE's own blocks and displays the real projected duration/bytes before upload.
3. Multi-packet transfer has monotonic sequence numbers and per-chunk CRC. Flash writing is dispatched outside audio ISR, transport stopped, to unowned-free sectors **within BEETLE's proven partition**.
4. Read-back verifies blocks; a new metadata generation points to the chunks only after complete checksum validation. On interrupted power, choose the last fully verified index.
5. Project saves contain logical U01–U13 references and selected generation mapping. Opening a project without a referenced sample shows `MISSING SAMPLE`; do not silently remap to another sound.
6. Individual delete/replace is sector-reclamation safe; when there is not enough scratch space, show an actionable `NOT ENOUGH SAFE SPACE` and keep the old bank intact.

## Engineering sequence before a deployable firmware

1. Implement BEETLE-native **lossless project packing + unpacking** and verify bit-for-bit decoded equality for all format versions V1–V23 and all twelve slots. A compressed payload that grows must fall back safely to the original bytes.
2. Build a host NOR simulator with byte/sector programming constraints, A/B records, interruptions before and during every erase/program/commit, CRC failures and worst-case compaction.
3. Reproduce 12 dense, 12 sparse and mixed project scenarios. Never report full capacity if migration or replacement cannot commit atomically.
4. Investigate the physical flash and actual XIP/read timing. Prove device recovery, complete backup and non-destructive migration first. Without this proof, do not enable writes.
5. Implement and A/B-listen the **independently authored** 4-bit codec against source WAVs; measure distortion, low-frequency noise, pitch and aliasing.
6. Instrument native DMA worst-case duration and USB/LCD timing with synth + all drums + six simultaneous user samples. No allocation, flash-blocking or silent voice reduction in ISR.
7. Add a guarded transfer protocol, Project Manager UI, compatibility and error messages. Ship firmware only after hardware proof.

## No external sample-engine source reuse

The design choices above are our own engineering constraints and proposed data structures; using standard ideas such as CRCs, transactional journals and 4-bit differential quantization does not require copying anyone else's code. The actual codec, allocator, storage/migration code and transfer implementation will be new BEETLE code. This does **not** remove Felucca/ETCH/JieLi and other existing credits in the already-derived BEETLE firmware.
