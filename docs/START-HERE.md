# BEETLE 1.01 RC1.0.11

**Complete GitHub-ready source release** · SikkenWorks

This archive is based on the supplied `BEETLE-main` repository and includes the full native FM1 firmware source, current HF2 WebInstaller, WebSimulator, Project Manager, tests, GitHub Pages configuration and assets.

## Start here

| File | Purpose |
| --- | --- |
| `README.md` | Complete BEETLE documentation and controls table |
| `CHANGELOG.md` | Version history and RC1.0.11 changes |
| `WebInstaller.html` | Current RC1.0.11 HF2 installer with embedded firmware |
| `BEETLE-1.01-RC1.0.11-EXPERIMENTAL.fwsc` | Exact firmware package embedded in the installer |
| `simulator.html` | Current WebSimulator with project import/export and tracker editing |
| `BEETLE-Project-Manager.html` | Current project-slot manager and backup/restore UI |
| `firmware/fm1-native/` | Native FM1 firmware and toolchain build script |
| `tests/` | Regression and focused tests |
| `docs/RC1.0.11-SONG-CHAINS-VALIDATION.md` | New clear-lane feature and software validation |

## SONG CHAINS controls

In Song Mode (ARP), **POS** shows the song position while **CH1 CH2 CH3 CH4 CH5 DR REP** label the independently configurable sources. K1 chooses a row, K2 selects a channel or REP, and K3 chooses a pattern 01–48 (`--` to silence) or repeat count. OCT+ inserts a row whose six channels all default to that position's pattern; existing customized rows remain unchanged. The native V19 schema migrates old V18 songs automatically. All controls are in the existing README table and both HELP screens.

## Updating GitHub

The ZIP has **no additional parent folder**: `README.md`, the three webapps and the source directories sit directly at its root. Extract the ZIP, copy its **contents** to the root of your existing BEETLE repository clone, keeping the local `.git` folder, and commit/push with GitHub Desktop. Alternately run:

```bash
git add -A
git commit -m "BEETLE 1.01 RC1.0.11"
git push
```

Copying all contents into a clone and using `git add -A` also removes files that were deliberately deleted from the release. The GitHub browser upload has a per-batch file limit; GitHub Desktop or command-line Git is more reliable for full-folder updates.

## Rebuilding the exact experimental package

The matched RC1.0.10 package is stored **only as a build reference** in `reference/BEETLE-RC1.0.10-BOOT-OTA-REFERENCE.fwsc.reference`; do not use it as an installer. After building the storage+OTA app, run:

```bash
python tools/build_rc1011_fwsc.py reference/BEETLE-RC1.0.10-BOOT-OTA-REFERENCE.fwsc.reference build/track-fm1-native-beetle-storage-lab-updater/app.bin BEETLE-1.01-RC1.0.11-EXPERIMENTAL.fwsc
```

The packer requires the exact RC1.0.10 reference identity, verifies application SHA-256 and CRC, copies its original protected header and OTA loader, and writes `FIRMWARE-MANIFEST.json`. The WebInstaller's embedded firmware and pinned SHA need to be updated after any recompile. No physical device tests have been performed here.

## Firmware caution

Software builds, simulation, CRC/SHA checks and package verification do **not** prove successful physical flashing. Flash storage and OTA recovery remain experimental. A verified full backup and independent recovery path are recommended before installing. The supplied `.fwsc` and `WebInstaller.html` are matched to one another, but physical FM1 operation has not been validated in this release.
