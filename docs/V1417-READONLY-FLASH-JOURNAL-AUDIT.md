# BEETLE V1.4.17 — Legacy flash journal audit before sample migration

**READ-ONLY DEVELOPMENT TOOL. NO V1.4.17 FWSC, NO FLASHING.**

## Why this stage matters

The current firmware uses 12 × 2 × 12 KiB legacy A/B journal banks (`0x97000` through `0xDEFFF`) and one 4 KiB volume marker at `0xDF000`. Our proposed shared Storage V2 design needs that same physical partition. Writing Storage V2 there *without recovering every existing committed project* could erase projects. A previously empty-looking area is not sufficient evidence of ownership.

This stage adds `tools/storage_v2/legacy_flash_audit.py`, an independently authored **offline full-flash image auditor**. It checks the original BEETLE marker across the entire marker sector, scans all 24 A/B journal banks with native-compatible rules, checks both outer journal CRC and inner TRK1 CRC, and selects only confirmed project generations (including generation wraparound). Unknown data, non-erased bank tails, uncommitted-only records, missing/malformed marker or ambiguous generations cause a refusal. A torn sister bank with one surviving verified generation can be recovered without treating the torn data as valid.

**Important:** this utility *does not read the FM1 over USB*. The standard Project Manager does not provide a verified full 1 MiB flash dump. Only use a full image obtained with a separate, verified, read-only procedure. **Do not attempt risky disassembly or flashing to obtain an image.** Keep the full image and exported projects private; do not publish them to GitHub.

## Run, entirely on your computer

From the repository root, with a preexisting verified local 1,048,576-byte full flash dump:

```bash
python tools/storage_v2/legacy_flash_audit.py /private/path/fm1-flash.bin
```

To optionally extract validated projects to a **new, previously nonexistent directory**:

```bash
python tools/storage_v2/legacy_flash_audit.py /private/path/fm1-flash.bin --export-dir /private/path/BEETLE-LOCAL-MIGRATION
python tools/storage_v2/migration_preflight.py /private/path/BEETLE-LOCAL-MIGRATION/MIGRATION-MANIFEST.json
```

The exporter creates `SLOT-01-verified.trk` ... only for occupied, CRC-validated slots, `LEGACY-AUDIT.json`, and a twelve-entry `MIGRATION-MANIFEST.json` that explicitly represents empty slots. Exports are normal TRK1 bytes. It does not rewrite flash or convert projects on the device. **A successful preflight means that this backup fits in the offline Storage V2 model, not that installing a new layout is safe.**

You can add optional U01–U13 BD4S files to the exported manifest *locally* before preflight. The scanner never assumes global samples are already present in the old physical project partition.

## Implementation/testing

- Full image length exactly 1 MiB; marker with ownership CRC and entire untouched marker-sector tail
- Twelve numbered project slots, with both 12 KiB banks inspected; no blind `0xFF`-header assumptions
- `TRKJ` outer header, version, inverse slot ID, transaction commit, generation, data size, data CRC, header CRC, erased tail
- Inner `TRK1` project header, schema up to V24, matching slot and size, inner header and body CRC
- Signed generation wraparound, same-generation conflict refusal, older committed fallback from interrupted updates
- Tests use synthetic 1 MiB images plus existing CRC-valid MOUNTAINKING/NUGGIE fixtures and verify that generated project exports pass the real Storage V2 offline migration/reboot test
- No hardware access, persistent write operations, new sample allocation in FM1, or change to executable firmware

Run:

```bash
python tests/storage_v2/test_legacy_flash_audit.py
python tests/storage_v2/test_migration_preflight.py
python tests/storage_v2/run_tests.py
```

## Still blocked before global samples work on FM1

1. Independent *non-destructive* backup and **proven recovery** of the actual bootloader, OTA state and all twelve projects
2. Proven flash sector ownership, **safe on-device migration/restore**, including power-loss recovery and rollback across old/new layouts
3. Reliable bounded USB sample transfer/read-back with concurrency control and no direct arbitrary flash write commands
4. FM1 RAM, main-loop vs. audio-ISR synchronization and DMA timing measurements under real heavy workloads, including LCD and USB multitrack
5. Physical validation of factory drums, synthesized voices, user samples and all twelve project slots after reboot and cross-firmware updates

**The existing experimental V1.4.14 firmware, WebInstaller, audio engine, Song Chain behavior and the V1.4.5 package identity `FM-1_2123` remain unchanged.**
