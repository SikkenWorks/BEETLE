/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE: full 16-step pattern lane clipboard, independent of UI key timing.
 * Retains all note/CHD/instrument/FX/value data and may paste across patterns.
 */
#ifndef TRACK_LANE_CLIPBOARD_H
#define TRACK_LANE_CLIPBOARD_H
#include "track_core.h"
struct track_lane_clipboard {
 struct track_step step[TRACK_STEPS];
 uint8_t valid;
};
static inline int track_lane_clip_copy(struct track_lane_clipboard *clip,const struct track_pattern *pat,uint8_t lane){
 if(!clip||!pat||lane>=TRACK_CHANNELS)return 0;
 for(uint8_t i=0;i<TRACK_STEPS;i++)clip->step[i]=pat->ch[lane][i];
 clip->valid=1u;return 1;
}
static inline int track_lane_clip_paste(const struct track_lane_clipboard *clip,struct track_pattern *pat,uint8_t lane){
 if(!clip||!clip->valid||!pat||lane>=TRACK_CHANNELS)return 0;
 for(uint8_t i=0;i<TRACK_STEPS;i++)pat->ch[lane][i]=clip->step[i];
 return 1;
}
#endif
