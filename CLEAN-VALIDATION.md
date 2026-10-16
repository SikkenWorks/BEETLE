# RC1.01 clean release validation

- All 444 files inside `source/` from the latest Project Manager V1.2 release preserved.
- SHA-256 and byte identity of the `.fwsc` and embedded WebInstaller package verified.
- `WebInstaller.html` at root and in `source/web/` are byte-for-byte identical.
- `simulator.html` and `BEETLE-Project-Manager.html` match their source/web versions.
- `test_web_crypto.cjs`: passed (MIDI 7-bit framing, V2 password-free AES-GCM, tamper detection, V1 migration).
- `test_web_browser.py`: passed (12-slot mocked connection).
- `test_connection_diagnostics.py`: passed (project-transfer, legacy firmware, no reply, 400/1280 px).
- `run_rc1.sh`: RC1 live mixer, V0.87–V0.72 and additional storage/error-injection tests reported PASS; full run **timed out** and was not completed, so no claim of full-suite pass. Some prior compiler warnings remain in source.
- No FM1 device was accessed, flashed or tested during the cleanup.
