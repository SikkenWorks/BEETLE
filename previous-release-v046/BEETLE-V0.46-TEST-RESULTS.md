# BEETLE V0.46 · testresultaten

- Host regression V0.46: **PASS**, inclusief Punch-In ON/OFF + ALGORITHM-opname, step-playback, V9-project-roundtrip, V8 drumfader-migratie en stabiele release-headroom.
- Gehele V0.45/V0.44/... regressieketen: **PASS**. De oude testverwachtingen voor standaard drumfader 100 en projectformat V8 zijn aangepast naar actuele gewenste default / versie.
- JieLi FM1 read-only full-app build: **PASS** (inclusief forbidden-symbol audit), **365688 bytes**.
- JieLi FM1 pilot OTA build: **PASS**. Behoudt de storage-write lock.
- JieLi FM1 experimentele opslag + OTA build: **PASS**, **387016 bytes**, linker POOL en stack gecontroleerd.
- FWSC-packer: **PASS**; referentie ETCH V1.37 SHA, beschermde eerste 16KiB en OTA-loader identiek, app-extract matcht de build, geldige container; product `FM-1_2046`.
- Webinstaller: **PASS** (gesimuleerde V15/BEETLE OTA-routes, checksum, onbekende versie geweigerd, gewijzigde binary geweigerd).
- **Niet uitgevoerd:** echte OTA-upgrade op hardware, oscilloscoop-/luistertest op FM1, fysieke brick/recovery-test, duurzaamheidstests van projectopslag.

Kijk in `logs/` voor de originele uitvoer. Vergelijk SHA-256 in `BEETLE-V0.46-STORAGE-EXPERIMENTAL-MANIFEST.json`.

**Let op:** pass van softwaretests is geen bewijs dat installatie op de FM1 veilig is.
