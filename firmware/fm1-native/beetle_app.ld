/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* FELUCCA memory map.
 * Code + constants execute in place from XIP (app.bin offset 0 = 0x02000120).
 * 0x01C00000..0x01C06000 holds .ram_text (code that must run while the flash
 * is busy: the flash driver); 0x01C06000..0x01C08000 stays for RAM-run debug.
 * RAM  : .data/.bss (96 KiB)
 * POOL : big zero-initialised buffers (FX lines, canvases), section .pool
 * STACKS with 256-byte guard bands; NOINIT survives resets and UBOOT entry.
 * 0x01C7FD50.. is reserved: boot_info, boot-state mailbox, vector table. */
MEMORY
{
    XIP    (rx)  : ORIGIN = 0x02000120, LENGTH = 0x8DFBC
    RAMTEXT (rwx): ORIGIN = 0x01C00000, LENGTH = 0x6000
    UACRAM (rw)  : ORIGIN = 0x01C06000, LENGTH = 0x1F00 /* preserves debug mailbox at 0x01C07F00 */
    RAM    (rwx) : ORIGIN = 0x01C08000, LENGTH = 96K
    POOL   (rw)  : ORIGIN = 0x01C20000, LENGTH = 0x54000
    NOINIT (rw)  : ORIGIN = 0x01C7C000, LENGTH = 0x3D50
}
SECTIONS
{
    .text : { KEEP(*(.text.crt0)) *(.text*) *(.rodata*) . = ALIGN(4); _etext = .; } > XIP
    .ram_text : { _rt_start = .; KEEP(*(.ram_text*)) . = ALIGN(4); _rt_end = .; } > RAMTEXT AT > XIP
    _rt_load = LOADADDR(.ram_text);
    .data : { . = ALIGN(4); _data_start = .; *(.data*) *(.sdata*) . = ALIGN(4); _data_end = .; } > RAM AT > XIP
    _data_load = LOADADDR(.data);
    .bss (NOLOAD) : { _bss_start = .; *(.bss*) *(.sbss*) *(COMMON) . = ALIGN(4); _bss_end = .; } > RAM
    /* BEETLE data, guard and LCD pixels are separate and cannot alias. */
    .pool (NOLOAD) : { _pool_start = .; *(.pool*) . = ALIGN(64); } > POOL
    .usb_audio_ram (NOLOAD) : { _usb_audio_start = .; *(.usb_audio_ram*) . = ALIGN(4); _usb_audio_end = .; } > UACRAM
    .beetle_song (NOLOAD) : { . = ALIGN(64); _beetle_song_start = .; KEEP(*(.beetle_song*)); . = ALIGN(64); _beetle_song_end = .; } > POOL
    .beetle_mem_gap (NOLOAD) : { _beetle_mem_gap_start = .; . = . + 128; _beetle_mem_gap_end = .; } > POOL
    .beetle_lcd_frame (NOLOAD) : { . = ALIGN(64); _beetle_frame_start = .; KEEP(*(.beetle_lcd_frame*)); . = ALIGN(64); _beetle_frame_end = .; _pool_end = .; } > POOL
    .noinit (NOLOAD) : { *(.noinit*) } > NOINIT
    /* Dedicated scratch after the established retained boot/noinit layout.
     * All data are overwritten before use; no overlap with ROM boot mailboxes. */
    .beetle_edit_scratch (NOLOAD) : { . = ALIGN(16); _beetle_edit_start = .;
       KEEP(*(.beetle_edit_scratch)); . = ALIGN(16); _beetle_edit_end = .; } > NOINIT
    ASSERT(_beetle_edit_end <= ORIGIN(NOINIT) + LENGTH(NOINIT), "BEETLE: edit scratch overlaps boot mailbox")
    _guard0 = 0x01C74000; _ustack_lo = 0x01C74100; _ustack_top = 0x01C7A000;
    _guard1 = 0x01C7A000; _sstack_lo = 0x01C7A100; _sstack_top = 0x01C7C000;
    ASSERT((_beetle_mem_gap_end - _beetle_mem_gap_start) >= 128, "BEETLE: memory isolation gap missing")
    ASSERT(_beetle_song_end <= _beetle_mem_gap_start, "BEETLE: sequencer/state overlap")
    ASSERT(_beetle_mem_gap_end <= _beetle_frame_start, "BEETLE: framebuffer overlap")
    ASSERT(_pool_end <= _guard0, "BEETLE: RAM pool intrudes on stack")
    /DISCARD/ : { *(.comment) *(.note*) }
}
