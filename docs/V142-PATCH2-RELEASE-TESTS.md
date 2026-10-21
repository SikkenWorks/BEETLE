# BEETLE V1.4.2 DMA PATCH 2 — Installer Repair & Build Validation

**Result:** the previously bundled V1.3.3 firmware was replaced with the actual V1.4.2 DMA Patch 2 package inside both WebInstallers. Native code/DSP and standalone FWSC remain identical to the previous Patch 2 candidate.

## Offline checks completed

| Check | Result |
|---|---|
| Root installer and `web/` copy | Byte-identical |
| Embedded FWSC and included candidate | Byte-identical |
| Package SHA256 | `a82ddfd02c1f26ee1923fff59ecd864eef8b7cdd806a75f2aa0c403a9089b6aa` |
| Product identity | `FM-1_2123` |
| Package CRC / loader integrity | Pass |
| Protected original boot/header | Preserved relative to V1.4.1 |
| Tampered/old firmware rejection | Pass in browser-core JS offline tests |
| Simulated CHECK -> OTA loader -> WRITE -> reboot | Pass |
| Simulated denied CHECK -> never WRITE | Pass |
| Unknown loader -> never WRITE | Pass |
| Global bank round-trip, CRC and replacement | Pass |
| DSP 12 types / 5 VA shape PCM matrix | Pass, hash `df02ac8b` |
| 120-second, 44.1kHz synthetic DSP stress | Pass, hash `1183fe60`, up to 8 synth+6 drumvoices |
| NUGGIE each 12 types over 64 loops | Pass with max 8 synth+4 drumvoices |
| JieLi native read-only linker build | Pass, app size 443,204 bytes |
| JieLi experimental storage+OTA+USB build | Pass, app size 484,008 bytes; SHA-256 `5268e4a4f1724809d69df372c24c722e37668251d2cdf835ef09b62380ba31fb` |
| Rebuilt application matches FWSC manifest | Pass |
| Hardware WebMIDI flash | NOT tested |
| Hardware DMA under load | NOT tested |
| Physical recovery/stock restore | NOT tested |
| Visual browser smoke | Blocked by execution environment policy; offline JS core validated separately |

## Physical test still required

Host measurements cannot reproduce the FM1's ISR timing, shared bus contention or DMA deadlines. The firmware continues to reduce voice budget after missed deadlines and can enter AUDIO STOP. Full stability under heavy loads has **not been demonstrated**. Flash only with backup and proven independent recovery path. Use `docs/V142-INSTALLER-INTEGRITY.md` and `docs/DMA-V142-PATCH2-REPORT.md` for the validation matrix.
