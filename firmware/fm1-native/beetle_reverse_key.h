/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE RC1.02 – reverse key/scale search, independent of playback KEYLOCK.
 * Analyses every pattern, including the in-RAM actively edited pattern.
 * Does not mutate the project or guess a unique tonal centre from a pitch set.
 */
#ifndef BEETLE_REVERSE_KEY_H
#define BEETLE_REVERSE_KEY_H
#include "track_song.h"
#include <stdint.h>
#define BEETLE_REVERSE_CANDIDATES 108u
struct beetle_reverse_candidate {uint8_t root,mode,fit;uint16_t outside;};
struct beetle_reverse_report {
 struct beetle_reverse_candidate ranked[BEETLE_REVERSE_CANDIDATES];
 uint32_t total_weight;
 uint32_t events;
 uint16_t outside_pitches;
 uint8_t exact_count;
};
/* Identical mode order and intervals to track_core.c, deliberately kept
 * chromatic: NEVER call track_chord_keylocked() to infer an original key. */
static const uint8_t beetle_reverse_scales[9][7]={
 {0,2,4,5,7,9,11},{0,2,3,5,7,9,10},{0,1,3,5,7,8,10},
 {0,2,4,6,7,9,11},{0,2,4,5,7,9,10},{0,2,3,5,7,8,10},
 {0,1,3,5,6,8,10},{0,2,3,5,7,8,11},{0,2,3,5,7,9,11}};
static uint16_t beetle_reverse_mask(uint8_t root,uint8_t mode){
 uint16_t mask=0;
 if(root>=12u||mode>=9u)return 0;
 for(uint8_t j=0;j<7u;j++)mask|=(uint16_t)(1u<<((root+beetle_reverse_scales[mode][j])%12u));
 return mask;
}
static uint8_t beetle_reverse_popcount(uint16_t x){
 uint8_t n=0;while(x){n+=(uint8_t)(x&1u);x>>=1u;}return n;
}
/* Never treat drum NOTE1 or NOTE2 as melodic evidence.
 * Roots are weighed twice, other chord tones once; repeated occurrences
 * matter. STP and --- are ignored. Out-of-range chords are harmless. */
static void beetle_reverse_analyse(const struct track_song_bank *song,
 const struct track_engine *engine,struct beetle_reverse_report *out){
 uint32_t weights[12]={0},roots[12]={0};
 if(!song||!engine||!out)return;
 out->total_weight=0;out->events=0;out->exact_count=0;out->outside_pitches=0;
 for(uint8_t pat=0u;pat<TRACK_PATTERNS;pat++){
  const struct track_pattern *p=pat==engine->pattern_slot?&engine->active:&song->patterns[pat];
  uint8_t length=p->length>=1u&&p->length<=TRACK_STEPS?p->length:TRACK_STEPS;
  for(uint8_t ch=0u;ch<TRACK_SYNTH_CHANNELS;ch++)for(uint8_t row=0u;row<length;row++){
   const struct track_step *s=&p->ch[ch][row];
   if(s->note>=128u)continue;
   const uint8_t pitch=(uint8_t)(s->note%12u);
   weights[pitch]+=2u;roots[pitch]++;out->total_weight+=2u;out->events++;
   if(s->chord){
    uint8_t chord[5]={0};uint8_t count=track_chord(s->note,s->chord,chord);
    /* chord[0] is the root, already counted above. */
    for(uint8_t n=1;n<count&&n<5u;n++){
     weights[chord[n]%12u]++;out->total_weight++;
    }
   }
  }
 }
 for(uint8_t mode=0u;mode<9u;mode++)for(uint8_t root=0u;root<12u;root++){
  uint32_t outside=0;const uint16_t mask=beetle_reverse_mask(root,mode);
  for(uint8_t pc=0u;pc<12u;pc++)if(!(mask&(1u<<pc)))outside+=weights[pc];
  uint8_t fit=out->total_weight?(uint8_t)((100u*(out->total_weight-outside)+out->total_weight/2u)/out->total_weight):0u;
  struct beetle_reverse_candidate c={root,mode,fit,(uint16_t)(outside>65535u?65535u:outside)};
  /* Insertion order: fewest notes outside first, then favour a frequently
   * played tonic for ambiguous pitch sets, then favour current scale. */
  uint8_t at=(uint8_t)(mode*12u+root);
  while(at){
   const struct beetle_reverse_candidate *before=&out->ranked[at-1u];
   if(before->outside<c.outside)break;
   if(before->outside==c.outside){
    if(roots[before->root]>roots[c.root])break;
    if(roots[before->root]==roots[c.root]){
     uint8_t existing=(uint8_t)(before->root==engine->harmony.root&&before->mode==engine->harmony.mode);
     uint8_t preferred=(uint8_t)(root==engine->harmony.root&&mode==engine->harmony.mode);
     if(existing>=preferred)break;
    }
   }
   out->ranked[at]=out->ranked[at-1u];at--;
  }
  out->ranked[at]=c;
 }
 if(out->events){
  for(uint8_t n=0u;n<BEETLE_REVERSE_CANDIDATES;n++){
   if(out->ranked[n].outside!=0u)break;
   out->exact_count++;
  }
  const struct beetle_reverse_candidate *best=&out->ranked[0];
  uint16_t mask=beetle_reverse_mask(best->root,best->mode);
  for(uint8_t pc=0u;pc<12u;pc++)if(weights[pc]&&!(mask&(1u<<pc)))out->outside_pitches|=(uint16_t)(1u<<pc);
 }
}
#endif
