# BEETLE V1.4.22 — Native Global User Samples (experimental)

## What is now in the FM1 application

- On a **valid existing Storage V2 volume**, twelve BEETLE project slots and thirteen global sample slots coexist within the same 288 KiB sector allocator, with disjoint project/sample erase ownership and a four-sector reserve.
- The web Project Manager converts imported WAV into BEETLE-original 22,050 Hz mono BD4S and transfers sample files over the acknowledged USB-MIDI sample protocol (INFO / BEGIN / DATA / COMMIT / DELETE).
- CH6 U01–U13 now invoke the **native DPCM4 live bridge**, sharing the existing six drum voice objects, mixing into the existing drum/mixer chain. Factory ROM drum notes remain supported.
- Decoding, CRC checks and flash reads occur in bounded background prefetch. The audio ISR consumes PCM windows. No flash or USB call occurs in the native sample rendering path.
- Sample INFO / BEGIN / DELETE stop audio ownership *before* reusing project staging scratch. Samples and project workflows borrow the same RAM because permanent 6 × 4 KiB caches failed actual target linker memory limits.
- A previous double interrupt lock on stream start, PCM ownership during USB transfer, stale ROM-length calculation on live retune, and boot initialization scratch overwrite were corrected.
- Product identity is `FM-1_2123`, from BEETLE V1.4.5. The protected firmware header and OTA-loader are unchanged.

## Files and build

- `firmware/fm1-native/beetle_bd4_live.[ch]`: active bridge.
- `firmware/fm1-native/beetle_bd4_prefetch.[ch]`: RAM-only six-voice decode and 44.1 kHz interpolation.
- `firmware/fm1-native/beetle_storage_v2_reader.[ch]`: CRC-aware validated V2 sample records.
- `firmware/fm1-native/beetle_storage_v2_writer.[ch]`: transaction-bound project/sample storage.
- `firmware/fm1-native/beetle_sample_transfer.inc`: device USB-MIDI transfer.
- `tests/dpcm4/run_v1422_live.py`: full V2 flash -> native C decoder -> exact reference output plus six simultaneous samplevoices.
- Firmware: `BEETLE-V1.4.22-GLOBAL-SAMPLES-LIVE-EXPERIMENTAL.fwsc`.
- The `reference/BEETLE-V1.4.5-PACKAGE-REFERENCE.fwsc` package is included to reproduce the firmware packaging (protected bytes are left intact).

Build when a compatible separately licensed JieLi toolchain is installed:

```sh
JIELI_TOOLCHAIN=/path/to/toolchain python firmware/fm1-native/build.py --beetle-storage-lab --updater --usb-audio --beetle-v2
python tools/build_v1422_fwsc.py reference/BEETLE-V1.4.5-PACKAGE-REFERENCE.fwsc build/track-fm1-native-beetle-storage-lab-updater-uac/app.bin BEETLE-V1.4.22-GLOBAL-SAMPLES-LIVE-EXPERIMENTAL.fwsc
```

## Required one-time Storage V2 migration

**Installing a firmware `.fwsc` does not rearrange existing legacy project flash.** An already migrated V2 volume is required. This is an explicit workflow using the user's proven external restoration method; the tool deliberately never writes a USB device:

1. Take a complete original 1,048,576-byte FM1 flash backup, and keep an unmodified copy.
2. Run `tools/storage_v2/legacy_flash_audit.py` against that backup. Export and verify every legacy project slot in an explicit twelve-slot manifest (including slots recorded as empty).
3. Run `tools/storage_v2/migration_preflight.py` on the manifest to validate projects, sample data and space allocation.
4. Run `tools/storage_v2/create_v2_recovery_image.py SOURCE.bin MIGRATION.json OUTPUT.bin --acknowledge-v2-firmware-required`. This creates a **new** full-device image; it never edits `SOURCE.bin`.
5. The output must be restored using the separately established full-flash recovery route, paired with matching V2-capable firmware. An ordinary WebInstaller `.fwsc` upload **does not** accomplish full-flash migration.
6. In BEETLE Project Manager open your `.beetlebank`, connect USB-MIDI and synchronize U01–U13. Empty slots remove existing device slots. Large uploads can be refused by `STORAGE FULL` without invalidating prior committed data.

No personal flash dump or migration manifest has been supplied in this run, so no device-specific migrated image can be created or certified here.

## Test evidence and limitations

- JieLi target V2/USB build and read-only build compile and link.
- Package is verified, app image matches the built program, V1.4.5 protected header/OTA-loader is byte-identical.
- 40,000 PCM output frames bit-identical to independent BD4S decoder from real flash-layout records.
- Six parallel stream voices for 44,000 render frames: zero measured host underruns with scheduled prefetch, and no flash reads from render callback.
- Storage V2 model: twelve independent projects, thirteen independent samples, corruption rejection, copy-on-write power-cut and storage-full tests.
- **Physical FM1 has not been available for these tests.** Audio onset latency, DMA deadline compliance under real USB+LCD load, long-session glitches, firmware migration in physical flash, WebMIDI behavior and The Firmware Hub external switching remain unverified. This is an experimental hardware candidate, **not a guarantee of glitch-free performance on the device**.
- Large samples are accepted only while there is sufficient flash space after projects and safety reserve. Twenty seconds cannot be guaranteed with twelve maximally sized projects.
