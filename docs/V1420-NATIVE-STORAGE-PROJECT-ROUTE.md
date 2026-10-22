# V1.4.20 Native Storage V2 Project Route

## Implemented

- One 288 KiB BEETLE-owned volume at 0x97000..0xDEFFF; marker at 0xDF000..0xDFFFF.
- V2 header `BTLSTV2\x01` and CRC, verified against the recovery-image generator; original TRKJ marker remains a distinct legacy mode.
- Live native C read and transactional writer in the experimental JieLi build, selected for PROJECT SAVE/LOAD/DELETE and USB project transfer only when the fully owned, valid V2 index is present.
- Independent project/sample erase-sector domains, 4-sector guard, index A/B generations, CRC readback before commit.
- The native writer receives data from the existing project encoder and compresses it with BPK1 in existing project staging RAM; no new audio RAM allocation.
- Fault injection for project writes and sample replace/remove, out-of-capacity and corruption handling.

## Migration

Use `tools/storage_v2/legacy_flash_audit.py`, `migration_preflight.py`, and `create_v2_recovery_image.py` offline with a locally held verified full flash backup. The output is a full image for an independent recovery method, not a normal browser update. Always verify all 12 project slots before and after migration. A normal firmware OTA flash alone does not convert legacy projects to V2. **Never upload or distribute a private flash backup.**

## Not yet implemented

- User-sample USB-MIDI transfer for compressed BD4S files.
- Scheduling/triggering U01..U13 from CH6 and live mixing of background-prefetched DPCM4 audio on FM1.
- Real hardware measurement of flash latency, ISR deadlines, and reliability under simultaneous 8 synthvoices, 6 drumvoices, LCD updates, USB audio.

The established recovery method is respected; the above items remain missing code/integration, not administrative blockers. V1.4.20 is a genuine experimental firmware build, not a verified end-to-end user-sample release. Identity `FM-1_2123` and protected assets match V1.4.5.

## Reproduce host validation

```
python tests/storage_v2/test_native_v2_writer.py
python tests/storage_v2/test_v1420_marker_parity.py
bash tests/storage_v2/run_v1419.sh
```

Native JieLi target:
```
python firmware/fm1-native/build.py --beetle-storage-lab --updater --usb-audio --beetle-v2
```
Set `JIELI_TOOLCHAIN` to the installed toolchain path first. A successful build does not prove on-device stability.
