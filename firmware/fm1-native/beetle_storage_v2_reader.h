/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE Storage V2 native read adapter. Original BEETLE B2SB/B2RC format.
 * All entry points run in main/storage context, NEVER from audio IRQ.
 * Only the dedicated 0x97000..0xDEFFF region may be read.
 */
#ifndef BEETLE_STORAGE_V2_READER_H
#define BEETLE_STORAGE_V2_READER_H
#include <stdint.h>
#define BEETLE_V2_BASE 0x97000u
#define BEETLE_V2_BYTES 0x48000u
#define BEETLE_V2_INDEX_BYTES 4096u
#define BEETLE_V2_PROJECTS 12u
#define BEETLE_V2_SAMPLES 13u
#define BEETLE_V2_READ_MAX 256u
/* read returns zero on success. Every read must be done outside audio IRQ. */
typedef int (*beetle_v2_read)(void *ctx,uint32_t physical,uint8_t *dst,uint32_t n);
struct beetle_v2_device {
 beetle_v2_read read;
 void *context;
 uint32_t index_address;
 uint32_t generation;
};
struct beetle_v2_ref {uint32_t offset,length,crc;};
struct beetle_v2_sample {uint32_t generation,byte_length,crc;uint16_t chunks;uint16_t index_offset;};
/* V1.4.19: the two namespaces are independent despite sharing the 288KiB
 * physical volume. This lightweight read-only audit detects any overlap
 * between project and sample record SECTORS (not only byte ranges), and
 * requires each sample chunk to occupy its own full erase sector. No heap.
 * 0 = isolated, -2 = invalid/colliding layout, -1 = unreadable index.
 * Does not claim that the physical flash has been migrated or can be written.
 */
struct beetle_v2_domains {
 uint16_t project_sectors;
 uint16_t sample_sectors;
 uint16_t assigned_projects;
 uint16_t assigned_samples;
};
int beetle_v2_audit_domains(struct beetle_v2_device *device,
 struct beetle_v2_domains *report,uint8_t *scratch);
/* Exact versioned 32-byte V2 ownership marker. Used by firmware and the
 * host migration-image parity test. Does not write flash. */
void beetle_v2_make_marker(uint8_t out[32]);
/* scratch is caller-owned 4096-byte storage buffer, no heap or audio scratch. */
int beetle_v2_open(struct beetle_v2_device *device,beetle_v2_read read,void *context,uint8_t *scratch);
/* 1 = assigned, 0 = unused, negative = invalid/corrupted. */
int beetle_v2_sample_info(struct beetle_v2_device *device,uint8_t slot,struct beetle_v2_sample *info,uint8_t *scratch);
/* CRC-verifies complete source chunk BEFORE returning bytes. Potentially slow:
 * call from background/prefetch only, NOT DMA/USB interrupts. Returns 0 on success. */
int beetle_v2_sample_read(struct beetle_v2_device *device,uint8_t slot,uint32_t offset,
 uint8_t *out,uint32_t length,uint8_t *scratch);
/* Fast background chunk fetch: index is CRC-checked and a complete sample
 * record is verified exactly once. Caller can retain 4068 bytes in RAM and
 * serve many short prefetch reads from that immutable generation. Never IRQ. */
int beetle_v2_sample_chunk_read(struct beetle_v2_device *device,uint8_t slot,
 const struct beetle_v2_sample *expected,uint16_t part,uint8_t *out,
 uint16_t *written,uint8_t index[4096]);
/* Check a committed project record and unpack into caller-provided RAM.
 * 1 = project present, 0 = empty, negative = invalid. */
/* Index-only NEW PROJECT allocation. Scans each slot's two metadata
 * references once. Never decodes an occupied project, never trusts an
 * unreadable/corrupt index as free. Return 0..11 free, 12 full, -1 error. */
/* NEW PROJECT: 2 x 4KiB CRC-validated index copies, no extra flash read.
 * Caller provides 8192 bytes. 0..11 free; 12 full; -1 invalid/unreadable. */
int beetle_v2_audit_and_first_unused_project_fast(beetle_v2_read read,
 void *context,struct beetle_v2_domains *report,uint8_t double_scratch[8192]);
int beetle_v2_first_unused_project(struct beetle_v2_device *device,uint8_t *scratch);
/* Shared CRC-checked index read: validate project/sample domain ownership
 * and find a free project without a second 4KiB NOR read. -1 on failure. */
int beetle_v2_audit_and_first_unused_project(struct beetle_v2_device *device,
 struct beetle_v2_domains *report,uint8_t *scratch);
int beetle_v2_project_read(struct beetle_v2_device *device,uint8_t slot,uint8_t *out,
 uint32_t capacity,uint32_t *written,uint8_t *scratch,
 uint8_t *packed_work,uint32_t packed_capacity);
#endif
