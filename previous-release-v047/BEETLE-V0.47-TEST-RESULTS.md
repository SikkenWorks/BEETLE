# BEETLE V0.47 tests

- Original GIF: 240x240; two 100 ms frames; original files included unmodified.
- Frame 0 native C render vs GIF RGB565: 57,600 / 57,600 exact pixels.
- Frame 1 native C render vs GIF RGB565: 57,600 / 57,600 exact pixels.
- Original V0.42 visual regression, updated for new boot art, passes. OTA animation independently retained.
- Read-only native JieLi build + read-only static audit: PASS.
- OTA pilot JieLi build: PASS.
- Experimental storage/OTA JieLi build: PASS.
- V0.46/V0.45/V0.44/V0.43/V0.41/V0.40 host regressions completed and passed in new run; the deeper historical suite timed out and was not claimed complete.
- V0.47 OTA mocked tests: stock V15, earlier BEETLE and loader resume PASS; unknown and altered firmware refused.
- Firmware structural/header/product/hash/app-content checks: PASS.
- Real hardware boot display, frame timing and OTA flashing: NOT TESTED.
