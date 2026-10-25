/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE USB-C recording: bounded recovery from a delayed DMA block.
 * No extra PCM buffers, allocations, or floating point in IRQ context.
 */
#ifndef BEETLE_USB_CAPTURE_RECOVERY_H
#define BEETLE_USB_CAPTURE_RECOVERY_H
#include <stdint.h>
/* USB capture is a recorder, not live monitoring. Preserve ~5ms of headroom
 * for an occasional delayed 128-frame render, rather than just ~2.4ms.
 * Both formats continue using the existing 512-frame rings. */
#define BEETLE_UAC_PRIME_44100 224u
#define BEETLE_UAC_PRIME_48000 244u
#define BEETLE_UAC_FADE_FRAMES 24u
/* Check subtraction only after checking for a stale/impossible position.
 * An unsigned underflow here used to incorrectly advertise free space. */
static inline int beetle_uac_can_write(uint32_t write_pos,uint32_t read_pos,
                                        uint32_t frames,uint32_t capacity){
 uint32_t used=write_pos-read_pos;
 return used<=capacity && frames<=capacity-used;
}
/* Exceptional path only: ramp from the most recent PCM value to silence when
 * a delayed DMA render actually starves USB. Avoid the indefinite DC hold. */
static inline int16_t beetle_uac_fade_tail(int16_t sample,uint32_t frames_left,
                                            uint32_t frames_total){
 if(!frames_total)return 0;
 return (int16_t)(((int32_t)sample*(int32_t)frames_left)/(int32_t)frames_total);
}
static inline uint32_t beetle_uac_fade_stereo(uint32_t sample,uint32_t left,
                                               uint32_t total){
 int16_t l=beetle_uac_fade_tail((int16_t)(sample&65535u),left,total);
 int16_t r=beetle_uac_fade_tail((int16_t)(sample>>16u),left,total);
 return (uint32_t)(uint16_t)l|((uint32_t)(uint16_t)r<<16u);
}
/* Startup/restart fade to avoid a click when the primed silence ends. */
static inline int16_t beetle_uac_fade_in(int16_t sample,uint32_t frames_done){
 if(frames_done>=BEETLE_UAC_FADE_FRAMES)return sample;
 return (int16_t)(((int32_t)sample*(int32_t)frames_done)/
                   (int32_t)BEETLE_UAC_FADE_FRAMES);
}
#endif
