# BEETLE V0.47 | Pixel-Exact Animated Startup

Built from the complete V0.46 source. All earlier audio, Punch-In recording, mixing, project, storage and OTA features remain.

The supplied `LoadingScreen.gif` (240x240, two 100-ms frames) replaces the startup's old scaled icon. The tree/background and magenta beetle are reproduced at their source pixel coordinates, with no artistic edits, added text, rescaling or interpolation. Native FM1 LCD uses RGB565. A host regression compares every one of the 57,600 pixels of each generated frame to the original GIF after RGB565 conversion: both frames matched. The original six source image assets are included under `source/assets/loading-screen/` and a reproducible generator under `source/tools/generate_beetle_boot.py`.

Three cross-compiled JieLi variants (readonly, pilot, experimental storage) linked and validated. Software OTA packaging and mocked installation checks passed. No physical FM1 test was performed; storage and OTA installation remain experimental and potentially destructive.

See `BEETLE-V0.47-README-NL.md` for paths and test commands.
