# BEETLE V1.4.24 — Storage V2 migration via Project Manager

## Supported workflow

The Project Manager uses V5 USB commands `0x5D` (read-only layout/preflight) and
`0x5E` (explicit conversion). The V5 command is implemented in the native
firmware under `beetle_project_transfer.inc`. It is **not** a firmware update,
and the GUI must not offer conversion to older protocol versions.

The firmware calculates a 32-bit FNV-1a fingerprint over 12 ordered descriptors:
`slot (u8), present (u8), raw TRK1 length (u16 LE), raw TRK1 CRC32 (u32 LE)`.
For empty slots all descriptor bytes other than the slot are zero. The host
reads all original projects with per-chunk integrity checks and computes the
same fingerprint. It verifies before the first erase and requires two explicit
confirmations including a checked, independently saved encrypted ZIP backup.

The conversion operation checks legacy BEETLE ownership, rescans projects,
rejects current dirty/playing sessions, erases only the BEETLE 288 KiB volume,
initializes an empty CRC-checked B2SB superblock, and programs the V2 marker
at the end. The manager restores occupied slots and reads them back as V24,
then verifies all 12 slots.

## Failures and limitations

This is NOT an atomic conversion: legacy journal and V2 partition cannot both
fit in 288 KiB while retaining all data. Power loss after erase can destroy
legacy data, even though the firmware/header/OTA regions stay untouched.
Use the downloaded original encrypted `.beetle` backups and the independently
verified FM1 recovery path to restore data. A browser download event alone
is not proof that the ZIP is saved: the user must check it manually.
If the operation fails during restore, the V2 partition may already be active
with only some project slots restored. Do not repeatedly initialize: restore
remaining `.beetle` files through the normal import action.

This version is built and browser-simulated only; no physical FM1 flash or
USB transport was available. Product/package identity `FM-1_2123` unchanged.

## Recovery UI

A **RESTORE V2 BACKUP ZIP** button is enabled when a verified Storage V2 volume
is connected. The manager accepts its own uncompressed migration ZIP; before
writing, it validates ZIP entry CRC, manifest slot inventory, AES-GCM project
authentication, project CRC and the original overall device fingerprint. It
restores projects only after explicit consent. This never re-formats the volume.
