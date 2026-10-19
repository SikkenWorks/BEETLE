# BEETLE V0.77 — Firmware Freedom qualification

Date: 2026-10-15. Base: complete V0.76 source.

## Built and verified in this environment

- Native read-only (`--hardware-test`), OTA pilot (`--pilot`) and A/B storage/updater (`--beetle-storage-lab --updater`) JieLi builds: **PASS**. Identity `FM-1_2077`. Native sound engine, flash layout and project schema V16 unchanged.
- Protected 16 KiB header and OTA loader: reused byte-for-byte from pinned ETCH V1.37 package; firmware package has a matching app audit.
- The WebInstaller embeds the exact newly packaged `.fwsc` bytes and hardcoded SHA-256.
- `node source/tests/ota/test_firmware_freedom_v077.cjs`: exact ETCH V1.36 and V1.37 hashes/identities, known BEETLE source restriction, checksum mismatch and wrong-target rejection, unknown firmware and loader-only denial, simulated CHECK → loader WRITE → matching reboot: **PASS**.
- `bash source/tests/track/run_v076.sh`: legacy functional regressions including audio, V16 project codec, instrument library and A/B storage fault injections: **PASS**.
- `node source/tests/ota/test_stock_install_v076.cjs`: previous stock / BEETLE OTA regression: **PASS**.
- Chromium UI tested through `set_content`: default OFF, visibility, target changes, two opt-ins and reset: **PASS**. Sandbox blocks direct `file://` page navigation and a secure-origin Web MIDI/crypto integration test; do not present it as fully browser-tested live flashing.

## Approved package profiles

- **Original V15:** SHA-256 matching existing stock restore implementation; user supplies their own original `.fwsc`.
- **ETCH 1.36**: exact image from the user-supplied original ETCH V1.36 release. The first 16 KiB and OTA-loader body match the BEETLE V1.37 reference.
- **ETCH 1.37**: exact SHA-pinned reference package already in `source/reference/`.
- **Felucca 1.6.1 and others:** upstream release metadata exists, but the binary cannot be independently downloaded and audited in this runtime. There is no bypass: those controls remain disabled.

## Hardware sign-off

BEETLE betas are tested on real FM1 hardware by the maintainer prior to GitHub publication. This chat performed only software tests. Before publishing V0.77 with Firmware Freedom, qualify stock V15 and each ETCH target on a physical device, verify full project backup, CHECK→loader→WRITE→boot, and confirm that a separately independent recovery method is available. Never infer physical safety from matching hashes alone.
