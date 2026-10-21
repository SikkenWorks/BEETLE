/* TRACK V0.18 · A/B journal. GPL-3.0-only.
 * Physical storage is enabled ONLY in standalone BEETLE_NATIVE_STORAGE=1.
 * Initial takeover requires destructive five-second physical confirmation.
 */
#ifndef TRACK_JOURNAL_H
#define TRACK_JOURNAL_H
#include <stdint.h>
#include "track_project_store.h"
#ifndef BEETLE_NATIVE_STORAGE
#define BEETLE_NATIVE_STORAGE 0
#endif
#define TRACK_JOURNAL_MAGIC 0x4A4B5254u /* TRKJ */
#define TRACK_JOURNAL_BANK_BYTES 12288u
#define TRACK_JOURNAL_HEADER_BYTES 32u
#define TRACK_JOURNAL_SLOTS TRACK_PROJECTS
#define TRACK_JOURNAL_BANKS 2u
/* 12 projects x 2 A/B banks x 12KiB = 288KiB. V4 compact codec
 * always fits: 48 patterns and max event/song data. Any existing ETCH/Felucca
 * objects in the dedicated partition REQUIRE an explicit destructive migration. */
#define TRACK_JOURNAL_PROVISION_BYTES (TRACK_JOURNAL_BANK_BYTES*TRACK_JOURNAL_SLOTS*TRACK_JOURNAL_BANKS)
enum {TRACK_JOURNAL_OK=0, TRACK_JOURNAL_IO=-1,TRACK_JOURNAL_INVALID=-2,
      TRACK_JOURNAL_FOREIGN=-3,TRACK_JOURNAL_CAPACITY=-4,
      TRACK_JOURNAL_NO_PROJECT=-5, TRACK_JOURNAL_HARDWARE_LOCKED=-6};
/* read, erase and program callbacks are mockable for host qualification.
 * Each callback must validate slot, bank, offset and physical sector ownership.
 */
struct track_journal_io {
 void *ctx;
 int (*read)(void *,uint8_t slot,uint8_t bank,uint32_t offset,uint8_t *dst,uint32_t n);
 int (*erase)(void *,uint8_t slot,uint8_t bank);
 int (*program)(void *,uint8_t slot,uint8_t bank,uint32_t offset,const uint8_t *src,uint32_t n);
};
struct track_journal_status {uint32_t generation;uint32_t payload_bytes;uint8_t active_bank;uint8_t available;};
/* Read only; fallback to valid older bank after incomplete/corrupt update. */
int track_journal_load(const struct track_journal_io *,uint8_t slot,
 struct track_store_project *out,uint8_t *scratch,uint32_t scratch_size,
 struct track_journal_status *status);
/* Fast, READ-ONLY browser preview (only two bank headers and project prefixes).
 * Titles are provisional; loading still performs the entire CRC validation.
 * Never use preview results to authorize a flash erase/write. */
int track_journal_preview(const struct track_journal_io *,uint8_t slot,char title[13]);
/* Destructive slot-only DELETE. Validate both complete A/B banks and full ownership
 * before erasing. An interrupted erase can leave an unreadable slot; never
 * treat power loss as successful deletion. No partition-wide erase. */
int track_journal_delete(const struct track_journal_io *,uint8_t slot);
/* Explicit recovery: erase only positively identified BEETLE records when project CRC is damaged; refuse foreign data. */
int track_journal_delete_corrupt(const struct track_journal_io *,uint8_t slot);
/* Read-only audit of ALL 24 full banks, independent from global marker.
 * Repair requires >=1 BEETLE-owned bank and ZERO foreign/unknown banks. */
int track_journal_audit_owned_volume(const struct track_journal_io *);
/* Host fault-injection and BEETLE-only flash journal. With native storage disabled the FM1 API RETURNS HARDWARE_LOCKED. */
int track_journal_save(const struct track_journal_io *,uint8_t slot,
 const struct track_store_project *p,uint8_t *scratch,uint32_t scratch_size,
 struct track_journal_status *status);
#endif
