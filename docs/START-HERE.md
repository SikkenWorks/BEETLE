# BEETLE V1.3 — Source and release files

SikkenWorks, open-source M-VAVE FM1 tracker. The root of this ZIP is ready for GitHub. V1.3 is a software-tested experimental release; physical FM1 and OTA validation are pending.

- `README.md`: current controls, features, credits, warnings
- `CHANGELOG.md`: all release changes
- `WebInstaller.html`: current SHA-256 pinned V1.3 installer
- `BEETLE-V1.3-USB-AUDIO-EXPERIMENTAL.fwsc`: embedded experimental firmware
- `simulator.html`: standalone browser simulator
- `BEETLE-Project-Manager.html`: project backup and management
- `firmware/fm1-native/`: native firmware source and JieLi build script
- `tests/track/run_v13_regression.sh`: release tests
- `docs/V1.3-VALIDATION.md`: findings, limitations and release verification

To rebuild: supply the JieLi toolchain via `JIELI_TOOLCHAIN`, then run
`python3 firmware/fm1-native/build.py --beetle-storage-lab --updater --usb-audio`.
The `.fwsc` packer preserves reference firmware protected areas and verifies its
hashes. A successful build is NOT evidence of a safe flash or tested recovery.

Update via GitHub Desktop or a local Git clone. Extract the ZIP root into the
repository, then commit and push. Do not modify the original protected loader.
