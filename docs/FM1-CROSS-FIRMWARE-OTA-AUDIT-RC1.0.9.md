# BEETLE 1.01 RC1.0.9 — Cross-Firmware OTA Audit

Audit source: `FM1-CROSS-FIRMWARE-OTA-AUDIT-BEETLE-B1-NERVE.md` (user-provided). Audited native BEETLE baseline: RC1.0.8. New corrected build: RC1.0.9 (`FM-1_2109`).

## Confirmed problems and corrections

| Requirement | Before: native RC1.0.8 | RC1.0.9 | Result |
|---|---|---|---|
| App-only package with a different, unused boot header | `ota.c` compared every byte of the first 16 KiB and returned `-5` | Full app compatibility and CRC checks replace unrelated head comparison | **Fixed in source and native replay** |
| Alternate loaderbody | SHA-256 pinned to the verified 6,898-byte loader; rejects with `-16` | Still SHA-256 pinned; incoming unknown loader is never executed | **Intentional safety restriction**; not universal cross-loader support |
| Encrypted FM1 app-layout checks | Missing | ISD/chip key `0x980F`, directory/XIP `0x02000120`, app at `0x4000`, app and area CRCs | **Added** |
| UFW table bounds / duplicate / overlap | Incomplete | Explicit total/payload bounds, duplicate-type, overlap and alignment checks | **Added** |
| APP_ONLY vs STOCK FULL_FLASH | Implicit trusted-loader vs official V15 branch | Separated code paths preserved; app-only never compares/writes boot head; V15 only with exact flash SHA | **Verified statically and by simulated host replay** |
| Actual staged loader | 6,898-byte SHA-pinned loader | Same SHA-pinned loader; CRC and flash readback before commit | **Preserved** |
| OTA success handshake | `0xE0000000` must reply ASCII `success` | Same; replay covers the full request stream and commit record | **Native replay PASS** |
| USB re-enumeration | HF2 host reconnect, loader `ota-FM-1_015` | Preserved; never writes to an unrecognised loader | **Browser protocol only; hardware not proven** |
| Firmware integrity | Embedded SHA and FWSC structural checks | Recompiled RC1.0.9, embedded byte-exact package, new SHA | **PASS** |

## Executed tests

Run `bash tests/ota/run_rc109_cross_firmware_audit.sh` from the repository root. Uses exact embedded native OTA C implementation and logical decoded bytes of original firmware packages; mocks only hardware flash and USB-MIDI transport.

- BEETLE RC1.0.9: accepted (native 1175 READs, 5 staged-sector erases, 1 commit).
- BEETLE with structurally valid alternate first-16-KiB header: accepted (1175 READs, 1 commit).
- Original ETCH V1.36 and V1.37 packages: accepted in native CHECK replay.
- Corrupted app byte with valid outer FWSC CRC: rejected with `-24`, zero erases/commits.
- Incorrect app XIP: rejected with `-23`, zero erases/commits.
- Untrusted loader with updated CRC wrappers but wrong SHA: rejected with `-16`, zero erases/commits.
- New RC1.0.9 standalone `.fwsc`: verified by Python UFW/SFC validator and accepted in full native replay.
- JavaScript installer layout and CRC validator: matches native acceptance/rejections.
- Three pi32v2 builds: storage+OTA, OTA pilot and read-only hardware-test successfully compiled; hardware not attached.

## Limits and remaining risks

- The official V15 .fwsc and original unmodified Felucca 1.6 / 1.1.5 .fwsc were **not available** for exact replay. No result is claimed for them. Stock restore remains pinned to an official V15 SHA-256.
- A mismatched executable loader, unfamiliar app layout, SDK/ROM requirements, or incompatible flash ownership still require a separately established restoration route. There is **no safe wildcard that accepts every custom firmware**.
- `ota-FM-1_015` is the expected identity of the staged community loader; this has not been validated on a physical FM1 under this build.
- No physical flash writes, USB re-enumeration, power-loss/restart or repeat update-away test has been performed. These are required before calling cross-firmware switching hardware-verified.
- Previously installed firmware (e.g. RC1.0.4) continues to run its **old** native CHECK. This RC1.0.9 change cannot retroactively unblock it until installed through an independently compatible path.
- The source for B1 and NERVE was not in the BEETLE project. **Their native implementations have not been audited or changed**; their web installers alone cannot prove native safety.

## Safety

Do not remove the loader SHA check or enable foreign full-flash images to expand support. Perform a full verified backup and establish an independent recovery route before physical installation. No automatic erases outside OTA staging have been added.
