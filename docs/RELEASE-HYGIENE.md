# BEETLE — Clean release rule (every iteration)

Every BEETLE iteration must produce a self-contained, clean **complete source** archive from the current tree. Never accumulate old generated firmware binaries, versioned HTML/README duplicates, build logs, compiler objects, temporary fixture outputs, `__pycache__`, `.git` packs or large obsolete release archives in the distributed ZIP.

**Retain:** complete active firmware and web source, every working function, portable source build tools and historical test dependencies, active native/source licenses and credits, diagnostic/OTA tools, reference assets required for verification, README in its existing GitHub style, full changelog and migration documentation. Do not silently remove features or required tests merely to shrink the archive.

**Canonical top-level delivery files:** README.md, CHANGELOG.md, LICENSE, BEETLE-MANAGER.html, BEETLE-Project-Manager.html, WebInstaller.html, simulator.html, current `.fwsc` and the current build manifest (plus the existing official images/assets). Version-numbered duplicates are distributed as optional separate direct-download files only, not multiple times inside the source ZIP.

**Validation before release:** run the BD4S loop/PCM/restart tests and appropriate other tests; compile native firmware; verify protected OTA header/loader; verify embedded WebInstaller .fwsc SHA-256 and the All-in-One Manager's three embedded tools; inspect the ZIP manifest for unexpected older files.

Create the archive using `python tools/package_clean_release.py --version V1.4.28 --output /path/to/release.zip`, changing the version for each iteration.

## README vs CHANGELOG (mandatory every iteration)

**README.md is a timeless user guide, not a running release log.** Keep the existing BEETLE banner/logo, GitHub links/shields, practical feature overview, installation and usage instructions, mappings, safety guidance, credits, upstream attributions and licenses. Update descriptions when behavior changes, but write them in current-tense product/manual style, not as "New in V..." or version-by-version fixes.

**CHANGELOG.md contains all per-version information**: new features, corrections, regressions, resource changes, test results and compatibility caveats. Add the newest entry at the top and retain chronological history. The README may link to CHANGELOG.md and show the current firmware version, but it must never duplicate its release-note sections.

The clean packaging tool fails if the top-level README contains a versioned release-notes heading.

## On-device HELP and WebSimulator parity (mandatory every iteration)

When changing or introducing hardware controls, keys, menus, steps, recording,
project formats, project management, selection, effects or other user-facing
functions, review `firmware/fm1-native/beetle_help.h` **in the same iteration**.
Revise both the firmware HELP and the active `simulator.html` HELP_LINES to match
current behavior. Do not copy version histories into HELP or README: specific
release changes belong in CHANGELOG.md. Recompile the firmware whenever native
HELP changes so that the `.fwsc` actually includes the new text.

**Always run** `python tests/track/test_help_contract.py` before compiling or
packaging. This validates that both HELP sets are identical, all rows fit the
240-pixel display (36 characters maximum), the 8-bit indexing does not overflow,
and essential controls, NOTE3 and SONG MODE instructions remain covered.
The clean release packer runs the contract automatically. An unreviewed HELP
change should block a release, not become stale technical documentation.

## Global usersample step-1 playback regression

The clean release must include and execute `tests/track/test_v1437_sample_start.c` to verify that a prefetch-waiting drum voice does not advance its position or envelope until the first PCM window is available, and that the sequencer start-barrier is bounded. Re-test native NOTE3 / V26 projects, HELP parity and stereo/multitrack capture whenever modifying the sample bridge or transport timing.

## Required on each release

Review embedded FM1 HELP when control behavior or user-visible workflow changes; keep the WebSimulator HELP in sync. Put all per-version feature/bugfix notes ONLY into CHANGELOG. Rebuild WebInstaller embedded firmware, validate Project Manager against the active project/sample format, and package one current firmware with source, tests, assets, credits and licenses (no previous releases, Git objects or temporary build files).
