# BEETLE — Clean release rule (every iteration)

Every BEETLE iteration must produce a self-contained, clean **complete source** archive from the current tree. Never accumulate old generated firmware binaries, versioned HTML/README duplicates, build logs, compiler objects, temporary fixture outputs, `__pycache__`, `.git` packs or large obsolete release archives in the distributed ZIP.

**Retain:** complete active firmware and web source, every working function, portable source build tools and historical test dependencies, active native/source licenses and credits, diagnostic/OTA tools, reference assets required for verification, README in its existing GitHub style, full changelog and migration documentation. Do not silently remove features or required tests merely to shrink the archive.

**Canonical top-level delivery files:** README.md, CHANGELOG.md, LICENSE, BEETLE-MANAGER.html, BEETLE-Project-Manager.html, WebInstaller.html, simulator.html, current `.fwsc` and the current build manifest (plus the existing official images/assets). Version-numbered duplicates are distributed as optional separate direct-download files only, not multiple times inside the source ZIP.

**Validation before release:** run the BD4S loop/PCM/restart tests and appropriate other tests; compile native firmware; verify protected OTA header/loader; verify embedded WebInstaller .fwsc SHA-256 and the All-in-One Manager's three embedded tools; inspect the ZIP manifest for unexpected older files.

Create the archive using `python tools/package_clean_release.py --version V1.4.28 --output /path/to/release.zip`, changing the version for each iteration.
