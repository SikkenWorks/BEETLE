# BEETLE V0.45 — verified local test results

Hardware verification: **NOT DONE**; all results are cross-compilation, host, mocked NOR or simulated browser/OTA.

- `tests/track/run_v045.sh`: **PASS** under ASan + UBSan. Independent 12-slot A/B journaling; overwrite slot 6 while comparing all other 11 banks byte-for-byte. Stable rendered energy over 14 consecutive note-loop retriggers; no denominator based on historical voice_count. Sustain FILTER over empty steps, replace with VOLUME, retain value, PITCH overlay without new onset, reset on note, release on STP.
- Full chained V0.44–V0.35 regressions: **PASS**, including V8 project format, older V6/V7 migration, ROM drums, hi-hat choke, Punch-In snapshot/reverse, mixer, song, and journal torn-write recovery. Previously unavailable V0.31/0.34 fixtures remain unavailable as documented.
- JieLi pi32v2 cross-builds: **PASS** for pilot, experimental storage OTA, and read-only diagnostic.
- HAL/boot audit: **PASS**, 19 tracked upstream files unchanged; read-only linked writer symbol audit: **PASS**.
- Storage experimental app: **385368 bytes**, spare **196196 bytes** in application code space.
- `.fwsc`: **PASS** format, exact app bytes, ETCH V1.37 first 16 KiB and OTA loader preserved, ID `FM-1_2045`.
- Embedded WebInstaller verification and simulated stock V15 route: **PASS**; rejects tampered package and unknown identity. Real stock/OTA installation and restore **NOT TESTED**.

**Known limits:** 12 KiB per project journal bank; extremely full six-lane 48-pattern projects can be rejected with capacity error without erasing flash. No physical safety or recovery guarantee. First-use formatting wipes the entire reserved project volume and might destroy other firmware data.
