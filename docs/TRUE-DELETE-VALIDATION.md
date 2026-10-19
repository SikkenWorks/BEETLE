# BEETLE RC1.01 Project Transfer V2 · true slot deletion

- Protocol 0x59: slot-specific erase of both 12 KiB A/B banks, with full preflight ownership and integrity checks. Reply 0x66 after verification.
- Full journal scanner refuses foreign or ambiguous data before any erase.
- Current open project is protected; must load NEW or another project on FM1 first.
- Power-loss and flash-write safety are NOT hardware certified.
- Firmware compile: native JieLi pi32v2, BEETLE storage/updater build successful.
- FWSC package structurally validated; protected boot bytes and OTA loader retained.
- Native host flash test: dual bank clear, other slots unaffected, failure and foreign data rejection.
- Web Project Manager supports V1 for import/export, V2 for true deletion.
- **Do not assume a successful simulation proves safe real hardware erasure or recovery.**

Final release verification:

- Embedded WebInstaller package is **byte-identical** to `BEETLE-1.01-RC1.01-TRUE-DELETE-EXPERIMENTAL.fwsc`.
- Firmware SHA-256: `2b79c6bf29fa65adb6f061aae4dfea23e13b4abf3829c96aacaee1abbc1e55de`.
- 3 JieLi pi32v2 builds successful: hardware-test (read-only), pilot (no storage writes), storage+updater (V2 DELETE).
- Native journal isolation/failure tests and browser protocol V1/V2 checks pass.
- Preserved all 393 original source files.
- Full legacy regression suite and on-device hardware tests were **not run** to completion for this release.
