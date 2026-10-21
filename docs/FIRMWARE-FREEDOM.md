# BEETLE V0.77 — Firmware Freedom

The installer has a separate **disabled-by-default** mode. No unapproved firmware receives any MIDI write commands.

| Target | Policy | Package origin |
|---|---|---|
| Official stock V15 | Approved hash-only fallback; stock SHA-256 checks performed in browser and BEETLE OTA | Supply original verified stock `.fwsc` |
| ETCH V1.36 | SHA-256 pinned, identical protected 16KiB header and OTA loader | Exact `ETCH-FM1-V1.36.fwsc` from provided original release |
| ETCH V1.37 | SHA-256 pinned, identical protected 16KiB header and OTA loader | Exact V1.37 release reference |
| Felucca | Not yet approved, disabled in selector | Only source ZIP was supplied, no verified compatible `.fwsc` |
| Other custom firmware | Not yet approved, disabled | Must receive explicit package audit and maintainer qualification |

## Operator steps
1. Back up the full device and projects using independently verified tools.
2. Enable Firmware Freedom by checkbox. Select target and original `.fwsc` file.
3. Wait for the exact SHA-256, product and protected-header check. Unknown / modified packages are rejected.
4. Connect a known running BEETLE device, not an unknown firmware or an already-active loader.
5. Check the separate Freedom acknowledgement and the general risk checkbox. Confirm the update on screen.
6. The installer requires the device to finish the CHECK stage, enter the loader, finish the WRITE stage, reboot and report the expected target ID.

There is **no universal cross-firmware OTA protocol** and no bypass for incompatible protected loaders. Current validation proves package integrity and simulated behaviour, not an independent repair path if firmware fails to boot.

BEETLE public betas undergo physical FM1 testing by their maintainer. V0.77 cross-firmware target routes must be tested and recorded separately before marking them release-ready.
