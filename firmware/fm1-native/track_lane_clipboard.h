/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE: full 16-step pattern lane clipboard, independent of UI key timing.
 * Retains all note/CHD/instrument/FX/value data and may paste across patterns.
 */
#ifndef TRACK_LANE_CLIPBOARD_H
#define TRACK_LANE_CLIPBOARD_H
#include "track_core.h"
struct track_lane_clipboard {
 struct track_step step[TRACK_STEPS];
 struct track_note2_fx note2_fx[TRACK_STEPS];
 uint8_t note3[TRACK_STEPS];
 uint8_t valid;
 uint8_t count;
};
static inline int track_lane_clip_copy(struct track_lane_clipboard *clip,const struct track_pattern *pat,uint8_t lane){
 if(!clip||!pat||lane>=TRACK_CHANNELS)return 0;
 for(uint8_t i=0;i<TRACK_STEPS;i++){clip->step[i]=pat->ch[lane][i];if(lane==TRACK_DRUM_CHANNEL)clip->note2_fx[i]=pat->note2_fx[i];clip->note3[i]=pat->note3[i];}
 clip->valid=1u;clip->count=TRACK_STEPS;return 1;
}
static inline int track_lane_clip_paste(const struct track_lane_clipboard *clip,struct track_pattern *pat,uint8_t lane){
 if(!clip||!clip->valid||!pat||lane>=TRACK_CHANNELS)return 0;
 for(uint8_t i=0;i<clip->count && i<TRACK_STEPS;i++){pat->ch[lane][i]=clip->step[i];if(lane==TRACK_DRUM_CHANNEL)pat->note2_fx[i]=clip->note2_fx[i];pat->note3[i]=clip->note3[i];}
 return 1;
}
/* Inclusive contiguous range; pasted from the current edit row, without wrap. */
static inline int track_lane_clip_copy_range(struct track_lane_clipboard *clip,
 const struct track_pattern *pat,uint8_t lane,uint8_t first,uint8_t last){
 if(!clip||!pat||lane>=TRACK_CHANNELS||first>last||last>=TRACK_STEPS)return 0;
 clip->count=(uint8_t)(last-first+1u);
 for(uint8_t i=0;i<clip->count;i++){clip->step[i]=pat->ch[lane][first+i];if(lane==TRACK_DRUM_CHANNEL)clip->note2_fx[i]=pat->note2_fx[first+i];clip->note3[i]=pat->note3[first+i];}
 clip->valid=1u;return 1;
}
static inline int track_lane_clip_paste_at(const struct track_lane_clipboard *clip,
 struct track_pattern *pat,uint8_t lane,uint8_t dest){
 if(!clip||!clip->valid||!pat||lane>=TRACK_CHANNELS||dest>=TRACK_STEPS)return 0;
 for(uint8_t i=0;i<clip->count && (unsigned)(dest+i)<TRACK_STEPS;i++)
  {pat->ch[lane][dest+i]=clip->step[i];if(lane==TRACK_DRUM_CHANNEL)pat->note2_fx[dest+i]=clip->note2_fx[i];pat->note3[dest+i]=clip->note3[i];}
 return 1;
}
/* Rotate just the active steps. Drum NOTE2 stays independent of NOTE1;
 * NOTE1 carries the existing shared per-step FX/dynamics/groove fields. */
static inline int track_lane_rotate(struct track_pattern *pat,uint8_t lane,
 uint8_t column,int32_t turns){
 if(!pat||lane>=TRACK_CHANNELS||pat->length<2u||!turns)return 0;
 uint8_t length=pat->length;
 while(turns>0){
  if(lane==(TRACK_CHANNELS-1u) && column==1u){
   uint8_t saved=pat->ch[lane][length-1u].chord;
   struct track_note2_fx fx=pat->note2_fx[length-1u];
   for(uint8_t i=length-1u;i>0;i--){pat->ch[lane][i].chord=pat->ch[lane][i-1u].chord;pat->note2_fx[i]=pat->note2_fx[i-1u];}
   pat->ch[lane][0].chord=saved;pat->note2_fx[0]=fx;
  }else{
   struct track_step saved=pat->ch[lane][length-1u];
   for(uint8_t i=length-1u;i>0;i--){
    if(lane==(TRACK_CHANNELS-1u)){uint8_t note2=pat->ch[lane][i].chord;
     pat->ch[lane][i]=pat->ch[lane][i-1u];pat->ch[lane][i].chord=note2;
    }else pat->ch[lane][i]=pat->ch[lane][i-1u];
   }
   if(lane==(TRACK_CHANNELS-1u)){uint8_t note2=pat->ch[lane][0].chord;
    pat->ch[lane][0]=saved;pat->ch[lane][0].chord=note2;
   }else pat->ch[lane][0]=saved;
  }
  turns--;
 }
 while(turns<0){
  if(lane==(TRACK_CHANNELS-1u) && column==1u){
   uint8_t saved=pat->ch[lane][0].chord;
   struct track_note2_fx fx=pat->note2_fx[0];
   for(uint8_t i=0;i+1u<length;i++){pat->ch[lane][i].chord=pat->ch[lane][i+1u].chord;pat->note2_fx[i]=pat->note2_fx[i+1u];}
   pat->ch[lane][length-1u].chord=saved;pat->note2_fx[length-1u]=fx;
  }else{
   struct track_step saved=pat->ch[lane][0];
   for(uint8_t i=0;i+1u<length;i++){
    if(lane==(TRACK_CHANNELS-1u)){uint8_t note2=pat->ch[lane][i].chord;
     pat->ch[lane][i]=pat->ch[lane][i+1u];pat->ch[lane][i].chord=note2;
    }else pat->ch[lane][i]=pat->ch[lane][i+1u];
   }
   if(lane==(TRACK_CHANNELS-1u)){uint8_t note2=pat->ch[lane][length-1u].chord;
    pat->ch[lane][length-1u]=saved;pat->ch[lane][length-1u].chord=note2;
   }else pat->ch[lane][length-1u]=saved;
  }
  turns++;
 }
 return 1;
}
#endif
