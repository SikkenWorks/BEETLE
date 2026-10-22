# BEETLE V1.4.13 — Hardware-unverified Hub/REC/sample safety pass

## Firmware Hub warning

Screenshot from the user says: **Different firmware's update loader is running. It will only accept its own package. Finish that update first, or use USB rescue to install any firmware.**

The Hub distinguishes a normally running application and an update-loader; its public KB says a normal firmware should be able to initiate a firmware switch. The screenshot alone does not show whether the FM1 truly runs an update-loader or whether the Hub has misclassified the normal BEETLE USB endpoint. Only live hardware data can distinguish them.

BEETLE's stock OTA code sends SysEx **0x11 identity** `FM-1_2123` from the application and supports a separate **0x50 BEETLE HELLO** read-only capability request. Its two-stage OTA staging gate in `firmware/fm1-native/ota.c` currently accepts only the exactly pinned legacy community loader and an exact-image official V15 restore route. It does **not** safely support arbitrary third-party `.fwsc` loaders. We did not loosen this gate, claim a Hub fix, flash a device or touch the bootloader.

### Non-destructive diagnosis

Open `BEETLE-Hub-Diagnostics.html` in Chrome or Edge on a computer (WebMIDI with SysEx permission). Keep BEETLE running normally. Click Connect MIDI, choose both FM1 ports, and click Read-only check.

- Identity returns `FM-1_2123` and BEETLE HELLO `0x60`: **the normal BEETLE app is responding**. If the Hub still calls it an updater, its port/updater classifier or identity catalogue requires examination by the Hub maintainer.
- Identity responds but HELLO does not: **indeterminate**; may be updater mode, different firmware, unsupported transport, or missing messages. Do not force a write.
- Neither responds: check USB MIDI cable, permission and whether another MIDI app holds the port.

The HTML sends only `0x11` and `0x50` SysEx requests. It cannot install, stage firmware, erase, reformat, unlock flash or fix another service's client code. The tool does not access private project/flash data.

### Safe route while investigating

1. Back up all BEETLE projects and sample-bank files on a computer.
2. If currently in a firmware updater, finish it using its own verified installer. **Do not power off while writing.**
3. If the device works in BEETLE and Hub continues to misclassify it, use the separate **verified stock V15 route** only after an independently proven recovery plan, then reconnect to The Firmware Hub. Stock restores may destroy or orphan data that other firmware owns.
4. Hardware recovery via OCT− and the Hub's rescue documentation is an emergency method; test/review prerequisites first.

See https://thefirmwarehub.com/kb/ for up-to-date protocol and recovery guidance.

## REC LED

In `track_fm1_leds` physical button 13 is no longer forced solid ON. Pages 3 and 4 follow `track_punch_rec_armed`; other pages follow `track_state.record_armed` (Tracker note-entry ON by default). Boot ripple remains unchanged. No write to project data; no audio IRQ work.

## User samples

Mapping U01–U13 uses notes 14–26, with the existing 14 built-in drums at 0–13. A new pure function proves no slot collision, but no global flash partition or DMA-active playback was activated. Host stream and memory tests are the previous V1.4.9 prototypes, not hardware evidence.

## Acceptance

- REC light follows note-entry toggle in TRACKER and Punch-REC toggle in FX pages: native-policy unit test PASS.
- Package identity unchanged `FM-1_2123`: offline firmware audit required on each build.
- All prior song/drum and prefetch host tests must pass.
- The Hub cross-firmware switch is **not verified**; two-stage installer must be tested on an actual FM1 with a validated backout route.
