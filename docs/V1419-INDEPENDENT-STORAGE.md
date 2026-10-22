# BEETLE V1.4.19: Independent Project & Global Sample Storage

## Implemented in this source release

The original BEETLE Volume V2 is a single **288 KiB** physical NOR region (`0x97000..0xDEFFF`). The new allocator maintains **two logically independent, physically non-overlapping erase-sector namespaces** within that region:

* **PROJECT**: twelve independently indexed slots. Project records may share 256-byte pages inside PROJECT-owned 4 KiB sectors. The format still retains previous generations and transactional metadata.
* **SAMPLE**: one global library of **U01–U13** independent of projects. Compressed sample chunks may share pages within SAMPLE-owned 4 KiB sectors but cannot cross an erase-sector boundary or enter a PROJECT sector. Sample generations can stay pinned while a voice finishes.
* **SHARED FREE POOL**: any fully erased, unowned sector can be allocated to either namespace. Allocation is not constrained by an arbitrary 10/20-second fixed split. The minimum **four entirely erased sectors** must remain free after any new allocation, and two metadata sectors remain reserved.

**Deleting or replacing a project never writes sample records. Deleting or replacing a sample never writes project records.** Both actions update the common, CRC-protected transactional B2SB index. `checkpoint()` explicitly retires obsolete metadata and reclaims full unreferenced sectors; active sample leases remain protected.

### Practical capacity

The original 10–20 second goal is a **combined length across all 13 slots** at 22,050 Hz 4-bit BD4S. Test coverage includes a near-20-second payload distributed across all slots, 12 projects including MOUNTAINKING and NUGGIE, reboot, deletion/replacement and free-space accounting. In that particular example, the current index owns **3 project sectors and 57 sample sectors**, with no overlaps. This does **not** guarantee 20 seconds when twelve projects are large. The allocator refuses an overfull upload without starting a NOR mutation.

Older V2 allocation models may have mixed record types in one erase sector. The native **read-only** audit (`beetle_v2_audit_domains`) explicitly reports/rejects that for this newer isolated layout. Its 144-byte page bitmap detects overlapping committed records even inside same-domain sectors. The Python storage model also protects previous metadata generations and immutable audio leases.

A new local **read-only report tool** shows the existing record counts, per-slot sizes, sector budgets, erased sectors and reserved capacity. It accepts a verified *V2-format* 288 KiB partition image or a private 1 MiB full-flash dump:

```bash
python tools/storage_v2/report_coexistence.py /path/to/v2-image.bin
```

It does NOT read physical FM1 over USB and does NOT convert legacy project partitions. A legacy image must first be audited/migrated with the existing separate tools; never apply a V2 decoder directly to unknown legacy flash.

## Tests and build

```bash
bash tests/storage_v2/run_v1419.sh
```

This runs older V2 migration, 13-slot DPCM4 and six-voice prefetch tests, plus:

1. Near-20-second shared U library plus twelve projects, with **zero shared erase sectors**.
2. Project overwrite/delete/checkpoint while sample records remain byte-identical.
3. Sample replacement while a prior generation is pinned by a voice; cold restart.
4. A too-large sample refused before any mutation with all twelve full-sized projects preserved.
5. Native C read-only audit through synthetic FM1 physical addresses.
6. Detection of an intentionally forged, CRC-correct index that illegally points a sample to a project sector.
7. Local V2 capacity report checked against in-memory flash.
8. Existing power-loss injection, twelve-slot migration image, decoder and sample-prefetch tests.

Two existing JieLi firmware configurations also compile, but **the generated application remains V1.4.14**, because the native Storage V2 allocator/write path is not yet linked into the running project save flow. The V1.4.19 sector checker itself compiles for JieLi `pi32v2`. The installer/OTA app, protected loader, WebInstaller, audio engine and product identity `FM-1_2123` are unchanged.

## Remaining implementation, not claimed complete

The physical FM1 still saves its existing twelve projects in the *legacy fixed A/B journal*. It cannot simultaneously use the new allocator merely because the offline model exists. Physical Storage V2 project **writes, volume migration, global sample USB transfer, and live sample triggering/mixing** still have to be hooked into the firmware and measured on FM1 hardware. Do not flash an offline migration image into the existing legacy firmware. This release contains a complete source prototype and tests, **not a new playable V1.4.19 `.fwsc`**.

The user reports an established recovery route; no new gate is being asserted here. The outstanding work is missing device code, not recovery access.
