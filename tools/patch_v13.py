#!/usr/bin/env python3
"""Deterministic V1.3 source patch; base V1.2. Never flashes hardware."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
def replace(path,old,new,n=1):
 p=r/path;s=p.read_text(); assert s.count(old)>=n,(path,old[:100],s.count(old)); s=s.replace(old,new,n);p.write_text(s)
# Maintain old API and historical project data; explicit non-wrapping first step on transport/Song transitions.
p='firmware/fm1-native/track_song.c'
replace(p,'uint8_t track_song_effect_mask(const struct track_song_bank *b,uint8_t pat,uint8_t step){',
'''uint8_t track_song_effect_mask_boundary(const struct track_song_bank *b,uint8_t pat,uint8_t step,uint8_t allow_wrap){''')
replace(p,'''  if(best<0){
   /* Carry a long-held Punch through the next pattern wrap. */''', '''  if(best<0 && allow_wrap){
   /* During an ordinary repeating pattern, a genuine held FX may cross the wrap.
    * At transport start or a different Song row, there is NO prior state. */''')
pth=r/p;pth.write_text(pth.read_text()+'''\nuint8_t track_song_effect_mask(const struct track_song_bank *b,uint8_t pat,uint8_t step){\n return track_song_effect_mask_boundary(b,pat,step,1u);\n}\n''')
replace('firmware/fm1-native/track_song.h','uint8_t track_song_effect_mask(const struct track_song_bank *bank,uint8_t pattern,uint8_t step);',
'''uint8_t track_song_effect_mask(const struct track_song_bank *bank,uint8_t pattern,uint8_t step);
/* At first transport step/new Song row, never infer a key pressed in a previous loop. */
uint8_t track_song_effect_mask_boundary(const struct track_song_bank *bank,uint8_t pattern,uint8_t step,uint8_t allow_wrap);''')
p='firmware/fm1-native/track_runtime.h'
# Bookkeeping for physical key down/up, tracked source per held effect for song transitions.
replace(p,'static uint8_t track_live_punch_mask,track_replay_punch_mask;', '''static uint8_t track_live_punch_mask,track_replay_punch_mask;
/* REC only saves fresh physical down edges after PLAY. Orphan OFF events are ignored.
 * Preserve the source clip even when Song Chains advances during a hold. */
static uint8_t track_recorded_punch_mask;
static uint8_t track_recorded_punch_source[8];''')
# helpers before start audio, no memory allocation, only on main when interrupts off
replace(p,'static void track_sync_punch_params(void){', '''static void track_punch_record_reset(void){
 track_recorded_punch_mask=0u;
 for(uint8_t i=0u;i<8u;i++)track_recorded_punch_source[i]=TRACK_PATTERNS;
}
/* Called with IRQs off, on explicit STOP. Any captured down must have a
 * matching recorded up; a stopped take cannot become an indefinite loop. */
static void track_punch_record_finish(void){
 if(track_state.record_armed){
  for(uint8_t fx=0u;fx<8u;fx++)if(track_recorded_punch_mask&(1u<<fx)){
   uint8_t src=track_recorded_punch_source[fx];
   uint8_t next=(uint8_t)((track_state.transport.play_step+1u)%TRACK_STEPS);
   if(src<TRACK_PATTERNS && track_song_record_value(&track_song,src,next,fx,0u,255u)==0)
    track_state.dirty=1u;
  }
 }
 track_punch_record_reset();
}
static void track_sync_punch_params(void){''')
# additional bool for replay with boundary when song row change
replace(p,'track_replay_punch_mask=track_song_effect_mask(&track_song,track_song_punch_pattern(&track_song,&track_state),0u);',
'''track_replay_punch_mask=track_song_effect_mask_boundary(&track_song,track_song_punch_pattern(&track_song,&track_state),0u,0u);''')
replace(p,'track_replay_punch_mask=track_song_effect_mask(&track_song,track_song_punch_pattern(&track_song,&track_state),track_state.transport.play_step);',
'''track_replay_punch_mask=track_song_effect_mask_boundary(&track_song,
        track_song_punch_pattern(&track_song,&track_state),track_state.transport.play_step,
        (uint8_t)(track_state.transport.play_step!=0u ||
                  !track_song.enabled || !track_song.composite_active ||
                  track_song.position==song_position_before_wrap));''')
# must introduce song_position_before_wrap before onwrap in IRQ
replace(p,'''   track_tick(&track_state);
   if(track_state.transport.play_step==0u && track_song.enabled){''',
'''   track_tick(&track_state);
   uint8_t song_position_before_wrap=track_song.position;
   if(track_state.transport.play_step==0u && track_song.enabled){''')
# ensure only tracking blank resets when starting, stop finalize and baseline
replace(p,'''    if(!track_state.record_armed){fm1_irq_off();track_live_punch_mask=0u;track_state.punch_mask=track_replay_punch_mask;fm1_irq_enable_all();}''',
'''    fm1_irq_off();
    if(!track_state.record_armed){track_punch_record_finish();track_live_punch_mask=0u;
      track_state.punch_mask=track_replay_punch_mask;}
    else track_punch_record_reset();
    fm1_irq_enable_all();
    /* Debounced held keys are not new Punch presses on arming. */
    track_old_note_mask=fm1_in.notes;''')
replace(p,'''    beetle_mute_cancel(&track_mute_queue);track_live_mutes.pending=track_live_mutes.pending_values=0u;track_live_solos.pending=track_live_solos.pending_values=0u;track_stop(&track_state);track_live_punch_mask=0u;track_replay_punch_mask=0u;track_sync_punch_params();''',
'''    beetle_mute_cancel(&track_mute_queue);track_live_mutes.pending=track_live_mutes.pending_values=0u;track_live_solos.pending=track_live_solos.pending_values=0u;
    track_punch_record_finish();track_stop(&track_state);
    track_live_punch_mask=0u;track_replay_punch_mask=0u;track_state.punch_mask=0u;track_sync_punch_params();''')
replace(p,'''    track_play(&track_state);
   }
   fm1_irq_enable_all();''',
'''    /* Start with no previous physical down events; only newly struck keys record. */
    track_punch_record_reset();track_old_note_mask=fm1_in.notes;
    track_live_punch_mask=0u;track_replay_punch_mask=0u;track_state.punch_mask=0u;
    track_play(&track_state);
   }
   fm1_irq_enable_all();''')
# Start audio after load or ISR-start preflight; reset initial UI record state on new session
replace(p,'track_live_punch_mask=0u;track_replay_punch_mask=0u;\n track_swing_pending_mask=0u;',
          'track_live_punch_mask=0u;track_replay_punch_mask=0u;track_punch_record_reset();\n track_swing_pending_mask=0u;')
# Handle keydown recording only when playback has actually started audio; send OFF only for recorded physical down.
replace(p,'''    if(track_state.record_armed&&track_state.transport.playing){
     fm1_irq_off();
     (void)track_punch_add(&track_rec_events,track_state.transport.play_step,i,after);
     (void)track_song_record_value(&track_song,track_state.pattern_slot,
       track_state.transport.play_step,i,after,
       (after&&i<5u)?track_punch_base_param[i]:255u);
     track_state.dirty=1u;
     fm1_irq_enable_all();
    }''',
'''    if(track_state.record_armed&&track_state.transport.playing&&track_audio_started){
     fm1_irq_off();
     uint8_t source=after?track_song_punch_pattern(&track_song,&track_state):track_recorded_punch_source[i];
     if(source<TRACK_PATTERNS && (after || (track_recorded_punch_mask&(1u<<i)))){
      if(track_song_record_value(&track_song,source,track_state.transport.play_step,i,after,
            (after&&i<5u)?track_punch_base_param[i]:255u)==0){
       (void)track_punch_add(&track_rec_events,track_state.transport.play_step,i,after);
       if(after){track_recorded_punch_mask|=(uint8_t)(1u<<i);track_recorded_punch_source[i]=source;}
       else{track_recorded_punch_mask&=(uint8_t)~(1u<<i);track_recorded_punch_source[i]=TRACK_PATTERNS;}
       track_state.dirty=1u;
      }
     }
     fm1_irq_enable_all();
    }''')
replace(p,'''     (void)track_song_record_value(&track_song,track_state.pattern_slot,
             track_state.transport.play_step,fx,2u,(uint8_t)value);''',
'''     uint8_t source=track_recorded_punch_mask&(1u<<fx)?track_recorded_punch_source[fx]:TRACK_PATTERNS;
     if(source<TRACK_PATTERNS && track_song_record_value(&track_song,source,
             track_state.transport.play_step,fx,2u,(uint8_t)value)==0)track_state.dirty=1u;''')
# Avoid ghost from stale old_note_mask in page changes: entering armed punch UI initializes baseline from current note bitmap elsewhere (existing branch handles).
# EDIT patch number visible on all pages and correlated to 1-based tracker.
replace(p,'''  if(track_osc_bank){
   static const char *const waves[]''',
'''  /* Explicit patch number mirrors the 1-based INST value in TRACKER. */
  char patch_label[24]="INST 1 / 4";
  patch_label[5]=(char)('1'+(track_input_instrument[track_state.transport.edit_channel]&3u));
  lcd_fill(139u,29u,95u,13u,TRACK_DARK);
  track_pix_text(patch_label,144u,30u,TRACK_PINK);
  if(track_osc_bank){
   static const char *const waves[]''')
# update HELP
replace('firmware/fm1-native/beetle_help.h','"EDIT: INSTRUMENT SETTINGS",','"EDIT: INSTRUMENT SETTINGS",\n "EDIT TOP: INST 1-4 = TRACKER NO.",')
replace('firmware/fm1-native/beetle_help.h','"GLOBAL PUNCH SOURCE: CH1",','"GLOBAL PUNCH SOURCE: CH1",\n "REC SONG: WRITES ACTIVE CH1 PAT",\n "FX NEW SONG ROW: NO GHOST CARRY",')
# Remove misleading old comment
replace(p,'''/* Channel VOL/MUTE/SOLO are audible via MIDI velocities.
 * PAN is currently stored/editable only; stereo routing is not implemented. */''',
'''/* All six lanes are mixed with independent stereo PAN. The post-master
 * stereo output is passed unchanged to USB UAC and analogue DMA. */''')
# soft power headroom bounded, constant max 8 voices and sample safe, preserves single note.
(r/'firmware/fm1-native/beetle_voice_headroom.h').write_text('''/* SPDX-License-Identifier: GPL-3.0-only
 * A bounded power-style polyphonic headroom curve in Q8. One audible voice
 * stays at unity; chords preserve energy without old near-linear attenuation.
 * Worst-case 8 simultaneously full-drive voices remain below hard saturation.
 */
#ifndef BEETLE_VOICE_HEADROOM_H
#define BEETLE_VOICE_HEADROOM_H
#include <stdint.h>
static inline uint16_t beetle_voice_divisor(uint8_t voices){
 static const uint16_t q8[9]={256u,256u,352u,432u,512u,576u,640u,704u,768u};
 return q8[voices>8u?8u:voices];
}
#endif
''')
replace('firmware/fm1-native/track_chip.c','#include "beetle_punch_timing.h"','#include "beetle_punch_timing.h"\n#include "beetle_voice_headroom.h"')
replace('firmware/fm1-native/track_chip.c','uint32_t target=(1u+(uint32_t)s->phrase_max_voices/2u)*256u;',
'''uint32_t target=beetle_voice_divisor(s->phrase_max_voices);''')
# Build a reusable equal-power panning table (no float/math on target). at center 256 both, full side 362, zero opposite.
import math
panlut=[round(256*math.sqrt(2*(1-x/100))) for x in range(101)]
(r/'firmware/fm1-native/beetle_stereo_pan.h').write_text('''/* SPDX-License-Identifier: GPL-3.0-only
 * Equal-power pan gain lookup (Q8), without FP or sqrt in the render ISR.
 * Center: 256/256; L100: 362/0; R100: 0/362.
 */
#ifndef BEETLE_STEREO_PAN_H
#define BEETLE_STEREO_PAN_H
#include <stdint.h>
static const uint16_t beetle_pan_q8[101]={\n'''+','.join(str(x)+'u' for x in panlut)+'''\n};
static inline void beetle_stereo_pan_gain(int32_t pan,uint16_t *l,uint16_t *r){
 if(pan<-100)pan=-100;if(pan>100)pan=100;
 uint32_t x=(uint32_t)(pan<0?-pan:pan);
 uint16_t low=beetle_pan_q8[x];
 uint16_t high=beetle_pan_q8[100u-x];
 *l=pan<0?high:low;
 *r=pan>0?high:low;
}
#endif
''')
replace('firmware/fm1-native/track_chip.c','#include "beetle_voice_headroom.h"','#include "beetle_voice_headroom.h"\n#include "beetle_stereo_pan.h"')
replace('firmware/fm1-native/track_chip.c',
''' int16_t mix_vol[TRACK_SYNTH_CHANNELS],mix_pan[TRACK_SYNTH_CHANNELS];''',
''' int16_t mix_vol[TRACK_SYNTH_CHANNELS];
 uint16_t pan_left[TRACK_SYNTH_CHANNELS],pan_right[TRACK_SYNTH_CHANNELS];''')
replace('firmware/fm1-native/track_chip.c',
'''  mix_pan[c]=mix?mix->ch[c].pan:0;''',
'''  beetle_stereo_pan_gain(mix?mix->ch[c].pan:0,&pan_left[c],&pan_right[c]);''')
replace('firmware/fm1-native/track_chip.c',
'''   const int32_t vol=mix_vol[v->channel],pan=mix_pan[v->channel];''',
'''   const int32_t vol=mix_vol[v->channel];''')
replace('firmware/fm1-native/track_chip.c',
'''   left+=((int64_t)mono*(100-pan))/100;
   right+=((int64_t)mono*(100+pan))/100;''',
'''   left+=((int64_t)mono*pan_left[v->channel])>>8;
   right+=((int64_t)mono*pan_right[v->channel])>>8;''')
replace('firmware/fm1-native/track_drum.c','#include "track_core.h"','#include "track_core.h"\n#include "beetle_stereo_pan.h"')
replace('firmware/fm1-native/track_drum.c',
''' const int32_t pan=m?m->pan:0;
 for(uint32_t f=0;f<frames;f++){''',
''' uint16_t pan_left,pan_right;
 beetle_stereo_pan_gain(m?m->pan:0,&pan_left,&pan_right);
 for(uint32_t f=0;f<frames;f++){''')
replace('firmware/fm1-native/track_drum.c',
'''  int32_t l=dst[f*2u]+mono*(100-pan)/100;
  int32_t r=dst[f*2u+1u]+mono*(100+pan)/100;''',
'''  int32_t l=dst[f*2u]+(int32_t)(((int64_t)mono*pan_left)>>8);
  int32_t r=dst[f*2u+1u]+(int32_t)(((int64_t)mono*pan_right)>>8);''')
# stereo USB signal: avoid accidental mono, retain native stereo; explicitly label coupling.
replace('firmware/fm1-native/audio_target.c','native_usb_stereo[2*i]=left;native_usb_stereo[2*i+1]=right;',
'''/* Same post-master stereo L/R samples as DAC; never copy native_mono.
    * UAC streams this exact interleaved pair, including six mixer PAN values. */
   native_usb_stereo[2*i]=left;native_usb_stereo[2*i+1]=right;''')
print('V1.3 patched')
