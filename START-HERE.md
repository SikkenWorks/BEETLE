# BEETLE 1.01 RC1.0.8

**Complete GitHub-ready source release** · SikkenWorks

This archive is based on the supplied `BEETLE-main` repository and includes the full native FM1 firmware source, current HF2 WebInstaller, WebSimulator, Project Manager, tests, GitHub Pages configuration and assets.

## Start here

| File | Purpose |
| --- | --- |
| `README.md` | Complete BEETLE documentation and controls table |
| `CHANGELOG.md` | Version history and RC1.0.8 changes |
| `WebInstaller.html` | Current RC1.0.8 HF2 installer with embedded firmware |
| `BEETLE-1.01-RC1.0.8-EXPERIMENTAL.fwsc` | Exact firmware package embedded in the installer |
| `simulator.html` | Current WebSimulator with project import/export and tracker editing |
| `BEETLE-Project-Manager.html` | Current project-slot manager and backup/restore UI |
| `firmware/fm1-native/` | Native FM1 firmware and toolchain build script |
| `tests/` | Regression and focused tests |
| `RC1.0.8-CLEAR-LANE-VALIDATION.md` | New clear-lane feature and software validation |

## New tracker control

Hold **SEQ + rightmost black key** in the tracker while stopped to request **CLEAR CURRENT LANE**. The confirmation defaults to NO. Use K1 for YES/NO, SEL to confirm or HOME to cancel. Only the current pattern's active lane is erased, including independent drum NOTE2 FX. Undo remains available. See the unchanged-format controls table in `README.md`.

## Updating GitHub

The ZIP has **no additional parent folder**: `README.md`, the three webapps and the source directories sit directly at its root. Extract the ZIP, copy its **contents** to the root of your existing BEETLE repository clone, keeping the local `.git` folder, and commit/push with GitHub Desktop. Alternately run:

```bash
git add -A
git commit -m "BEETLE 1.01 RC1.0.8"
git push
```

Copying all contents into a clone and using `git add -A` also removes files that were deliberately deleted from the release. The GitHub browser upload has a per-batch file limit; GitHub Desktop or command-line Git is more reliable for full-folder updates.

## Firmware caution

Software builds, simulation, CRC/SHA checks and package verification do **not** prove successful physical flashing. Flash storage and OTA recovery remain experimental. A verified full backup and independent recovery path are recommended before installing. The supplied `.fwsc` and `WebInstaller.html` are matched to one another, but physical FM1 operation has not been validated in this release.
