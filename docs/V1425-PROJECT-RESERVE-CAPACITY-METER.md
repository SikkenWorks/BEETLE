# BEETLE V1.4.25 — Global sample capacity meter and project reserve

The old sample bank manager could report sample occupancy but not remaining free NOR flash or protect the ability to save projects after many user samples. Storage V2 holds projects and sample records within the same 288 KiB partition. Its historical guard required only four fully erased sectors.

## Changes

- Firmware command `0x76` returns 15 bytes (incl. sequence): version, active sample bytes, estimated further usable blank sample bytes, fully erased unowned sectors, project reserve (8 sectors), recovery guard (4 sectors), and blank pages in already sample-owned sectors. It is read-only and pauses the native renderer before borrowing the existing V2 scratch; no new permanent PCM buffer is allocated.
- The native sample writer now refuses sample transactions that would leave fewer than twelve completely erased sectors: eight reserved for future projects plus the existing four reserved for recovery. This condition is checked BEFORE any data page is written. Normal project writes keep the preexisting four-sector recovery guard.
- The Project Manager shows a pink FM1 SAMPLE SPACE meter, occupancy, estimates and protected project reserve. Fragmentation and COW journal overhead can make the actual upload capacity smaller, and the real backend checks each upload again.
- An old V1.4.24/1.4.25 firmware will not respond to `0x76`; the meter says NOT VERIFIED, and the manager does not invent a free-space percentage. Previous inventory, reconnect and per-slot safe synchronization remain supported. The reserved-project-space protection requires this modified native firmware; it cannot be added by a browser-only patch.
- Storage-full sample errors are shown as FM1 STORAGE FULL with advice to free sample space or trim a WAV. The partial batch can retain earlier committed samples; uploads do not have all-or-nothing transaction semantics.

## Limits and validation

An eight-sector reserve is a minimum allowance, not a guarantee that any twelve future maximum-size project updates will fit. Real on-device flash layout, fragmentation, journal generation history, host transfer, audio deadlines, OTA switching and recovery remain unverified on a physical FM1. Project volumes must have been explicitly migrated to Storage V2. Original samples cannot be downloaded from the device; keep an external `.beetlebank` backup. No existing hardware identity, OTA loader, protected header or firmware build source files were removed.

Tests: `python tests/storage_v2/test_native_v2_writer.py`, `python tests/storage_v2/test_project_reserve_1425.py`, `python tests/web/test_v1425_sample_inventory_safesync.py` and `python tests/web/test_v1425_sample_capacity_meter.py`. JieLi build: `python firmware/fm1-native/build.py --beetle-storage-lab --updater --usb-audio --beetle-v2` with `JIELI_TOOLCHAIN` set.
