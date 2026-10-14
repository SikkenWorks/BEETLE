/* SPDX-License-Identifier: GPL-3.0-only
 * TRACK native on-device runtime. LCD, rotary encoders, I2S DMA, watchdog,
 * analog output ramp and recovery all originate in ETCH V1.28/Felucca.
 * Not a browser or hardware emulator.
 * V0.33 adds physical A/B flash persistence, 12 slots; initial formatting
 * erases legacy ETCH/Felucca user data ONLY after five-second consent.
 */
#ifndef TRACK_NATIVE_RUNTIME_H
#define TRACK_NATIVE_RUNTIME_H
#include "track_core.h"
#include "track_sustain_fx.h"
#include "track_name.h"
#include "track_navigation.h"
#include "track_lane_clipboard.h"
#include "track_ui_controls.h"
#include "track_mixer.h"
#include "track_swing.h"
#include "track_punch_events.h"
#include "track_song.h"
#include "track_drum.h"
#include "track_master_fx.h"
#include "track_project_store.h"
#include "track_journal.h"
#define TRACK_PINK 0xF81Au
#define TRACK_DATA_PINK 0xF81Au
#define TRACK_BRIGHT 0xFDDEu
#define TRACK_ROW_BG 0x3005u
#define TRACK_WHITE 0xFFFFu
#define TRACK_DARK 0x0000u
#include "beetle_keyboard_octave.h"
#include "beetle_help.h"
static struct track_engine track_state;
static struct track_drum_engine track_drum __attribute__((section(".beetle_song"),aligned(16)));
static struct track_master_settings track_master_cfg;
static struct track_master_fx_state track_master_dsp __attribute__((section(".beetle_song"),aligned(64)));
static uint8_t track_drum_component,track_drum_field;
static uint8_t track_master_focus;
static uint8_t track_master_chain_edit;
/* Channel VOL/MUTE/SOLO are audible via MIDI velocities.
 * PAN is currently stored/editable only; stereo routing is not implemented. */
/* track_mix is declared before audio_target.c so ISR can render it. */
static struct track_punch_events track_rec_events;
/* 48 patterns and 64 song entries live in RAM and are journalled to flash on SAVE. */
/* TRACK 0.8: place 48-pattern RAM song bank in large POOL to keep the
 * 96 KiB low-RAM section within the FM1 linker memory map. */
static struct track_song_bank track_song __attribute__((section(".beetle_song"),aligned(64)));
static uint8_t track_live_punch_mask,track_replay_punch_mask;
/* Saved mixer-style Punch preset is separate from temporary playback automation. */
static uint8_t track_punch_base_param[5];
static void track_sync_punch_params(void);
static uint8_t track_punch_focus=255u; /* most recently depressed FX key */
static uint32_t track_seq_since;
static uint8_t track_seq_fired;
static struct track_lane_clipboard beetle_lane_clip;
/* Encoder audition: preview auto-releases; never writes a pattern. */
static uint8_t track_scroll_preview_active,track_scroll_preview_channel,track_scroll_preview_note;
static uint32_t track_scroll_preview_until;
static uint32_t track_old_note_mask;
static struct track_name_editor track_editor;
static char track_title[TRACK_NAME_MAX+1u];
static uint8_t track_named,track_page,track_menu_index,track_fx_cursor,track_osc_cursor,track_osc_bank;
static uint8_t track_help_offset; /* V0.54: scroll state, not project data. */
static uint8_t track_audio_started,track_prev_notes[TRACK_CHANNELS][5];
static uint8_t track_lane_instrument[TRACK_SYNTH_CHANNELS];
static uint8_t track_prev_note_count[TRACK_CHANNELS];
static uint32_t track_audio_clock,track_last_poll,track_last_paint,track_home_since;
static uint8_t track_swing_pending_mask;
static uint32_t track_swing_pending_at[TRACK_CHANNELS];
static uint32_t track_boot_input_unlock_ms,track_last_encoder_ms;
static uint8_t track_last_encoder_id;
static int32_t track_last_encoder_delta;
static uint32_t track_recovered_state_faults;
static uint8_t track_home_fired;
static uint8_t track_song_preplay_row,track_song_follow_active;
/* V0.13: REC hold opens the current-pattern punch event editor. */
static uint32_t track_rec_since;
static uint8_t track_rec_long,track_punch_edit_row,track_punch_edit_index;
static uint8_t track_fx_drum_lane,track_edit_poly;
static int16_t track_type_queued;
/* Global live keyboard octave is native_controls.octave. Shared by EDIT and
 * TRACKER, never written into project notes unless a note is entered.
 * Using chord-aware key-up handling prevents OCT-+OCT+ from shifting pitch. */
static struct etch_octave_chord track_key_oct_chord;
static uint8_t track_preset_last_note=60u;
static uint32_t track_boot_splash_until;
static uint8_t track_slot_seen[TRACK_PROJECTS],track_overwrite_wait;
static int track_storage_result=TRACK_JOURNAL_HARDWARE_LOCKED;
static uint32_t track_storage_confirm_since;
/* V0.34: defer LOAD/NEW until unsaved work has been explicitly confirmed. */
/* 1 = open LOAD browser, 2 = NEW session, 3 = load browser-selected slot. */
static uint8_t track_unsaved_action;
/* A SAVE to the project we actually opened/created needs no redundant
 * overwrite confirmation. This is NOT inferred from the menu slot selector. */
static uint8_t track_session_saved_slot; /* 255 = no verified current slot */
static uint32_t track_saved_toast_until;
static uint8_t track_storage_busy_kind; /* 1 save, 2 load */
static uint8_t track_storage_progress_done,track_storage_progress_total,track_storage_just_initialized,track_storage_phase;
/* 0=scan pending,1=project,2=empty,3=error; no flash scanning from the LCD renderer. */
static uint8_t track_load_status[TRACK_PROJECTS],track_load_scan;
static char track_load_titles[TRACK_PROJECTS][TRACK_NAME_MAX+1u];
static int track_load_feedback;

/* Each held tracker-keyboard note is gated on release, just as in EDIT.
 * 255 = unused; latch channel because SELECT can turn before key release. */
static uint32_t track_preview_mask;
static uint8_t track_preview_channel[27],track_preview_note[27];
static uint32_t track_edit_prev_keys;
static uint8_t track_edit_voice_key[27],track_edit_voice_note[27];
static uint32_t track_edit_order[27],track_edit_serial;
static uint32_t track_edit_last_onset_ms[27];
/* Four shared instrument patches for the five synth lanes.
 * Native RAM-only instrument editor parameters. These follow the WebSimulator
 * 8-row UI. TYPE/DUTY/CUTOFF and channel level are implemented in DSP/mixer;
 * ADSR time/value parameters remain visible/editable but their DSP wiring
 * is tracked separately (not claimed functional on FM1 in this release). */
struct track_instrument_ui_values {
 uint8_t attack,decay,sustain,release,duty,volume;
 uint16_t cutoff;
};
static struct track_instrument_ui_values track_ui_inst[4];
#if BEETLE_NATIVE_STORAGE
/* Scratch and project staging MUST NOT be placed on the tiny firmware stack.
 * This memory is never touched from an audio ISR and is reused for loads. */
static struct track_journal_status beetle_last_journal;
static void beetle_project_capture(uint8_t slot){
 track_song_capture(&track_song,&track_state);
 beetle_staged.song=track_song;beetle_staged.mixer=track_mix;
 for(uint8_t i=0u;i<TRACK_DRUM_COUNT;i++)beetle_staged.drum[i]=track_drum.components[i];
 beetle_staged.master=track_master_cfg;
 for(uint8_t i=0u;i<5u;i++)beetle_staged.punch_param[i]=track_punch_base_param[i];
 beetle_staged.harmony=track_state.harmony;beetle_staged.bpm=track_state.transport.bpm;
 beetle_staged.slot=slot;
 for(uint8_t i=0u;i<12u;i++)beetle_staged.title[i]=track_title[i]?track_title[i]:' ';
 beetle_staged.title[12]=0;
 for(uint8_t i=0u;i<4u;i++){
  struct track_store_instrument *v=&beetle_staged.instruments[i];
  v->type=track_chip.osc_type[i];v->attack=track_ui_inst[i].attack;
  v->decay=track_ui_inst[i].decay;v->sustain=track_ui_inst[i].sustain;
  v->release=track_ui_inst[i].release;v->duty=track_ui_inst[i].duty;
  v->volume=track_ui_inst[i].volume;v->cutoff=track_ui_inst[i].cutoff;
  v->va=track_chip.patches[i];
 }
}
static void beetle_project_apply(void){
 /* Caller stopped playback, interrupted audio, and validated full TRK1 CRC. */
 track_song=beetle_staged.song;track_mix=beetle_staged.mixer;
 for(uint8_t i=0u;i<TRACK_DRUM_COUNT;i++)track_drum.components[i]=beetle_staged.drum[i];
 track_master_cfg=beetle_staged.master;
 for(uint8_t i=0u;i<5u;i++)track_punch_base_param[i]=beetle_staged.punch_param[i];
 track_live_punch_mask=track_replay_punch_mask=0u;track_sync_punch_params();
 track_state.harmony=beetle_staged.harmony;
 track_state.transport.bpm=beetle_staged.bpm;
 track_state.transport.playing=0u;track_state.transport.play_step=0u;
 track_state.pattern_slot=0u;track_state.active=track_song.patterns[0];
 track_state.project_slot=beetle_staged.slot;
 track_chip_silence(&track_chip);track_drum_stop(&track_drum);
 for(uint8_t c=0;c<TRACK_SYNTH_CHANNELS;c++)track_lane_instrument[c]=(uint8_t)(c&3u);
 for(uint8_t i=0u;i<4u;i++){
  const struct track_store_instrument *v=&beetle_staged.instruments[i];
  track_chip.osc_type[i]=v->type;track_chip.patches[i]=v->va;
  track_chip.env_attack[i]=v->attack;track_chip.env_decay[i]=v->decay;
  track_chip.env_sustain[i]=v->sustain;track_chip.env_release[i]=v->release;
  track_ui_inst[i].attack=v->attack;track_ui_inst[i].decay=v->decay;
  track_ui_inst[i].sustain=v->sustain;track_ui_inst[i].release=v->release;
  track_ui_inst[i].duty=v->duty;track_ui_inst[i].volume=v->volume;
  track_ui_inst[i].cutoff=v->cutoff;
  /* V0.25: restore the actual DSP duty/filter state, not just screen
   * values. Matches the knob mapping used in the FM1 instrument editor. */
  track_chip.duty[i]=(uint8_t)(v->duty<19u?0u:v->duty<38u?1u:v->duty<63u?2u:3u);
  track_chip.cutoff[i]=(uint8_t)(v->cutoff/75u>250u?250u:v->cutoff/75u);
 }
 for(uint8_t c=0;c<TRACK_SYNTH_CHANNELS;c++){
  uint8_t instrument=track_lane_instrument[c];
  const struct track_instrument_ui_values *ins=&track_ui_inst[instrument];
  track_chip.duty[c]=(uint8_t)(ins->duty<19u?0u:ins->duty<38u?1u:ins->duty<63u?2u:3u);
  track_chip.cutoff[c]=(uint8_t)(ins->cutoff/75u>250u?250u:ins->cutoff/75u);
  track_chip.fx_type[c]=0u;track_chip.fx_value[c]=50u;
  track_chip.drive[c]=0u;track_chip.step_gain[c]=100u;
  track_chip.cut_after[c]=0xFFFFFFFFu;track_chip.fx_samples[c]=0u;
 }
 track_drum.step_gain=100u;
 for(uint8_t i=0u;i<TRACK_NAME_MAX;i++)track_title[i]=beetle_staged.title[i];
 track_title[TRACK_NAME_MAX]=0;track_named=1u;track_slot_seen[track_state.project_slot]=1u;track_session_saved_slot=track_state.project_slot;
 track_state.dirty=0u;
}
static int beetle_project_save_staged(void){
 /* HAL write permit is held only for this verified transaction. */
 beetle_write_permit=1u;
 int rc=track_journal_save(&beetle_hal_journal_io,beetle_staged.slot,
      &beetle_staged,beetle_codec_buffer,sizeof(beetle_codec_buffer),&beetle_last_journal);
 beetle_write_permit=0u;
 return rc;
}
static int beetle_project_load_slot(uint8_t slot){
 int owner=beetle_hal_probe_ownership();
 if(owner!=BEETLE_CLAIM_EXISTING)return owner==BEETLE_CLAIM_FOREIGN?TRACK_JOURNAL_FOREIGN:TRACK_JOURNAL_NO_PROJECT;
 int rc=track_journal_load(&beetle_hal_journal_io,slot,&beetle_staged,
     beetle_codec_buffer,sizeof(beetle_codec_buffer),&beetle_last_journal);
 if(rc==TRACK_JOURNAL_OK){fm1_irq_off();beetle_project_apply();fm1_irq_enable_all();}
 return rc;
}
#endif
/* Called under the audio IRQ lock (or in its audio callback). Replay takes
 * precedence only if the FX is not physically held; knob moves remain live.
 * Never overwrite user presets with recorded playback values. */
static void track_sync_punch_params(void){
 for(uint8_t fx=0u;fx<5u;fx++){
  uint8_t value=track_punch_base_param[fx];
  if(!(track_live_punch_mask&(1u<<fx)) && (track_replay_punch_mask&(1u<<fx)))
   value=track_song_effect_parameter(&track_song,track_state.pattern_slot,
           track_state.transport.play_step,fx,value);
  track_chip.punch_param[fx]=value;
 }
}
static void track_ui_inst_reset(void){
 for(uint8_t i=0u;i<4u;i++){
  track_ui_inst[i].attack=1u;track_ui_inst[i].decay=14u;
  track_ui_inst[i].sustain=100u;track_ui_inst[i].release=16u;
  track_ui_inst[i].duty=50u;track_ui_inst[i].cutoff=7000u;
  track_ui_inst[i].volume=65u;
  track_chip.env_attack[i]=1u;track_chip.env_decay[i]=14u;
  track_chip.env_sustain[i]=100u;track_chip.env_release[i]=16u;
 }
}

/* 19 = HELP / SHORTCUTS, only opened explicitly from PROJECTS.
 * 0 TRACK / 1 PROJECTS / 2 NAME / 3 PER STEP FX / 4 PUNCH / 5 EDIT
 * 6 MIXER / 7 SCALE/LOCK / 8 PROJECT SLOT SAVE STATUS / 9 SONG
 * 10 PUNCH EVENT TRACKER (REC held 1.5s) */
/* Guard against invalid cursor/data before indexing. A bad state is surfaced
 * as a diagnostic count, not silently mistaken for an empty song. */
static void track_runtime_validate(void){
 uint8_t faulty=0u;
 if(track_state.transport.bpm<30u || track_state.transport.bpm>300u){track_state.transport.bpm=120u;faulty=1u;}
 if(track_state.active.length==0u || track_state.active.length>TRACK_STEPS){
  /* Re-create a proper empty pattern, including note sentinel 255. */
  for(uint8_t c=0u;c<TRACK_CHANNELS;c++)for(uint8_t st=0u;st<TRACK_STEPS;st++){
   struct track_step *e=&track_state.active.ch[c][st];
   e->note=255u;e->instrument=c;e->chord=0u;e->effect=0u;e->value=50u;
  }
  track_state.active.length=16u;faulty=1u;
 }
 if(track_page>19u){track_page=0u;faulty=1u;}
 if(track_state.transport.edit_channel>=TRACK_CHANNELS){track_state.transport.edit_channel=0u;faulty=1u;}
 if(track_state.transport.edit_step>=TRACK_STEPS){track_state.transport.edit_step=0u;faulty=1u;}
 if(track_state.transport.edit_column>=TRACK_NAV_COLUMNS){track_state.transport.edit_column=0u;faulty=1u;}
 if(track_state.pattern_slot>=TRACK_PATTERNS){track_state.pattern_slot=0u;faulty=1u;}
 if(track_song.count==0u||track_song.count>TRACK_SONG_ENTRIES){
  track_song.count=1u;track_song.chain[0].pattern=0u;track_song.chain[0].repeats=1u;faulty=1u;
 }
 if(track_song.edit_row>=track_song.count)track_song.edit_row=0u;
 if(track_mix.row>=TRACK_CHANNELS){track_mix.row=0u;faulty=1u;}
 if(track_mix.col>=TRACK_MIXER_FIELDS){track_mix.col=0u;faulty=1u;}
 if(faulty)track_recovered_state_faults++;
}
static void track_note_label(uint8_t note,char buf[5]){
 static const char *const n[]={"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};
 if(note==TRACK_NOTE_STOP){buf[0]='S';buf[1]='T';buf[2]='P';buf[3]=0;return;}
 if(note==TRACK_NOTE_EMPTY){buf[0]='-';buf[1]='-';buf[2]='-';buf[3]=0;return;}
 const char *k=n[note%12u];buf[0]=k[0];buf[1]=k[1]==' '?'-':'#';
 int octave=(int)(note/12u)-1;buf[2]=octave<0?'-':(char)('0'+(octave>9?9:octave));buf[3]=0;
}
static void track_u8(char *dst,uint32_t v){dst[0]=(char)('0'+(v/10u)%10u);dst[1]=(char)('0'+v%10u);dst[2]=0;}
static void track_wheel_value(int32_t detents){
 if(!detents)return;
 struct track_step *st=&track_state.active.ch[track_state.transport.edit_channel][track_state.transport.edit_step];
 int32_t v;
 switch(track_state.transport.edit_column){
 case 0:
  st->note=track_note_scroll(st->note,detents,track_state.transport.edit_channel==TRACK_DRUM_CHANNEL);
  break;
 case 1:
  if(track_state.transport.edit_channel==TRACK_DRUM_CHANNEL){
   v=(int32_t)st->chord+detents;if(v<0)v=TRACK_DRUM_COUNT;if(v>TRACK_DRUM_COUNT)v=0;st->chord=(uint8_t)v;
  }else{v=(int32_t)st->instrument+detents;if(v<0)v=0;if(v>3)v=3;st->instrument=(uint8_t)v;}
  break;
 case 2:
  if(track_state.transport.edit_channel==TRACK_DRUM_CHANNEL){
   v=(int32_t)st->instrument+detents;if(v<0)v=0;if(v>3)v=3;st->instrument=(uint8_t)v;
  }else{v=(int32_t)st->chord+detents;
   st->chord=(uint8_t)((v%TRACK_CHORD_COUNT+TRACK_CHORD_COUNT)%TRACK_CHORD_COUNT);}
  break;
 case 3: v=(int32_t)st->effect+detents;
  {uint8_t next=(uint8_t)((v%TRACK_FX_COUNT+TRACK_FX_COUNT)%TRACK_FX_COUNT);
   if(next==TRACK_FX_VOLUME && st->effect!=TRACK_FX_VOLUME)st->value=100u;
   if(next==TRACK_FX_DRUM_PITCH && st->effect!=TRACK_FX_DRUM_PITCH)st->value=12u;
   st->effect=next;}break;
 }
 track_state.dirty=1u;
}
static void track_preview_release(uint32_t held){
 uint32_t released=track_preview_mask & ~held;
 if(!released)return;
 fm1_irq_off();
 for(uint8_t k=0u;k<27u;k++)if(released&(1u<<k)){
  if(track_preview_channel[k]<TRACK_DRUM_CHANNEL)
   track_chip_note_off(&track_chip,track_preview_channel[k],track_preview_note[k]);
  track_preview_channel[k]=255u;
 }
 track_preview_mask &= held;
 fm1_irq_enable_all();
}
static void track_fm1_boot(void){
 track_init(&track_state);track_mixer_init(&track_mix);track_drum_init(&track_drum);
 track_master_default(&track_master_cfg);track_drum_component=0u;track_drum_field=0u;
 track_master_focus=0u;track_master_chain_edit=0u;
 for(uint32_t z=0u;z<sizeof(track_master_dsp)/sizeof(uint32_t);z++)((uint32_t*)&track_master_dsp)[z]=0u;
 track_master_dsp_init(&track_master_dsp);track_punch_clear(&track_rec_events);
 track_song_init(&track_song,&track_state);track_chip_init(&track_chip);
 for(uint8_t k=0u;k<5u;k++)track_punch_base_param[k]=track_chip.punch_param[k];
 track_chip_fx_reset();track_ui_inst_reset();track_drum_init(&track_drum);track_old_note_mask=0;track_page=0u;track_named=0u;
 track_last_encoder_ms=0u;track_last_encoder_id=0u;track_last_encoder_delta=0;track_recovered_state_faults=0u;
 track_live_punch_mask=0u;track_replay_punch_mask=0u;track_punch_focus=255u;track_seq_since=0u;track_seq_fired=0u; /* lane clipboard persists after SEQ release */
 track_last_poll=0u;track_last_paint=0u;track_boot_input_unlock_ms=etch_ms+550u;track_home_since=0u;track_home_fired=0u;track_song_follow_active=0u;track_song_preplay_row=0u;
 track_scroll_preview_active=0u;track_scroll_preview_until=0u;
 track_audio_clock=0u;track_audio_started=0u;track_swing_pending_mask=0u;for(uint8_t ci=0;ci<TRACK_CHANNELS;ci++)track_swing_pending_at[ci]=0u;track_preview_mask=0u;
 for(uint8_t k=0u;k<27u;k++){track_preview_channel[k]=255u;track_preview_note[k]=255u;track_edit_last_onset_ms[k]=0u;}
 track_type_queued=0;track_rec_since=0u;track_rec_long=0u;track_punch_edit_row=0u;track_punch_edit_index=0u;
 track_preset_last_note=60u;track_boot_splash_until=etch_ms+900u;
 track_fx_drum_lane=0u;track_edit_poly=0u;track_key_oct_chord.last=0u;track_key_oct_chord.combo_latched=0u;
 native_controls.octave=0;track_edit_prev_keys=0u;track_edit_serial=0u;track_overwrite_wait=0u;
 for(uint8_t i=0u;i<TRACK_PROJECTS;i++)track_slot_seen[i]=0u;
 track_storage_result=TRACK_JOURNAL_HARDWARE_LOCKED;track_storage_confirm_since=0u;track_unsaved_action=0u;track_session_saved_slot=255u;track_saved_toast_until=0u;track_storage_busy_kind=0u;
 track_storage_progress_done=0u;track_storage_progress_total=0u;track_storage_just_initialized=0u;track_storage_phase=0u;
 track_load_scan=0u;track_load_feedback=TRACK_JOURNAL_NO_PROJECT;
 for(uint8_t i=0u;i<TRACK_PROJECTS;i++){
  track_load_status[i]=0u;
  for(uint8_t j=0u;j<=TRACK_NAME_MAX;j++)track_load_titles[i][j]=0;
 }
#if BEETLE_NATIVE_STORAGE
 /* Boot preflight only READS flash ownership. No auto-claim/erase/program. */
 beetle_storage_last_audit=beetle_hal_probe_ownership();
 track_storage_result=beetle_storage_last_audit!=BEETLE_CLAIM_EXISTING && beetle_storage_last_audit>=BEETLE_CLAIM_FOREIGN?TRACK_JOURNAL_FOREIGN:
                      beetle_storage_last_audit<0?TRACK_JOURNAL_IO:TRACK_JOURNAL_NO_PROJECT;
#endif
 for(uint8_t k=0u;k<27u;k++){track_edit_voice_key[k]=255u;track_edit_voice_note[k]=255u;track_edit_order[k]=0u;}
 for(uint8_t c=0;c<TRACK_CHANNELS;c++)track_prev_note_count[c]=0u;
 for(uint8_t c=0;c<TRACK_SYNTH_CHANNELS;c++)track_lane_instrument[c]=(uint8_t)(c&3u);
 for(uint8_t i=0;i<TRACK_NAME_MAX+1u;i++)track_title[i]=0;
 /* Disable ETCH's built-in ARP/sequence; TRACK drives its notes in ALNK0. */
 native_controls.arp_mode=0u;
 native_audio.perf_last_seq=0u;
 native_audio.voice_limit=8u;
}
/* The note sequencer runs at 44.1k in the I2S ISR. No malloc/flash/UI here.
 * ETCH's existing audio mixer and DMA protection stay in charge. */
static void track_audio_all_off(struct etch_native_audio *audio){
 (void)audio;
 track_chip_silence(&track_chip);track_drum_stop(&track_drum);
 /* A stopped transport is a fresh musical session. Clear automation,
  * but do not change the saved tracker pattern or instrument patches. */
 for(uint8_t c=0;c<TRACK_SYNTH_CHANNELS;c++){
  const struct track_instrument_ui_values *ins=&track_ui_inst[track_lane_instrument[c]];
  struct track_step stop={TRACK_NOTE_STOP,0u,0u,0u,50u};
  uint8_t duty=(uint8_t)(ins->duty<19u?0u:ins->duty<38u?1u:ins->duty<63u?2u:3u);
  uint8_t cutoff=(uint8_t)(ins->cutoff/75u>250u?250u:ins->cutoff/75u);
  beetle_step_fx_event(&track_chip,c,&stop,duty,cutoff,track_state.transport.bpm);
 }
 track_drum.step_gain=100u;
 for(uint8_t c=0;c<TRACK_CHANNELS;c++)track_prev_note_count[c]=0u;
}
static void track_audio_fire(struct etch_native_audio *audio,uint8_t lane_mask){
 (void)audio;
 const uint8_t position=track_state.transport.play_step;
 /* Drum channel: two independent sample triggers on the same step. The
  * second hit reuses the former CHD byte (0=empty; 1..16=drum index+1).
  * Oscillator channels remain fully unchanged. */
 const struct track_step *dr=&track_state.active.ch[TRACK_DRUM_CHANNEL][position];
 if((lane_mask&(1u<<TRACK_DRUM_CHANNEL))&&dr->effect==TRACK_FX_VOLUME)track_drum.step_gain=dr->value;
 if(!(lane_mask&(1u<<TRACK_DRUM_CHANNEL))){}
 else if(dr->note==TRACK_NOTE_STOP){track_drum_stop(&track_drum);}
 else if(!(track_state.punch_mask&(1u<<7))&&track_mixer_velocity(&track_mix,TRACK_DRUM_CHANNEL,94u)){
  (void)track_drum_trigger_pair_tuned(&track_drum,dr->note,
      (dr->chord>=1u&&dr->chord<=TRACK_DRUM_COUNT)?(uint8_t)(dr->chord-1u):TRACK_NOTE_EMPTY,
      (int8_t)(dr->effect==TRACK_FX_DRUM_PITCH?(int32_t)dr->value-12:0));
 }
 uint8_t pitches[TRACK_CHANNELS][5],lengths[TRACK_CHANNELS]={0u};
 uint8_t instruments[TRACK_CHANNELS],velocity[TRACK_CHANNELS];
 for(uint8_t c=0;c<TRACK_DRUM_CHANNEL;c++){
  if(!(lane_mask&(1u<<c)))continue;
  const struct track_step *s=&track_state.active.ch[c][position];
  /* The current note's instrument stays selected throughout its sustain.
   * Empty steps must not reset EQ, volume, or step FX. */
  if(s->note<=127u)track_lane_instrument[c]=s->instrument;
  instruments[c]=track_lane_instrument[c];
  const struct track_instrument_ui_values *ins=&track_ui_inst[instruments[c]];
  uint8_t duty=(uint8_t)(ins->duty<19u?0u:ins->duty<38u?1u:ins->duty<63u?2u:3u);
  uint8_t cutoff=(uint8_t)(ins->cutoff/75u>250u?250u:ins->cutoff/75u);
  beetle_step_fx_event(&track_chip,c,s,duty,cutoff,track_state.transport.bpm);
  if(s->note==TRACK_NOTE_STOP){track_chip_release_channel(&track_chip,c);continue;}
  velocity[c]=track_mixer_velocity(&track_mix,c,94u)?94u:0u;
  if(s->note==TRACK_NOTE_EMPTY || (track_state.punch_mask&(1u<<7)) || velocity[c]==0u)continue;
  int32_t note=(int32_t)s->note;
  if(track_state.punch_mask&(1u<<5))note+=12;
  if(track_state.punch_mask&(1u<<6))note-=12;
  /* PITCH is applied in DSP to the original note, including on sustained
   * notes. Do not pre-transpose as that would double the effect. */
  if(note<0)note=0;if(note>127)note=127;
  /* EMPTY sustains; STP stops; a NEW note fades out earlier pitches of
   * this SAME lane, avoiding stuck chords across repeated patterns. */
  track_chip_stop_channel(&track_chip,c);
  lengths[c]=track_chord(track_quantize((uint8_t)note,&track_state.harmony),s->chord,pitches[c]);
 }
 /* First guarantee each channel its root, then share the remaining four
  * voices with chord tones. Never let CH1 consume all eight voices. */
 for(uint8_t n=0;n<5u;n++)for(uint8_t c=0;c<TRACK_DRUM_CHANNEL;c++){
  if(n<lengths[c]){
   if(!track_chip_note(&track_chip,c,instruments[c],pitches[c][n],velocity[c]))return;
  }
 }
}
/* Render exact step boundaries even when they fall in the middle of DMA
 * blocks. Timing uses integer sample counts; no allocations or UI in ALNK0. */
static void track_fm1_audio_process(struct etch_native_audio *audio,uint32_t frames,int16_t *dst){
 if(!dst)return;
 if(!track_state.transport.playing){
  if(track_audio_started){track_audio_all_off(audio);track_audio_started=0u;}
  track_audio_clock=0u;track_swing_pending_mask=0u;
  track_chip_render(&track_chip,&track_mix,dst,frames);
  track_drum_render(&track_drum,&track_mix,dst,frames);track_chip_process_punch(&track_chip,track_state.punch_mask,dst,frames);track_master_process(&track_master_dsp,&track_master_cfg,dst,frames);return;
 }
 if(!track_audio_started){
  track_audio_started=1u;track_audio_clock=0u;
  track_replay_punch_mask=track_song_effect_mask(&track_song,track_state.pattern_slot,0u);
  track_sync_punch_params();
  track_state.punch_mask=(uint8_t)(track_live_punch_mask|track_replay_punch_mask);
  track_chip_begin_step(&track_chip);
  track_swing_pending_mask=0u;track_audio_fire(audio,0x3Fu);
 }
 if(track_state.punch_mask&(1u<<7))track_audio_all_off(audio);
 const uint32_t bpm=track_state.transport.bpm?track_state.transport.bpm:120u;
 uint32_t interval=661500u/bpm; /* 44100 Hz * 60 / 4 / BPM */
 if(interval<551u)interval=551u;
 uint32_t offset=0u;
 if(track_audio_clock>=interval)track_audio_clock=0u; /* BPM change guard */
 while(offset<frames){
  /* No allocations or blocking inside this DMA ISR; the deferred lane
   * event is delivered at exactly its sample timestamp. */
  uint8_t due=0u;
  for(uint8_t ci=0u;ci<TRACK_CHANNELS;ci++)if((track_swing_pending_mask&(1u<<ci))&&track_audio_clock>=track_swing_pending_at[ci])due|=(uint8_t)(1u<<ci);
  if(due){track_swing_pending_mask&=(uint8_t)~due;track_audio_fire(audio,due);}
  uint32_t until=interval-track_audio_clock;
  for(uint8_t ci=0u;ci<TRACK_CHANNELS;ci++)if((track_swing_pending_mask&(1u<<ci))&&track_audio_clock<track_swing_pending_at[ci]){
   uint32_t wait=track_swing_pending_at[ci]-track_audio_clock;
   if(wait<until)until=wait;
  }
  uint32_t chunk=frames-offset<until?frames-offset:until;
  track_chip_render(&track_chip,&track_mix,dst+offset*2u,chunk);
  track_drum_render(&track_drum,&track_mix,dst+offset*2u,chunk);
  track_chip_process_punch(&track_chip,track_state.punch_mask,dst+offset*2u,chunk);
  track_master_process(&track_master_dsp,&track_master_cfg,dst+offset*2u,chunk);
  offset+=chunk;track_audio_clock+=chunk;
  if(track_audio_clock>=interval){
   track_audio_clock-=interval;
   track_swing_pending_mask=0u;
   track_tick(&track_state);
   if(track_state.transport.play_step==0u && track_song.enabled){
    if(track_song_on_wrap(&track_song,&track_state)<0){
     track_replay_punch_mask=0u;
     track_sync_punch_params();
     track_state.punch_mask=track_live_punch_mask;
     track_audio_all_off(audio);track_audio_started=0u;
     if(offset<frames){track_chip_render(&track_chip,&track_mix,dst+offset*2u,frames-offset);track_drum_render(&track_drum,&track_mix,dst+offset*2u,frames-offset);track_chip_process_punch(&track_chip,track_state.punch_mask,dst+offset*2u,frames-offset);track_master_process(&track_master_dsp,&track_master_cfg,dst+offset*2u,frames-offset);}
     return;
    }
   }
   track_replay_punch_mask=track_song_effect_mask(&track_song,track_state.pattern_slot,track_state.transport.play_step);
   track_sync_punch_params();
   track_state.punch_mask=(uint8_t)(track_replay_punch_mask|track_live_punch_mask);
   track_chip_begin_step(&track_chip);
   track_swing_pending_mask=track_swing_mask(&track_mix,track_state.transport.play_step);
   for(uint8_t ci=0u;ci<TRACK_CHANNELS;ci++)track_swing_pending_at[ci]=track_swing_delay(interval,track_mix.ch[ci].swing);
   track_audio_fire(audio,(uint8_t)(0x3Fu&~track_swing_pending_mask));
  }
 }
}
static void track_fm1_leds(void){
 fm1_irq_off();
 for(uint8_t b=2u;b<14u;b++)fm1_led_key(b,1u);
 struct etch_harmony_settings h={track_state.harmony.root,track_state.harmony.mode,track_state.harmony.keylock,0u};
 uint32_t guide=etch_led_scale_guide_targets(native_controls.octave,&h);
 uint32_t locked=etch_led_keylock_targets(fm1_in.notes,native_controls.octave,&h);
 for(uint8_t k=0u;k<27u;k++){
  uint8_t active=(uint8_t)(((fm1_in.notes|guide|locked)>>k)&1u);
  if(track_page==6u){
   int8_t lane=beetle_black_mute_lane(k);
   active=(uint8_t)(lane>=0?track_mix.ch[(uint8_t)lane].mute:0u);
  }
  fm1_led_key(14u+k,active);
 }
 fm1_led_key(0u,(fm1_in.buttons&1u)!=0u);
 fm1_led_key(1u,(fm1_in.buttons&2u)!=0u);
 fm1_irq_enable_all();
}
/* TRACK display is native 240x240 RGB565. The supplied 1280px mockup is
 * mapped to the LCD grid, using only integer-position 1-bit shapes. The two
 * clipped 120-line passes fit within ETCH's unchanged 173-line DMA canvas.
 * No resizing/anti-aliasing on the target LCD. */
static void track_panel_begin_slice(uint16_t top,uint16_t height){
 etch_scope_origin=top;etch_scope_height=height;
 etch_scope_active=1u;etch_panel_composite=1u;
 for(uint32_t i=0;i<240u*(uint32_t)height;i++)etch_scope_canvas[i]=0u;
}
/* User-supplied GIF frames: matching 22x23 nearest-neighbour 1-bit assets.
 * Frame 1 is the specified STOP/IDLE pose. Frame 0 and 1 animate on PLAY.
 * Both use the exact same pixel footprint; no LCD interpolation. */
/* Shared two-frame play glyph: boot, tracker and OTA must agree. */
/* The PLAY glyph follows the sample clock and BPM, not UI scheduling.
 * One frame per 16th note. Dropped LCD frames therefore never drift. */
static uint8_t track_insect_frame(uint8_t playing,uint32_t now_ms){
 (void)now_ms;
 return playing?(uint8_t)(track_state.transport.play_step&1u):1u;
}
static void track_insect_draw(uint16_t x,uint16_t y,uint8_t frame){
 const uint32_t *pixels=beetle_play_icon[frame&1u];
 for(uint16_t row=0u;row<23u;row++)for(uint16_t col=0u;col<22u;col++)
  if(pixels[row]&(1u<<(21u-col)))lcd_fill(x+col,y+row,1u,1u,TRACK_PINK);
}
#include "track_pixel_font.h"
/* WEB SOURCE OF TRUTH: TRACK-Web-V0.19 tracker()/mixer() 240x240 pixels.
 * All rectangles and 5x7 text use the same integer glyph positioning.
 * Colour conversion from 24-bit CSS into RGB565 is necessarily lossy. */
static void track_pix_cell(const char *v,uint16_t center,uint16_t y){
 if(!v)v="---";
 uint16_t n=0u;while(v[n]&&n<12u)n++;
 /* JS Math.round(x-(len+2)*3+.5) == x-(len+2)*3+1 */
 uint16_t left=(uint16_t)(center-(uint16_t)((n+2u)*3u)+1u);
 track_pix_text("[",left,y,TRACK_WHITE);
 track_pix_text(v,(uint16_t)(left+6u),y,TRACK_PINK);
 track_pix_text("]",(uint16_t)(left+6u*(n+1u)),y,TRACK_WHITE);
}
static void track_fmt_int(char *dst,uint32_t value){
 char tmp[12];uint8_t n=0u;
 do{tmp[n++]=(char)('0'+value%10u);value/=10u;}while(value&&n<11u);
 uint8_t i=0u;while(n)dst[i++]=tmp[--n];dst[i]=0;
}
static void track_page_header(const char *title,uint16_t line_y){
 track_pix_top(title,track_state.transport.edit_channel,track_state.transport.bpm);
 if(track_page==10u){lcd_fill(7u,3u,27u,9u,TRACK_DARK);track_pix_text("0/6",8u,4u,TRACK_PINK);}
 if(line_y)lcd_fill(22u,line_y,196u,1u,0x48C8u);
}
static void track_web_list(const char *const *items,uint8_t count,uint8_t selected,uint16_t first,uint8_t pitch){
 for(uint8_t i=0;i<count;i++){
  uint16_t y=(uint16_t)(first+(uint16_t)i*pitch);
  if(i==selected)lcd_fill(24u,y-2u,194u,pitch,0x4807u);
  track_pix_text(i==selected?"> ":"  ",29u,y,i==selected?TRACK_BRIGHT:TRACK_WHITE);
  track_pix_text(items[i],41u,y,i==selected?TRACK_BRIGHT:TRACK_WHITE);
 }
}
static const char *const track_web_stepfx[TRACK_FX_COUNT]={
 "---","FILTER","DRIVE","DELAY","REVERB","CHORUS","VIB","ARP","CUT","TREMOLO","VOLUME","PITCH"};
/* V0.48: calculate the exact displayed bracket width before placing a cursor.
 * The previous 13px tall rectangle overlapped the next 10px tracker row. */
static uint8_t track_grid_value_chars(const struct track_step *step,uint8_t drum,uint8_t column){
 if(column==0u){
  if(drum){const char *text=track_drum_name(step->note);uint8_t n=0u;while(text[n]&&n<12u)n++;return n;}
  char name[5];track_note_label(step->note,name);
  uint8_t n=0u;while(name[n]&&n<4u)n++;return n;
 }
 if(column==1u){
  if(drum){const char *text=step->chord>=1u&&step->chord<=TRACK_DRUM_COUNT?
     track_drum_name((uint8_t)(step->chord-1u)):"---";
   uint8_t n=0u;while(text[n]&&n<12u)n++;return n;}
  return step->note==TRACK_NOTE_EMPTY?2u:2u; /* INST always 2 digits when note exists. */
 }
 if(column==2u)return drum?2u:3u; /* Drum INST vs synth CHD. */
 return 3u; /* FX: exactly three characters. */
}
static void track_grid_lane_status(void){
 /* Two columns, three rows, matching user's 240x240 reference at x=36..46.
  * The indicator is for the visible pattern, not the whole project. */
 for(uint8_t lane=0u;lane<TRACK_CHANNELS;lane++){
  uint16_t x=(uint16_t)(36u+(uint16_t)(lane&1u)*6u);
  uint16_t y=(uint16_t)(210u+(uint16_t)(lane>>1u)*6u);
  lcd_fill(x,y,5u,5u,track_lane_has_data(&track_state.active,lane)?TRACK_PINK:TRACK_WHITE);
 }
}
static void track_fm1_grid(void){
 track_page_header(0,0u);
 static const char *const headings[]={"STEP","NOTE","INST","CHD","FX"};
 static const char *const drums[]={"STEP","NOTE","NOTE2","INST","FX"};
 static const uint16_t xs[]={36u,77u,121u,163u,202u};
 static const char *const chords[TRACK_CHORD_COUNT]={"---","MAJ","MIN","DIM","AUG","SU2","SU4","MJ7","MN7","DO7","AD9","MN9"};
 static const char *const chord_full[TRACK_CHORD_COUNT]={"EMPTY","MAJOR","MINOR","DIMINISHED","AUGMENTED","SUSPENDED 2","SUSPENDED 4","MAJOR 7","MINOR 7","DOMINANT 7","ADD 9","MINOR 9"};
 uint8_t drum=(uint8_t)(track_state.transport.edit_channel==TRACK_DRUM_CHANNEL);
 for(uint8_t i=0u;i<5u;i++)track_pix_center(drum?drums[i]:headings[i],xs[i],28u,TRACK_WHITE);
 uint8_t first=(uint8_t)((track_state.transport.playing?track_state.transport.play_step:track_state.transport.edit_step)&0xF0u);
 for(uint8_t i=0u;i<16u;i++){
  uint8_t row=(uint8_t)(first+i); if(row>=track_state.active.length || row>=TRACK_STEPS)continue;
  uint16_t y=(uint16_t)(43u+(uint16_t)i*10u);
  uint8_t focused=(uint8_t)(!track_state.transport.playing&&track_state.transport.edit_step==row);
  uint8_t live=(uint8_t)(track_state.transport.playing&&track_state.transport.play_step==row);
  if(focused){
   lcd_fill(21u,y-1u,201u,11u,0x3005u);
   uint8_t col=track_state.transport.edit_column;
   if(col>=TRACK_NAV_COLUMNS)col=0u;
   /* Editable columns are NOTE, INST, CHD, FX: headings [1..4]. */
   uint8_t index=(uint8_t)(col+1u);
   const struct track_step *cell=&track_state.active.ch[track_state.transport.edit_channel][row];
   uint8_t chars=track_grid_value_chars(cell,drum,col);
   /* Same center and glyph pitch as track_pix_cell(); frame stays within
    * its own row (y-1 .. y+7), and exactly surrounds white brackets. */
   uint16_t cell_width=(uint16_t)(6u*(chars+2u));
   uint16_t left=(uint16_t)(xs[index]-(uint16_t)((chars+2u)*3u)+1u);
   uint16_t x=(uint16_t)(left-2u);
   lcd_fill(x,y-1u,cell_width+4u,9u,TRACK_PINK);
   lcd_fill(x+1u,y,cell_width+2u,7u,0x680Bu);
  }
  if(live){lcd_fill(21u,y-2u,201u,13u,0xF018u);lcd_fill(23u,y,197u,9u,0x880Du);}
  const struct track_step *st=&track_state.active.ch[track_state.transport.edit_channel][row];
  char number[12];track_u8(number,row);track_pix_cell(number,xs[0],y);
  if(drum){
   track_pix_cell(track_drum_name(st->note),xs[1],y);
   track_pix_cell(st->chord>=1u&&st->chord<=TRACK_DRUM_COUNT?track_drum_name((uint8_t)(st->chord-1u)):"---",xs[2],y);
   track_u8(number,st->instrument+1u);track_pix_cell(number,xs[3],y);
  }else{
   char pitch[5];track_note_label(st->note,pitch);track_pix_cell(pitch,xs[1],y);
   if(st->note==255u)track_pix_cell("--",xs[2],y);
   else{track_u8(number,st->instrument+1u);track_pix_cell(number,xs[2],y);}
   track_pix_cell(chords[st->chord<TRACK_CHORD_COUNT?st->chord:0u],xs[3],y);
  }
  const char *fx=track_web_stepfx[st->effect<TRACK_FX_COUNT?st->effect:0u];
  char fx3[4]={fx[0],fx[1],fx[2],0};track_pix_cell(fx3,xs[4],y);
 }
 if(track_state.transport.edit_column==2u&&!drum){
  const struct track_step *st=&track_state.active.ch[track_state.transport.edit_channel][track_state.transport.edit_step];
  char hint[32]="CHD: ";const char *ch=chord_full[st->chord<TRACK_CHORD_COUNT?st->chord:0u];
  uint8_t i=5u;while(*ch&&i<31u)hint[i++]=*ch++;hint[i]=0;
  track_pix_text(hint,48u,216u,TRACK_BRIGHT);
 }
 char pat[32]="PAT ";char num[12];track_u8(num,track_state.pattern_slot+1u);
 pat[4]=num[0];pat[5]=num[1];pat[6]=' ';pat[7]='/';pat[8]=' ';pat[9]='4';pat[10]='8';pat[11]=0;
 track_pix_text(pat,129u,224u,TRACK_PINK);

 track_grid_lane_status();
 track_insect_draw(10u,205u,track_insect_frame(track_state.transport.playing,etch_ms));
 if((int32_t)(track_saved_toast_until-etch_ms)>0){
  lcd_fill(104u,202u,126u,13u,TRACK_DARK);
  track_pix_text("PROJECT SAVED",112u,204u,TRACK_PINK);
 }
}
static void track_fm1_mixer_screen(void){
 track_page_header("MIXER",44u);
 static const char *const names[]={"CH","VOL","PAN","MUT","SOL","SWG"};
 static const uint16_t xs[]={10u,44u,86u,135u,172u,206u};
 for(uint8_t j=0;j<6u;j++)track_pix_text(names[j],xs[j],56u,TRACK_PINK);
 for(uint8_t c=0;c<TRACK_CHANNELS;c++){
  uint16_t y=(uint16_t)(75u+22u*c);
  if(track_mix.row==c)lcd_fill(7u,y-2u,227u,20u,0x4807u);
  char val[12];
  for(uint8_t j=0;j<6u;j++){
   if(track_mix.row==c&&j>0u&&track_mix.col+1u==j)
    lcd_fill(xs[j]-3u,y-2u,j<3u?35u:33u,20u,0x880Du);
   if(j==0u)track_fmt_int(val,c+1u);
   else if(j==1u)track_fmt_int(val,track_mix.ch[c].volume);
   else if(j==2u){int v=track_mix.ch[c].pan;val[0]=v<0?'-':'+';if(v<0)v=-v;track_fmt_int(val+1u,(uint32_t)v);}
   else if(j==5u)track_fmt_int(val,track_mix.ch[c].swing);
   else{uint8_t on=j==3u?track_mix.ch[c].mute:track_mix.ch[c].solo;
    val[0]=on?'O':'-';val[1]=on?'N':'-';val[2]=0;}
   /* Purple CH index only while actual post-mixer PCM is being produced. */
   uint16_t ink=(j==0u&&track_state.transport.playing&&track_lane_pcm_peak[c]>=96u)?TRACK_PINK:TRACK_WHITE;
   track_pix_text(val,xs[j],y+4u,ink);
  }
 }
 track_pix_text("K1 LANE K2 FIELD K3 SET",10u,207u,TRACK_PINK);
 track_pix_text("BLACK KEYS 1-6: MUTE",12u,222u,TRACK_WHITE);
}
static void track_fm1_song_screen(void){
 lcd_fill(0u,0u,240u,240u,TRACK_DARK);
 track_page_header("SONG MODE",43u);
 track_pix_text("POS",22u,54u,TRACK_PINK);
 track_pix_text("PATTERN",106u,54u,TRACK_PINK);
 track_pix_text("REPEAT",198u,54u,TRACK_PINK);
 /* Same eight 16px rows and same x/y coordinates as the web canvas. */
 const uint8_t focus=(track_song.enabled&&track_state.transport.playing)?track_song.position:track_song.edit_row;
 const uint8_t start=(uint8_t)((focus/8u)*8u);
 for(uint8_t j=0u;j<8u;j++){
  uint8_t r=(uint8_t)(start+j);if(r>=track_song.count)break;
  uint16_t y=(uint16_t)(71u+16u*j);
  uint8_t live=(uint8_t)(track_song.enabled&&track_state.transport.playing&&r==track_song.position);
  if(live){lcd_fill(15u,y-2u,215u,15u,0xE817u);lcd_fill(17u,y,211u,11u,0x780Bu);}
  else if(r==track_song.edit_row){
   lcd_fill(15u,y-2u,215u,16u,0x4007u);
   lcd_fill(track_song.edit_column==0u?102u:194u,y-2u,30u,16u,0x880Du);
  }
  char num[4];track_u8(num,r+1u);track_pix_text(num,22u,y,TRACK_WHITE);
  track_u8(num,track_song.chain[r].pattern+1u);track_pix_text(num,106u,y,TRACK_WHITE);
  track_u8(num,track_song.chain[r].repeats);track_pix_text(num,198u,y,TRACK_WHITE);
 }
 track_pix_text(track_song.enabled?(track_song.loop?"[X] SONG ON  LOOP: ON":"[X] SONG ON  LOOP: OFF"):(track_song.loop?"[ ] SONG OFF  LOOP: ON":"[ ] SONG OFF  LOOP: OFF"),11u,193u,TRACK_PINK);
 track_pix_text("OCT+ ADD  OCT- DELETE",10u,208u,TRACK_BRIGHT);
 track_pix_text("K1 POS  K2 PAT/REP  K3 VAL",10u,219u,TRACK_BRIGHT);
}
static void track_fm1_overlay(const char *title){
 track_fm1_grid(); /* consistent background with the original tracker UI */
 lcd_fill(10u,70u,220u,111u,0x1007u);
 lcd_fill(10u,70u,220u,2u,TRACK_PINK);
 label(title,17u,79u,TRACK_BRIGHT);
}
static void track_record_banner(void){
 /* REC lives in the upper status row, never over page titles at y21. */
 if(track_state.record_armed){lcd_fill(73u,3u,94u,9u,0x500Au);
  track_pix_text("REC: PUNCH FX",78u,4u,TRACK_PINK);}
}
static void track_fm1_drum_editor_screen(void){
 track_page_header("DRUM ENGINE / EDIT",43u);
 track_pix_text("COMPONENT",16u,55u,TRACK_PINK);
 track_pix_text(track_drum_name(track_drum_component),116u,55u,TRACK_WHITE);
 static const char *const f[]={"ATTACK","DECAY","SUSTAIN","RELEASE","PITCH","EQ LOW","EQ MID","EQ HIGH","GAIN"};
 const struct track_drum_timbre *p=&track_drum.components[track_drum_component];
 int values[9]={p->attack,p->decay,p->sustain,p->release,p->pitch,p->eq_low,p->eq_mid,p->eq_high,p->gain};
 for(uint8_t i=0u;i<9u;i++){
  uint16_t y=(uint16_t)(76u+(uint16_t)i*15u);
  if(track_drum_field==i)lcd_fill(8u,y-2u,222u,13u,0x4807u);
  track_pix_text(f[i],15u,y,track_drum_field==i?TRACK_PINK:TRACK_BRIGHT);
  char num[12];int v=values[i];uint8_t prefix=0u;if(v<0){num[0]='-';v=-v;prefix=1u;}track_fmt_int(num+prefix,(uint32_t)v);
  track_pix_text(num,182u,y,TRACK_WHITE);
 }
 track_pix_text("SELECT DRUM  K1 FIELD  K3 VALUE",4u,219u,TRACK_PINK);
}
/* Pixel-perfect 4-knob MASTER FX UI, small enough for 240x240 OLED. */
/* Fully circular, filled pixel knob. No floating-point calculations in UI.
 * Scanline widths form a true round rim; the pointer starts at the center.
 * One horizontal span per row avoids hundreds of SPI-facing point doodles. */
static void track_master_knob(uint16_t x,uint16_t y,uint8_t value){
 static const uint8_t rim[12]={11,10,10,10,10,9,9,8,7,6,4,0};
 static const uint8_t face[10]={9,8,8,8,8,7,7,6,4,0};
 /* 13 positions on a 270-degree arc, clockwise from lower-left. */
 static const int8_t pointer_x[13]={-7,-9,-9,-8,-6,-3,0,3,6,8,9,9,7};
 static const int8_t pointer_y[13]={ 7, 4, 0,-4,-7,-9,-9,-9,-7,-4,0,4,7};
 for(int8_t dy=-11;dy<=11;dy++){
  uint8_t a=(uint8_t)(dy<0?-dy:dy);
  uint8_t r=rim[a];
  lcd_fill((uint16_t)(x-r),(uint16_t)((int)y+dy),(uint16_t)(r*2u+1u),1u,TRACK_BRIGHT);
  if(a<=9u){uint8_t f=face[a];
   lcd_fill((uint16_t)(x-f),(uint16_t)((int)y+dy),(uint16_t)(f*2u+1u),1u,0x1803u);
  }
 }
 /* Bright marker on a solid knob, connected rather than scattered dots. */
 uint8_t idx=(uint8_t)((uint16_t)value*12u/100u);
 int32_t nx=pointer_x[idx],ny=pointer_y[idx];
 for(int32_t d=2;d<=8;d++){
  int32_t px=(int32_t)x+nx*d/9,py=(int32_t)y+ny*d/9;
  lcd_fill((uint16_t)px,(uint16_t)py,2u,2u,TRACK_PINK);
 }
 lcd_fill(x-1u,y-1u,3u,3u,TRACK_WHITE);
}
static void track_fm1_master_screen(void){
 track_page_header("MASTER / FX CHAIN",43u);
 static const char *const labels[]={"4 BAND EQ","COMPRESSOR","REVERB"};
 for(uint8_t i=0u;i<3u;i++){
  uint8_t fx=track_master_cfg.order[i];if(fx>2u)continue;
  uint16_t y=(uint16_t)(54u+20u*i);
  if(track_master_focus==i)lcd_fill(10u,y-2u,220u,18u,0x4807u);
  char name[23];name[0]=(char)('1'+i);name[1]=' ';name[2]=track_master_cfg.enabled[fx]?'[':'(';
  name[3]=track_master_cfg.enabled[fx]?'X':' ';name[4]=track_master_cfg.enabled[fx]?']':')';name[5]=' ';
  uint8_t j=6u;const char *nm=labels[fx];while(*nm&&j<21u)name[j++]=*nm++;name[j]=0;
  track_pix_text(name,16u,y,track_master_focus==i?TRACK_PINK:TRACK_WHITE);
 }
 uint8_t fx=track_master_cfg.order[track_master_focus];
 if(fx==0u){
  track_pix_text("LOW  LO-MID HI-MID HIGH",6u,120u,TRACK_WHITE);
  lcd_fill(15u,145u,209u,1u,0x4807u);
  lcd_fill(15u,171u,209u,1u,0x4807u);
  for(uint8_t i=0u;i<4u;i++){
   uint16_t x=(uint16_t)(26u+i*57u);uint8_t gain=track_master_cfg.eq[i];
   uint16_t base=184u;uint16_t height=(uint16_t)(gain*44u/100u);
   lcd_fill(x,135u,34u,1u,0x4807u);
   /* Bandwidth marker and adjustable frequency center. */
   uint16_t center=(uint16_t)(x+6u+(uint16_t)track_master_cfg.eq_freq[i]*19u/100u);
   uint16_t bandwidth=(uint16_t)(4u+track_master_cfg.eq_width[i]/10u);
   lcd_fill(center,135u,1u,51u,0x4807u);
   lcd_fill(center-bandwidth/2u,base-height,bandwidth,2u,TRACK_PINK);
   lcd_fill(center,base-height,2u,height+1u,TRACK_PINK);
   char n[12];track_fmt_int(n,gain);track_pix_text(n,x,188u,TRACK_BRIGHT);
  }
  /* Live stereo waveform trace, not a frequency-domain spectrum analyzer. */
  for(uint16_t i=0u;i<51u;i++){
   int16_t sample=native_scope[native_scope_ready][(uint16_t)(i*20u)];
   int32_t y=167-(int32_t)sample/2800;
   if(y<144)y=144;if(y>187)y=187;
   lcd_fill((uint16_t)(17u+i*4u),(uint16_t)y,2u,1u,TRACK_BRIGHT);
  }
  track_pix_text("OCT- WIDTH OCT+ FREQ",10u,207u,TRACK_WHITE);
  track_pix_text("K1-4 EQ SEQ+K4 ORDER",10u,219u,TRACK_PINK);
 }else{
  const uint8_t values[4]={
   fx==1u?track_master_cfg.comp_threshold:track_master_cfg.reverb_wet,
   fx==1u?track_master_cfg.comp_ratio:track_master_cfg.reverb_feedback,
   fx==1u?track_master_cfg.comp_gain:track_master_cfg.reverb_tone,
   fx==1u?track_master_cfg.comp_attack:track_master_cfg.reverb_spread};
  static const char *const comp[] = {"THR","RATIO","MAKEUP","ATTACK"};
  static const char *const rev[] = {"WET","TAIL","TONE","WIDTH"};
  const char *const *names=fx==1u?comp:rev;
  track_pix_text(fx==1u?"COMPRESSOR / DYNAMICS":"REVERB / SPACE",17u,118u,TRACK_PINK);
  for(uint8_t i=0u;i<4u;i++){
   uint16_t x=(uint16_t)(15u+i*57u),cx=(uint16_t)(x+16u),cy=165u;
   track_pix_text(names[i],x,135u,TRACK_WHITE);
   track_master_knob(cx,cy,values[i]);
   char num[12];track_fmt_int(num,values[i]);
   track_pix_center(num,cx,188u,TRACK_BRIGHT);
  }
  track_pix_text(fx==1u?"K1 THR K2 RAT K3 GAIN K4 ATK":"K1 WET K2 TAIL K3 TONE K4 WID",6u,210u,TRACK_WHITE);
 }
 track_pix_text("SEL FX  ENV ON/OFF  SEQ+K4 ORDER",3u,224u,TRACK_PINK);
}
static void beetle_draw_keyboard_octave(uint16_t y){
 char msg[24]="KEY OCT ";int8_t v=native_controls.octave;
 msg[8]=(char)(v<0?'-':'+');msg[9]=(char)('0'+(v<0?-v:v));
 msg[10]=' ';msg[11]='O';msg[12]='C';msg[13]='T';msg[14]='-';msg[15]='/';msg[16]='+';msg[17]=0;
 track_pix_text(msg,9u,y,TRACK_WHITE);
}
static void track_fm1_paint(void){
 if((int32_t)(track_boot_splash_until-etch_ms)>0){
  beetle_boot_art((uint8_t)((etch_ms/100u)&1u));
  return;
 }
 if(track_page==0u){track_fm1_grid();return;}
 if(track_page==6u){track_fm1_mixer_screen();return;}
 if(track_page==9u){track_fm1_song_screen();return;}
 if(track_page==16u){track_fm1_drum_editor_screen();return;}
 if(track_page==17u){track_fm1_master_screen();return;}
 if(track_page==2u){
  track_page_header("PROJECT NAME",43u);
  char slot[18]="SLOT ";track_fmt_int(slot+5u,(uint32_t)track_state.project_slot+1u);
  track_pix_text(slot,18u,53u,TRACK_PINK);
  char name[TRACK_NAME_MAX+1u];
  for(uint8_t i=0u;i<TRACK_NAME_MAX;i++)name[i]=track_editor.touched[i]?track_editor.name[i]:'_';name[TRACK_NAME_MAX]=0;
  uint16_t left=(uint16_t)(120u-track_pix_width(name)/2u);
  track_pix_text(name,left,82u,TRACK_WHITE);
  lcd_fill((uint16_t)(left+6u*track_editor.cursor),92u,5u,2u,TRACK_PINK);
  track_pix_text("K1  POSITION  K2  CHAR",17u,123u,TRACK_WHITE);
  char sh[32]="K4  SLOT ";char slotnum[12];track_fmt_int(slotnum,track_state.project_slot+1u);
  uint8_t at=9u,j=0u;while(slotnum[j]&&at<22u)sh[at++]=slotnum[j++];
  sh[at++]='/';sh[at++]='1';sh[at++]='2';sh[at]=0;
  track_pix_text(sh,17u,142u,TRACK_PINK);
  track_pix_text("SAVE  CONFIRM",17u,177u,TRACK_PINK);
  char def[32]="EMPTY = PROJECT ";
  uint8_t at2=16u,j2=0u;while(slotnum[j2]&&at2<22u)def[at2++]=slotnum[j2++];def[at2]=0;
  track_pix_text(def,17u,207u,TRACK_BRIGHT);
  return;
 }
 if(track_page==11u){
  track_page_header("OVERWRITE?",43u);
  char slot[18]="SLOT ";track_fmt_int(slot+5u,(uint32_t)track_state.project_slot+1u);
  track_pix_center(slot,120u,77u,TRACK_PINK);
  track_pix_center("SELECT = YES",120u,112u,TRACK_WHITE);
  track_pix_center("HOME = NO",120u,134u,TRACK_WHITE);
  track_pix_center("ONLY THIS SLOT CHANGES",120u,192u,TRACK_BRIGHT);
  return;
 }
 if(track_page==10u){
  track_page_header("LANE 0 / REC FX",42u);
  track_pix_text("STEP",12u,38u,TRACK_PINK);track_pix_text("FX",69u,38u,TRACK_PINK);
  track_pix_text("ON/OFF",154u,38u,TRACK_PINK);
  const struct track_song_clip *clip=&track_song.clips[track_state.pattern_slot];
  uint8_t first=(uint8_t)((track_punch_edit_row/16u)*16u);
  for(uint8_t i=0u;i<16u;i++){
   uint8_t row=(uint8_t)(first+i);if(row>=TRACK_STEPS)break;
   uint16_t y=(uint16_t)(51u+i*9u);
   if(row==track_punch_edit_row)lcd_fill(8u,y-1u,224u,9u,0x680Au);
   char num[4];track_u8(num,row);track_pix_text(num,14u,y,TRACK_BRIGHT);
   uint8_t count=0u;const struct track_song_event *ev=0;
   for(uint8_t n=0u;n<clip->count;n++)if(clip->data[n].step==row){if(count==track_punch_edit_index)ev=&clip->data[n];count++;}
   static const char *const fx[]={"REPEAT","REVERSE","FILTER","TAPE","STUTTER","OCT UP","OCT DN","MUTE"};
   track_pix_text(ev?(ev->down?fx[ev->fx&7u]:"[STP]"):"---",68u,y,TRACK_BRIGHT);
   if(ev&&ev->down==2u){char param[12]="ALG ";track_fmt_int(param+4u,ev->param);
    track_pix_text(param,154u,y,TRACK_PINK);
   }else track_pix_text(ev?(ev->down?"ON":"STOP"):"--",154u,y,TRACK_WHITE);
   if(row==track_punch_edit_row){track_u8(num,count);track_pix_text(num,212u,y,TRACK_PINK);}
  }
  track_pix_text(clip->overflow?"FX EVENTS FULL":"K1 STEP K2 EVENT K3 FX",9u,201u,clip->overflow?TRACK_PINK:TRACK_WHITE);
  track_pix_text("OCT+ ADD OCT- DELETE",9u,213u,TRACK_BRIGHT);
  track_pix_text("SEL CLEAR STEP  RIGHT=CH1",9u,225u,TRACK_PINK);return;
 }
 if(track_page==3u){
  track_page_header("PER STEP FX",42u);
  const struct track_step *st=&track_state.active.ch[track_state.transport.edit_channel][track_state.transport.edit_step];
  char line[32]="STEP ";char n[4];track_u8(n,track_state.transport.edit_step);
  line[5]=n[0];line[6]=n[1];line[7]=' ';line[8]=' ';line[9]='N';line[10]='O';line[11]='T';line[12]='E';
  if(track_state.transport.edit_channel==TRACK_DRUM_CHANNEL){line[13]=(char)('1'+track_fx_drum_lane);line[14]=0;}
  else line[13]=0;
  track_pix_text(line,16u,52u,TRACK_PINK);
  if(track_state.transport.edit_channel==TRACK_DRUM_CHANNEL){
   uint8_t hit=track_fx_drum_lane?(st->chord>=1u&&st->chord<=TRACK_DRUM_COUNT?(uint8_t)(st->chord-1u):255u):st->note;
   track_pix_text(track_drum_name(hit),174u,52u,TRACK_BRIGHT);
  }else{char v[5];track_note_label(st->note,v);track_pix_text(v,174u,52u,TRACK_BRIGHT);}
  char typ[40]="TYPE ";const char *fx=track_web_stepfx[st->effect<TRACK_FX_COUNT?st->effect:0u];
  uint8_t i=5u;while(*fx&&i<38u)typ[i++]=*fx++;typ[i]=0;
  char amt[20]="AMOUNT ";
  if(st->effect==TRACK_FX_DRUM_PITCH){
   int8_t semis=(int8_t)st->value-12;
   amt[7]=(char)(semis<0?'-':'+');track_fmt_int(n,(uint32_t)(semis<0?-semis:semis));
   i=8u;for(uint8_t k=0u;n[k]&&i<18u;)amt[i++]=n[k++];amt[i]=0;
  }else{track_fmt_int(n,st->value);i=7u;for(uint8_t k=0u;n[k]&&i<18u;)amt[i++]=n[k++];amt[i++]='%';amt[i]=0;}
  const char *const items[]={typ,amt};track_web_list(items,2u,track_fx_cursor,86u,28u);
  track_pix_text("K1 PARAM  K3 CHANGE",15u,176u,TRACK_PINK);
  if(track_state.transport.edit_channel==TRACK_DRUM_CHANNEL)track_pix_text("K2 NOTE1 / NOTE2",15u,187u,TRACK_PINK);
  track_pix_text("FX > PUNCH-IN",15u,200u,TRACK_WHITE);return;
 }
 if(track_page==4u){
  track_page_header("PUNCH-IN FX",43u);
  static const char *const punches[]={"REPEAT","REVERSE","FILTER","TAPE STOP","STUTTER","OCT UP","OCT DOWN","MUTE"};
  for(uint8_t j=0u;j<8u;j++){
   uint16_t x=(uint16_t)(17u+(j%2u)*106u),y=(uint16_t)(58u+(j/2u)*32u);
   uint8_t held=(uint8_t)(((track_live_punch_mask|track_replay_punch_mask)&(1u<<j))!=0u);
   lcd_fill(x,y,99u,26u,held?0xE818u:0x3806u);
   char name[20];name[0]=(char)('1'+j);name[1]=' ';
   uint8_t k=0u;while(punches[j][k]&&k<16u){name[k+2u]=punches[j][k];k++;}name[k+2u]=0;
   track_pix_text(name,x+4u,y+6u,held?TRACK_DARK:TRACK_WHITE);
   if(held&&j<5u){char alg[16]="ALG ";char val[12];track_fmt_int(val,track_chip.punch_param[j]);
    uint8_t i=4u;for(uint8_t p=0u;val[p]&&i<14u;)alg[i++]=val[p++];alg[i]=0;
    track_pix_text(alg,x+4u,y+16u,TRACK_WHITE);}
  }
  track_pix_text(track_state.record_armed?"REC: ON":"REC: OFF",12u,202u,track_state.record_armed?TRACK_PINK:TRACK_WHITE);
  track_pix_text(track_replay_punch_mask?"REC FX PLAYING / LIT":"WHITE KEYS HOLD TO PLAY",12u,221u,TRACK_BRIGHT);return;
 }
 if(track_page==5u){
  track_page_header(track_osc_bank==0u?"INSTRUMENT / OSC":
    track_osc_bank==1u?"VA OSC A / SHAPE":
    track_osc_bank==2u?"VA OSC B / SHAPE":"VA OSC / MIX",43u);
  if(track_osc_bank){
   static const char *const waves[]={"SAW","TRIANGLE","SQUARE","SINE","PULSE"};
   static const char *const sidwaves[]={"SAW","TRIANGLE","PULSE","NOISE","PULSE"};
   static const char *const tuning[]={"SHAPE","COARSE","FINE","PWIDTH"};
   static const char *const combining[]={"DETUNE B","MIX A/B","MORPH","DRIVE","SUB OSC","NOISE","FM DEPTH"};
   static const char *const sidcombine[]={"DETUNE B","MIX A/B","FILT MODE","RESONANCE","SUB OSC","NOISE","FM DEPTH"};
   const struct track_step *st=&track_state.active.ch[track_state.transport.edit_channel][track_state.transport.edit_step];
   const struct track_va_patch *p=&track_chip.patches[st->instrument&3u];
   const uint8_t sid_mode=(uint8_t)(track_chip.osc_type[st->instrument&3u]>=TRACK_CHIP_RESID_LITE);
   char values[7][24],items[7][40];const char *entries[7];
   uint8_t count=(track_osc_bank==3u)?7u:4u;
   for(uint8_t i=0;i<count;i++){
    const char *nm=track_osc_bank==3u?(sid_mode?sidcombine[i]:combining[i]):tuning[i];
    const char *valname=0;int x=0;
    if(track_osc_bank==1u){
     if(i==0u)valname=sid_mode?sidwaves[p->wave_a]:waves[p->wave_a];
     else x=i==1u?p->coarse:i==2u?p->fine:p->pulse_width;
    }else if(track_osc_bank==2u){
     if(i==0u)valname=sid_mode?sidwaves[p->wave_b]:waves[p->wave_b];
     else x=i==1u?p->coarse_b:i==2u?p->fine_b:p->pulse_width_b;
    }else x=i==0u?p->detune:i==1u?p->mix:i==2u?p->morph:i==3u?p->drive:i==4u?p->sub:i==5u?p->noise:p->fm_depth;
    uint8_t at=0;
    if(valname){while(valname[at]&&at<23u){values[i][at]=valname[at];at++;}values[i][at]=0;}
    else{if(x<0){values[i][at++]='-';x=-x;}track_fmt_int(values[i]+at,(uint32_t)x);}
    at=0;while(nm[at]&&at<10u){items[i][at]=nm[at];at++;}while(at<11u)items[i][at++]=' ';
    uint8_t j=0;while(values[i][j]&&at<39u)items[i][at++]=values[i][j++];items[i][at]=0;entries[i]=items[i];
   }
   track_web_list(entries,count,track_osc_cursor,51u,20u);
   track_pix_text("K1 SELECT  K3 EDIT",12u,195u,TRACK_PINK);
   beetle_draw_keyboard_octave(214u);return;
  }
  static const char *const types[]={"RETRO PULSE","RETRO WAVE","VA OSC","VA OSC2","SID 6581","SID 8580","SID DIRTY","SID BASS","SID NOISE"};
  static const char *const keys[]={"TYPE","ATTACK","DECAY","SUSTAIN","RELEASE","DUTY","CUTOFF","VOLUME"};
  const struct track_step *st=&track_state.active.ch[track_state.transport.edit_channel][track_state.transport.edit_step];
  const struct track_instrument_ui_values *v=&track_ui_inst[st->instrument&3u];
  char values[8][24];char items[8][40];const char *entry[8];
  uint8_t type=track_chip.osc_type[st->instrument&3u]%TRACK_CHIP_TYPES;
  const char *name=types[type];uint8_t k=0u;
  while(name[k]&&k<23u){values[0][k]=name[k];k++;}values[0][k]=0;
  const uint8_t cents[]={v->attack,v->decay,v->sustain,v->release};
  for(uint8_t i=0u;i<4u;i++){
   uint8_t n=cents[i];values[i+1u][0]='0';values[i+1u][1]='.';
   values[i+1u][2]=(char)('0'+n/10u);values[i+1u][3]=(char)('0'+n%10u);values[i+1u][4]=0;
  }
  track_fmt_int(values[5],v->duty);k=0u;while(values[5][k])k++;values[5][k++]='%';values[5][k]=0;
  track_fmt_int(values[6],v->cutoff);k=0u;while(values[6][k])k++;
  values[6][k++]=' ';values[6][k++]='H';values[6][k++]='z';values[6][k]=0;
  track_fmt_int(values[7],v->volume);k=0u;while(values[7][k])k++;values[7][k++]='%';values[7][k]=0;
  for(uint8_t i=0u;i<8u;i++){
   uint8_t pos=0u;while(keys[i][pos]&&pos<9u){items[i][pos]=keys[i][pos];pos++;}
   while(pos<9u)items[i][pos++]=' ';
   k=0u;while(values[i][k]&&pos<39u)items[i][pos++]=values[i][k++];
   items[i][pos]=0;entry[i]=items[i];
  }
  track_web_list(entry,8u,track_osc_cursor,51u,19u);
  track_pix_text(track_edit_poly?"POLY AUDITION":"MONO AUDITION",17u,202u,TRACK_PINK);
  beetle_draw_keyboard_octave(216u);return;
 }
 if(track_page==19u){
  track_page_header("HELP / SHORTCUTS",43u);
  track_pix_text("K1 SCROLL  K4 PAGE",9u,49u,TRACK_PINK);
  lcd_fill(9u,62u,221u,1u,0x4807u);
  for(uint8_t row=0u;row<BEETLE_HELP_VISIBLE;row++){
   uint8_t idx=(uint8_t)(track_help_offset+row);
   if(idx>=BEETLE_HELP_COUNT)break;
   uint16_t y=(uint16_t)(68u+(uint16_t)row*13u);
   const char *str=beetle_help_lines[idx];
   if(str[0]=='#'){
    lcd_fill(8u,y-2u,218u,11u,0x3005u);
    track_pix_text(str+2,12u,y,TRACK_PINK);
   }else track_pix_text(str,12u,y,TRACK_WHITE);
  }
  /* A small scrollbar shows progress through the complete list. */
  lcd_fill(231u,68u,2u,125u,0x3807u);
  uint8_t max=beetle_help_last_offset();
  uint16_t thumb_y=(uint16_t)(68u+(max?(uint32_t)track_help_offset*104u/max:0u));
  lcd_fill(230u,thumb_y,4u,21u,TRACK_PINK);
  lcd_fill(9u,201u,221u,1u,0x4807u);
  track_pix_text("HOME: MENU  SEQ: TRACKER",10u,208u,TRACK_BRIGHT);
  char count[18]="";char a[8],b[8];
  track_fmt_int(a,(uint32_t)track_help_offset+1u);
  track_fmt_int(b,BEETLE_HELP_COUNT);
  uint8_t n=0u,j=0u;while(a[j]&&n<8u)count[n++]=a[j++];count[n++]='/';
  j=0u;while(b[j]&&n<15u)count[n++]=b[j++];count[n]=0;
  track_pix_center(count,120u,225u,TRACK_PINK);
  return;
 }
 if(track_page==1u){
  track_page_header("PROJECTS",43u);
  const char *const entries[]={"SAVE","SAVE AS","LOAD PROJECT","NEW PROJECT","HELP"};
  track_web_list(entries,5u,track_menu_index,54u,26u);
  char status[24]="SLOT ";track_fmt_int(status+5u,(uint32_t)track_state.project_slot+1u);
  track_pix_text(status,20u,188u,TRACK_PINK);
  track_pix_text("K1 MENU K4 SLOT",20u,203u,TRACK_WHITE);
  track_pix_text("SEL = OPEN  (LOAD SHOWS LIST)",9u,217u,TRACK_BRIGHT);return;
 }
 if(track_page==7u){
  track_page_header("SCALE GUIDE / KEYLOCK",43u);
  char root[32]="ROOT    ",scale[32]="SCALE   ",lock[32]="KEYLOCK ";
  const char *v=etch_note_name(track_state.harmony.root);uint8_t i=8u;while(*v&&i<29u)root[i++]=*v++;root[i]=0;
  v=etch_mode_name(track_state.harmony.mode);i=8u;while(*v&&i<29u)scale[i++]=*v++;scale[i]=0;
  static const char *const modes[]={"OFF","GUIDE","LOCK"};v=modes[track_state.harmony.keylock%3u];i=8u;while(*v&&i<29u)lock[i++]=*v++;lock[i]=0;
  const char *const entries[]={root,scale,lock};track_web_list(entries,3u,track_menu_index%3u,62u,35u);
  track_pix_text("GUIDE: ILLUMINATE SCALE KEYS",10u,192u,TRACK_PINK);
  track_pix_text("LOCK: QUANTIZE NOTES",10u,207u,TRACK_WHITE);
  track_pix_text("K1 SELECT K3 SET MODE",10u,220u,TRACK_BRIGHT);return;
 }
 if(track_page==14u){
  track_page_header("LOAD PROJECT",43u);
  for(uint8_t j=0u;j<TRACK_PROJECTS;j++){
   uint16_t y=(uint16_t)(52u+13u*j);
   if(track_state.project_slot==j)lcd_fill(6u,y-2u,228u,12u,0x4807u);
   char line[27];line[0]='0';line[1]=(char)('1'+j);
   if(j>=9u){line[0]='1';line[1]=(char)('0'+j-9u);}
   line[2]=' ';line[3]=' ';
   const char *name=track_load_status[j]==0u?"SCANNING...":
       track_load_status[j]==2u?"[EMPTY]":track_load_status[j]==4u?"[SETUP REQUIRED]":track_load_status[j]==3u?"[READ ERROR]":track_load_titles[j];
   uint8_t pos=4u;
   while(name[pos-4u] && pos<25u){line[pos]=name[pos-4u];pos++;}
   line[pos]=0;
   track_pix_text(line,16u,y,track_state.project_slot==j?TRACK_WHITE:TRACK_BRIGHT);
  }
  if(track_load_feedback==TRACK_JOURNAL_NO_PROJECT)track_pix_text("K1 SLOT  SEL LOAD  HOME BACK",5u,217u,TRACK_PINK);
  else track_pix_text("LOAD ERROR - CHECK SELECTED SLOT",4u,217u,TRACK_PINK);
  return;
 }
 if(track_page==15u){
  track_page_header("INITIALIZING",43u);
  track_pix_center(track_storage_phase?"SAVING PROJECT...":"BEETLE PROJECT STORAGE",120u,78u,TRACK_WHITE);
  char detail[32]="ERASE / VERIFY  ";char v[12];track_fmt_int(v,track_storage_progress_done);
  uint8_t i=16u,j=0u;while(v[j]&&i<25u)detail[i++]=v[j++];detail[i++]='/';
  track_fmt_int(v,track_storage_progress_total);j=0u;while(v[j]&&i<30u)detail[i++]=v[j++];detail[i]=0;
  track_pix_center(detail,120u,117u,TRACK_PINK);
  track_pix_center("DO NOT POWER OFF",120u,167u,TRACK_WHITE);return;
 }
 if(track_page==18u){
  track_page_header(track_storage_busy_kind==2u?"LOADING PROJECT":"SAVING PROJECT",43u);
  track_pix_center(track_storage_busy_kind==2u?"READING + VERIFYING...":"WRITING + VERIFYING...",120u,100u,TRACK_PINK);
  track_pix_center(track_storage_busy_kind==2u?"PLEASE WAIT":"DO NOT POWER OFF",120u,151u,TRACK_WHITE);
  return;
 }
 if(track_page==8u){
  track_page_header( track_storage_result==TRACK_JOURNAL_OK?(track_storage_just_initialized?"STORAGE READY":"PROJECT SAVED"):"NOT SAVED",43u);
  track_pix_center(track_storage_result==TRACK_JOURNAL_OK?"BEETLE ONLY":
   track_storage_result==TRACK_JOURNAL_FOREIGN?"FOREIGN STORAGE":
   track_storage_result==TRACK_JOURNAL_NO_PROJECT?"NO PROJECT":
   track_storage_result==TRACK_JOURNAL_HARDWARE_LOCKED?"FLASH SAVE LOCKED":
   track_storage_result==TRACK_JOURNAL_CAPACITY?"PROJECT TOO FULL":"STORAGE ERROR",120u,109u,TRACK_WHITE);
  track_pix_center(track_storage_result==TRACK_JOURNAL_OK?
   (track_storage_just_initialized?"INITIALIZED + PROJECT SAVED":"CRC VERIFIED"):
   "NO DATA WRITTEN",120u,131u,TRACK_BRIGHT);
  #if BEETLE_PILOT
  track_pix_center("V0.43 PILOT - OTA READY",120u,178u,TRACK_PINK);
#endif
  track_pix_center("SEQ TO RETURN",120u,158u,TRACK_WHITE);return;
 }
 if(track_page==13u){
  track_page_header("UNSAVED PROJECT",43u);
  track_pix_center("DISCARD UNSAVED WORK?",120u,78u,TRACK_PINK);
  track_pix_center(track_unsaved_action==2u?"CREATE NEW PROJECT":track_unsaved_action==3u?"LOAD SELECTED PROJECT":"OPEN PROJECT LIST",120u,108u,TRACK_WHITE);
  track_pix_center("SELECT = DISCARD",120u,154u,TRACK_BRIGHT);
  track_pix_center("HOME = KEEP WORK",120u,182u,TRACK_WHITE);
  return;
 }
 if(track_page==12u){
  track_page_header("BEETLE STORAGE",43u);
  track_pix_center("ERASE ALL 12 SLOTS?",120u,70u,TRACK_PINK);
  track_pix_center("ETCH/FELUCCA INCLUDED",120u,98u,TRACK_WHITE);
  track_pix_center("FULL FLASH BACKUP FIRST",120u,120u,TRACK_BRIGHT);
  track_pix_center("HOLD SELECT + SAVE",120u,154u,TRACK_PINK);
  if(track_storage_confirm_since){
   uint32_t elapsed=etch_ms-track_storage_confirm_since;
   uint32_t remain=elapsed>=BEETLE_STORAGE_HOLD_MS?0u:(BEETLE_STORAGE_HOLD_MS-elapsed+999u)/1000u;
   char caption[32]="KEEP HOLDING: ";char sec[12];track_fmt_int(sec,remain);
   uint8_t n=14u,j=0u;while(sec[j]&&n<26u)caption[n++]=sec[j++];
   caption[n++]='s';caption[n]=0;
   track_pix_center(caption,120u,169u,TRACK_WHITE);
  }else track_pix_center("HOLD BOTH FOR 5 SEC",120u,169u,TRACK_WHITE);
  track_pix_center("HOME = CANCEL",120u,203u,TRACK_BRIGHT);return;
 }
}
static void track_input_diag_draw(void){
#if BEETLE_PILOT
 if(beetle_frame_faults||track_recovered_state_faults){
  lcd_fill(7u,195u,225u,23u,TRACK_DARK); /* opaque diagnostic strip: no text overlaps */
  track_pix_text(beetle_frame_faults?"FRAME GUARD ERROR":"STATE RECOVERED",13u,203u,TRACK_PINK);
 }else if(track_last_encoder_ms && (uint32_t)(etch_ms-track_last_encoder_ms)<1800u){
  lcd_fill(7u,195u,225u,23u,TRACK_DARK);
  static const char *const knob[]={"SELECT","ALGO","K1","K2","K3","K4"};
  uint8_t id=track_last_encoder_id<6u?track_last_encoder_id:0u;
  track_pix_text("ENC:",13u,203u,TRACK_PINK);
  track_pix_text(knob[id],45u,203u,TRACK_PINK);
  track_pix_text(track_last_encoder_delta>=0?"+":"-",104u,203u,TRACK_BRIGHT);
 }
#endif
}
static void track_fm1_draw(void){
 track_runtime_validate();
 /* LCD uses only dedicated guarded RAM, never the pattern song bank. */
 track_panel_begin_slice(0u,120u);track_fm1_paint();track_record_banner();track_input_diag_draw();etch_panel_commit();
 track_panel_begin_slice(120u,120u);track_fm1_paint();track_record_banner();track_input_diag_draw();etch_panel_commit();
 /* Reset after BOTH LCD slices, so low-level audio peaks never become stale. */
 if(track_page==6u){fm1_irq_off();for(uint8_t c=0u;c<TRACK_CHANNELS;c++)track_lane_pcm_peak[c]=0u;fm1_irq_enable_all();}
}
static void track_begin_save(void){
 track_name_begin(&track_editor,track_named?track_title:0);
 track_page=2u;
}
static void track_complete_save(void){
 track_storage_just_initialized=0u;
 if(track_state.transport.playing||native_safe||native_tripped||native_updating){
  track_storage_result=TRACK_JOURNAL_HARDWARE_LOCKED;track_page=8u;return;
 }
 track_name_finish(&track_editor,track_state.project_slot,track_title);
#if BEETLE_NATIVE_STORAGE
 /* Display feedback BEFORE any potentially slow flash inspection or I/O. */
 track_storage_busy_kind=1u;track_page=18u;screen();
 native_pause();
 beetle_storage_last_audit=beetle_hal_probe_ownership();
 if(beetle_storage_last_audit==BEETLE_CLAIM_IO || beetle_storage_last_audit==BEETLE_CLAIM_INVALID){
  track_storage_result=TRACK_JOURNAL_IO;track_page=8u;native_start();return;
 }
 /* Never silently overwrite an existing project after firmware reboot. */
 if(!track_overwrite_wait && track_state.project_slot!=track_session_saved_slot && beetle_storage_last_audit==BEETLE_CLAIM_EXISTING){
  /* Preview is for prompting only. The full journal SAVE validates BOTH
   * banks and refuses foreign data before any erase, regardless of preview. */
  char peek_title[13];
  int found=track_journal_preview(&beetle_hal_journal_io,track_state.project_slot,peek_title);
  if(found==TRACK_JOURNAL_OK){track_page=11u;track_overwrite_wait=1u;native_start();return;}
  if(found!=TRACK_JOURNAL_NO_PROJECT){track_storage_result=found;track_page=8u;native_start();return;}
 }
 beetle_project_capture(track_state.project_slot);
 if(beetle_storage_last_audit!=BEETLE_CLAIM_EXISTING){
  /* First-use blank flash can be explicitly provisioned, erasing ALL slots.
   * An overwrite of a previously initialized volume NEVER offers this
   * destructive route. Foreign/corrupt ownership requires recovery. */
  if(beetle_storage_last_audit==BEETLE_CLAIM_BLANK && !track_overwrite_wait){
   track_storage_result=TRACK_JOURNAL_NO_PROJECT;
   track_page=12u;track_storage_confirm_since=0u;native_start();return;
  }
  track_storage_result=TRACK_JOURNAL_FOREIGN;
  track_page=8u;native_start();return;
 }
 /* The LCD gets an immediate activity state, then flash work starts.
  * NOR erase/program/CRC are synchronous and cannot be interrupted safely. */
 track_storage_result=beetle_project_save_staged();
 if(track_storage_result==TRACK_JOURNAL_OK){
  track_named=1u;track_slot_seen[track_state.project_slot]=1u;
  track_session_saved_slot=track_state.project_slot;track_state.dirty=0u;
  track_saved_toast_until=etch_ms+1300u;
 }
 native_start();
#else
 /* Read-only pilot build: no physical project writes. */
 track_storage_result=TRACK_JOURNAL_HARDWARE_LOCKED;
#endif
 track_overwrite_wait=0u;
 /* Only successful saves jump back into the sequencer. Failed writes retain
  * the explicit error page; nothing is silently reported as saved. */
 track_page=track_storage_result==TRACK_JOURNAL_OK?0u:8u;
}
static void beetle_project_new_session(void){
 uint8_t selected=track_state.project_slot;
 fm1_irq_off();
 track_chip_silence(&track_chip);track_drum_stop(&track_drum);
 track_init(&track_state);track_song_init(&track_song,&track_state);
 track_mixer_init(&track_mix);track_drum_init(&track_drum);
 track_chip_init(&track_chip);
 for(uint8_t k=0u;k<5u;k++)track_punch_base_param[k]=track_chip.punch_param[k];
 track_punch_clear(&track_rec_events);
 track_ui_inst_reset();
 for(uint8_t c=0;c<TRACK_SYNTH_CHANNELS;c++)track_lane_instrument[c]=(uint8_t)(c&3u);
 track_state.project_slot=selected;track_state.dirty=0u;
 track_live_punch_mask=0u;track_replay_punch_mask=0u;
 track_swing_pending_mask=0u;track_preview_mask=0u;
 for(uint8_t i=0;i<27u;i++){track_preview_channel[i]=255u;track_preview_note[i]=255u;}
 fm1_irq_enable_all();
 for(uint8_t i=0;i<=TRACK_NAME_MAX;i++)track_title[i]=0u;
 track_named=0u;track_unsaved_action=0u;track_session_saved_slot=255u;track_saved_toast_until=0u;track_page=0u;
}
static void beetle_project_open_load_browser(void){
 track_page=14u;track_load_feedback=TRACK_JOURNAL_NO_PROJECT;track_load_scan=0u;
 for(uint8_t i=0u;i<TRACK_PROJECTS;i++){
  track_load_status[i]=0u;track_load_titles[i][0]=0;
 }
}
static void beetle_project_scan_slot(uint8_t slot){
 if(slot>=TRACK_PROJECTS || track_load_status[slot])return;
#if BEETLE_NATIVE_STORAGE
 if(beetle_storage_last_audit!=BEETLE_CLAIM_EXISTING){
  /* Boot/save already performed the full sector ownership audit. Do not
   * re-scan 4 KiB for every one of the twelve menu items. */
  track_load_status[slot]=beetle_storage_last_audit==BEETLE_CLAIM_BLANK?2u:4u;
  return;
 }
 /* Read-only metadata preview: NEVER apply scanned project to live tracker. */
 char preview[13];
 int rc=track_journal_preview(&beetle_hal_journal_io,slot,preview);
 if(rc==TRACK_JOURNAL_OK){
  track_load_status[slot]=1u;track_slot_seen[slot]=1u;
  for(uint8_t j=0u;j<TRACK_NAME_MAX;j++)track_load_titles[slot][j]=preview[j];
  track_load_titles[slot][TRACK_NAME_MAX]=0u;
 }else track_load_status[slot]=rc==TRACK_JOURNAL_NO_PROJECT?2u:3u;
#else
 track_load_status[slot]=3u;
#endif
}
static void beetle_project_scan_one(void){
 /* Prioritize the visible selection, then read one other preview per tick.
  * All previews are tiny and bounded; input polling remains interactive. */
 if(track_load_status[track_state.project_slot]==0u)
  beetle_project_scan_slot(track_state.project_slot);
 while(track_load_scan<TRACK_PROJECTS && track_load_status[track_load_scan])track_load_scan++;
 if(track_load_scan<TRACK_PROJECTS)beetle_project_scan_slot(track_load_scan++);
}
static void beetle_storage_erase_progress(uint8_t done,uint8_t total){
 track_storage_progress_done=done;track_storage_progress_total=total;
 screen();
}
static void beetle_project_load_selected(void){
#if BEETLE_NATIVE_STORAGE
 if(!track_state.transport.playing&&!native_safe&&!native_tripped&&!native_updating){
  track_storage_busy_kind=2u;track_page=18u;screen();
  native_pause();track_storage_result=beetle_project_load_slot(track_state.project_slot);native_start();
  track_page=track_storage_result==TRACK_JOURNAL_OK?0u:14u;
  if(track_page==14u)track_load_feedback=track_storage_result;
 }else {track_storage_result=TRACK_JOURNAL_HARDWARE_LOCKED;track_page=8u;}
#else
 track_storage_result=TRACK_JOURNAL_HARDWARE_LOCKED;track_page=8u;
#endif
 track_unsaved_action=0u;
}
static void track_fm1_poll(void){
 uint32_t now=etch_ms;
 if(now-track_last_poll<18u)return;
 track_last_poll=now;
 uint32_t buttons=fm1_in.buttons, released=0;
 uint32_t pressed=fm1_input_edges(&released);
 int32_t k1=fm1_enc_take(2),k2=fm1_enc_take(3);
 int32_t k3=fm1_enc_take(4),k4=fm1_enc_take(5),sel=fm1_enc_take(0);
 int32_t alg=fm1_enc_take(1); /* physical ALGORITHM rotary */
 int32_t preset=fm1_enc_take(6); /* Physical PRESET: note / EMPTY / STP */
 uint8_t changed=(uint8_t)(pressed||released||k1||k2||k3||k4||sel||alg);
 track_runtime_validate();
 /* Drain the initial encoder/button pulses left by the ETCH boot splash. */
 if((int32_t)(now-track_boot_input_unlock_ms)<0){
  buttons=0u;pressed=0u;released=0u;k1=0;k2=0;k3=0;k4=0;sel=0;alg=0;changed=0u;
 }else{
  int32_t values[6]={sel,alg,k1,k2,k3,k4};
  for(uint8_t j=0u;j<6u;j++)if(values[j]){
   track_last_encoder_id=j;track_last_encoder_delta=values[j];track_last_encoder_ms=now;
  }
 }
 /* Shared keyboard octave: octaves shift LIVE keyboard notes in both
  * EDIT and TRACKER, never existing pattern cells. A simultaneous OCT chord
  * continues toggling mono/poly only in EDIT. The physical recovery combo
  * at power-up remains handled before this input loop. */
 if(track_page==0u || track_page==5u){
  enum etch_octave_action key_oct=etch_octave_scan(&track_key_oct_chord,buttons);
  if(key_oct==ETCH_OCT_MINUS||key_oct==ETCH_OCT_PLUS){
   native_controls.octave=beetle_keyboard_shift(native_controls.octave,
      key_oct==ETCH_OCT_MINUS?-1:1);
   changed=1u;
   /* Held EDIT notes are rearticulated at the new octave on next poll. */
   if(track_page==5u && track_edit_prev_keys){
    fm1_irq_off();track_chip_silence(&track_chip);fm1_irq_enable_all();
    track_edit_prev_keys=0u;
    for(uint8_t j=0u;j<27u;j++){track_edit_voice_key[j]=255u;track_edit_voice_note[j]=255u;}
   }
  }else if(key_oct==ETCH_OCT_TOGGLE && track_page==5u){
   track_edit_poly^=1u;
   fm1_irq_off();track_chip_silence(&track_chip);fm1_irq_enable_all();
   track_edit_prev_keys=0u;
   for(uint8_t j=0u;j<27u;j++){track_edit_voice_key[j]=255u;track_edit_voice_note[j]=255u;track_edit_order[j]=0u;}
   changed=1u;
  }
 }else { /* Never leave a half-pressed octave chord stuck across pages. */
  track_key_oct_chord.last=0u;track_key_oct_chord.combo_latched=0u;
 }
 if(track_page==13u){
  if(pressed&(1u<<8)){track_unsaved_action=0u;track_page=1u;track_home_fired=1u;track_home_since=now;}
  else if(pressed&(1u<<3)){
   uint8_t action=track_unsaved_action;
   track_unsaved_action=0u;
   if(action==1u){
    /* DISCARD must open the 12-project picker, NEVER auto-load slot 1. */
    beetle_project_new_session();beetle_project_open_load_browser();
   }else if(action==2u)beetle_project_new_session();
   else if(action==3u)beetle_project_load_selected();
  }
  if(changed||now-track_last_paint>=((track_state.transport.playing||(int32_t)(track_boot_splash_until-now)>0)?90u:850u)){screen();track_last_paint=now;}
  return;
 }
 if(track_page==12u){
  if(pressed&(1u<<8)){track_storage_confirm_since=0u;track_name_begin(&track_editor,track_title);track_page=2u;screen();return;}
#if BEETLE_NATIVE_STORAGE
  if(!native_safe&&!native_tripped&&!native_updating&&!track_state.transport.playing&&
     (buttons&((1u<<3)|(1u<<9)))==((1u<<3)|(1u<<9))){
   if(!track_storage_confirm_since)track_storage_confirm_since=now?now:1u;
   if(beetle_hold_finished(now,track_storage_confirm_since)){
    track_page=15u;track_storage_progress_done=0u;track_storage_phase=0u;
    track_storage_progress_total=(uint8_t)((BEETLE_FLASH_MARKER_END-BEETLE_FLASH_BASE)/BEETLE_FLASH_SECTOR);
    screen();native_pause();
    int fmt=beetle_hal_init_exclusive(beetle_storage_erase_progress);
    if(fmt==0){track_storage_phase=1u;screen();track_storage_result=beetle_project_save_staged();}
    else track_storage_result=TRACK_JOURNAL_IO;
    track_storage_just_initialized=(uint8_t)(fmt==0 && track_storage_result==TRACK_JOURNAL_OK);
    if(track_storage_result==TRACK_JOURNAL_OK){
     track_named=1u;track_slot_seen[track_state.project_slot]=1u;track_state.dirty=0u;
     track_session_saved_slot=track_state.project_slot;track_saved_toast_until=etch_ms+1300u;
    }
    native_start();track_overwrite_wait=0u;track_storage_confirm_since=0u;
    track_page=track_storage_result==TRACK_JOURNAL_OK?0u:8u;
    screen();return;
   }
  }else track_storage_confirm_since=0u;
#endif
  if(changed || now-track_last_paint>=250u){screen();track_last_paint=now;}
  return;
 }

 if(track_page==14u){
  track_preview_release(0u);(void)fm1_input_note_edges();
  if(k1||k4){track_state.project_slot=beetle_list_move(track_state.project_slot,k1+k4);track_load_feedback=TRACK_JOURNAL_NO_PROJECT;}
  if(pressed&(1u<<8)){track_page=1u;track_home_fired=1u;track_home_since=now;}
  else if(pressed&(1u<<3)){
   if(track_state.dirty){track_unsaved_action=3u;track_page=13u;}
   else if(track_load_status[track_state.project_slot]==2u){
    track_load_feedback=TRACK_JOURNAL_NO_PROJECT;
   }else beetle_project_load_selected();
  }
  if(track_page==14u && track_load_scan<TRACK_PROJECTS){beetle_project_scan_one();changed=1u;}
  if(changed||now-track_last_paint>=((track_state.transport.playing||(int32_t)(track_boot_splash_until-now)>0)?90u:850u)){screen();track_last_paint=now;}
  return;
 }
 /* HELP never changes patterns, instrument state, slots or tempo.
  * Trap navigation here so HOME does not accidentally arm long-press guide. */
 if(track_page==19u){
  if(k1||k4){
   track_help_offset=beetle_help_scroll(track_help_offset,k1+3*k4);
   changed=1u;
  }
  if(pressed&(1u<<8)){track_page=1u;track_home_fired=1u;track_home_since=now;changed=1u;}
  if(released&(1u<<8)){track_home_fired=0u;track_home_since=0u;}
  if(released&(1u<<11)){track_page=0u;changed=1u;}
  (void)fm1_input_note_edges();
  if(changed||now-track_last_paint>=750u){screen();track_last_paint=now;}
  return;
 }
 /* Allow preview voices to release even when switching channels/pages,
  * pressing PLAY or entering REC. No stuck notes after navigation. */
 track_preview_release((track_page==0u && !track_state.record_armed &&
                       !track_state.transport.playing)?fm1_in.notes:0u);

 /* Punch-In white keys: F G A B C D E F (left to right), hold-to-play.
  * Input read is from the existing Felucca debounced key matrix. */
 if(track_page==4u || track_state.record_armed){
  static const uint8_t white_key_pos[8]={0u,2u,4u,6u,7u,9u,11u,12u};
  uint32_t mask=fm1_in.notes;
  for(uint8_t i=0u;i<8u;i++){
   uint32_t bit=1u<<white_key_pos[i];uint8_t before=(uint8_t)((track_old_note_mask&bit)!=0u);
   uint8_t after=(uint8_t)((mask&bit)!=0u);
   if(before!=after){
    fm1_irq_off();
    if(after){track_live_punch_mask|=(uint8_t)(1u<<i);track_punch_focus=i;/* Preserve the ALGORITHM setting per FX across press/release. */}
    else {track_live_punch_mask&=(uint8_t)~(1u<<i);/* Preserve the ALGORITHM setting per FX across press/release. */
     if(track_punch_focus==i){track_punch_focus=255u;for(uint8_t j=0u;j<8u;j++)if(track_live_punch_mask&(1u<<j))track_punch_focus=j;}}
    track_state.punch_mask=(uint8_t)(track_live_punch_mask|track_replay_punch_mask);
    track_sync_punch_params();fm1_irq_enable_all();
    if(track_state.record_armed&&track_state.transport.playing){
     fm1_irq_off();
     (void)track_punch_add(&track_rec_events,track_state.transport.play_step,i,after);
     (void)track_song_record_value(&track_song,track_state.pattern_slot,
       track_state.transport.play_step,i,after,
       (after&&i<5u)?track_punch_base_param[i]:255u);
     track_state.dirty=1u;
     fm1_irq_enable_all();
    }
    changed=1u;
   }
  }
  track_old_note_mask=mask;
 }else if(track_live_punch_mask){
  fm1_irq_off();track_live_punch_mask=0u;track_punch_focus=255u;
  /* Keep each Punch-In ALGORITHM setting when leaving the page. */
  track_state.punch_mask=track_replay_punch_mask;track_sync_punch_params();fm1_irq_enable_all();
  track_old_note_mask=0u;changed=1u;
 }

  /* ALGORITHM edits a held Punch-In and remembers its value until reconfigured.
  * Octaves/MUTE deliberately have no parameter. Otherwise ALG chooses
  * the current pattern in the TRACK screen; SELECT remains channel turn. */
 if(alg){
  uint8_t fx=track_punch_focus;
  if(fx>=5u){fx=255u;for(uint8_t j=0u;j<5u;j++)if(track_live_punch_mask&(1u<<j))fx=j;}
  if(fx<5u && (track_live_punch_mask&(1u<<fx))){
   int32_t value=(int32_t)track_punch_base_param[fx]+alg*3;
   if(value<0)value=0;if(value>100)value=100;
   fm1_irq_off();
   if(track_punch_base_param[fx]!=(uint8_t)value){
    track_punch_base_param[fx]=(uint8_t)value;
    if(track_state.record_armed&&track_state.transport.playing){
     (void)track_song_record_value(&track_song,track_state.pattern_slot,
             track_state.transport.play_step,fx,2u,(uint8_t)value);
    }
    track_state.dirty=1u;
   }
   track_sync_punch_params();fm1_irq_enable_all();
  }else if(!track_live_punch_mask && track_page==0u && !track_state.transport.playing){
   int32_t pat=(int32_t)track_state.pattern_slot+alg;
   if(pat<0)pat=0;if(pat>=TRACK_PATTERNS)pat=TRACK_PATTERNS-1;
   fm1_irq_off();(void)track_song_switch(&track_song,&track_state,(uint8_t)pat);fm1_irq_enable_all();
  }
 }
 uint32_t mixer_key_pressed=0u;
 if(track_page==11u && (pressed&(1u<<8))){
  track_overwrite_wait=0u;track_page=2u;track_home_fired=1u;changed=1u;
 }
 if(buttons&(1u<<8)){
  if(!track_home_since)track_home_since=now;
  if(!track_home_fired&&track_page!=11u&&now-track_home_since>=1500u){track_page=7u;track_home_fired=1u;changed=1u;}
 }
 if(released&(1u<<8)){
  if(!track_home_fired)track_page=1u;
  track_home_since=0u;track_home_fired=0u;changed=1u;
 }
 /* SEQ is tracker/home; SEQ+OP1/OP2 copy/paste the selected pattern lane. */
 if(pressed&(1u<<11)){track_seq_since=now?now:1u;track_seq_fired=0u;}
 if((buttons&(1u<<11))&&track_seq_since&&!track_seq_fired&&now-track_seq_since>=1200u){
  track_seq_fired=1u; /* block unwanted short-press when using chord */
 }
 if(released&(1u<<11)){
  if(!track_seq_fired){track_page=0u;changed=1u;}
  track_seq_since=0u;track_seq_fired=0u;/* Copy persists across SEQ releases and pattern switches. */
 }
 if(pressed&(1u<<9)){
  if(track_page==2u)track_complete_save();else if(track_named){track_name_begin(&track_editor,track_title);track_complete_save();}else track_begin_save();
  changed=1u;
 }
 if(track_page==11u){
  if(pressed&(1u<<3)){track_overwrite_wait=1u;track_complete_save();changed=1u;}
 }else if(track_page==2u){
  if(k1)track_name_move(&track_editor,k1);
  if(k2)track_name_change(&track_editor,k2);
  if(k4){
   int32_t slot=(int32_t)track_state.project_slot+k4;
   if(slot<0)slot=0;if(slot>=TRACK_PROJECTS)slot=TRACK_PROJECTS-1;
   track_state.project_slot=(uint8_t)slot;
  }
 }else{
  if(pressed&(1u<<2)){
    if(track_page!=3u && track_state.transport.edit_channel==TRACK_DRUM_CHANNEL)
     track_fx_drum_lane=(uint8_t)(track_state.transport.edit_column==1u);
    track_page=track_page==3u?4u:3u;
   }
  if(pressed&(1u<<6)){track_page=(uint8_t)(track_page==5u?16u:5u);track_osc_cursor=0u;track_osc_bank=0u;track_type_queued=0;}
  if(pressed&(1u<<5)){track_page=17u;track_osc_cursor=0u;} /* LFO => MASTER */
  if(pressed&(1u<<10)){track_page=9u;} /* ARP => SONG */
  /* GLO is the dedicated MIXER shortcut. ARP alone opens SONG.
   * A second press returns to TRACKER, never opens SONG. */
  if(pressed&(1u<<7))track_page=(track_page==6u)?0u:6u;
  /* Do not toggle record at button-down: a long press is a separate gesture. */
  if(pressed&(1u<<13)){track_rec_since=now?now:1u;track_rec_long=0u;}
  /* REC hold no longer changes pages: virtual lane 0 owns the Punch-In
   * recording editor. Short and long REC presses both toggle at release. */
  if(released&(1u<<13)){
   if(!track_rec_long){
    track_state.record_armed^=1u;
    if(!track_state.record_armed){fm1_irq_off();track_live_punch_mask=0u;track_state.punch_mask=track_replay_punch_mask;fm1_irq_enable_all();}
   }
   track_rec_since=0u;track_rec_long=0u;changed=1u;
  }
  if(pressed&(1u<<12)){
   fm1_irq_off();
   if(track_state.transport.playing){
    track_stop(&track_state);track_live_punch_mask=0u;track_replay_punch_mask=0u;track_sync_punch_params();
    if(track_song_follow_active){track_song.edit_row=track_song_preplay_row;track_song_follow_active=0u;}
   }else{
    if(track_song.enabled){
     track_song_preplay_row=track_song.edit_row;
     track_song_follow_active=1u;
     track_song_start(&track_song,&track_state);
     track_song.edit_row=track_song.position;
    }
    track_play(&track_state);
   }
   fm1_irq_enable_all();
  }
  if(track_scroll_preview_active && ((int32_t)(now-track_scroll_preview_until)>=0 || track_page!=0u || track_state.transport.playing)){
  fm1_irq_off();track_chip_note_off(&track_chip,track_scroll_preview_channel,track_scroll_preview_note);
  track_scroll_preview_active=0u;fm1_irq_enable_all();
 }
 if(track_page==0u){
   fm1_irq_off();
   if(pressed&(1u<<3)){
    (void)track_clear_step(&track_state,track_state.transport.edit_channel,track_state.transport.edit_step);
    changed=1u;
   }
   if(k1)track_state.transport.edit_step=track_row_move(track_state.transport.edit_step,k1);
   if(k2)track_state.transport.edit_column=track_column_move(track_state.transport.edit_column,k2);
   if(k3){
    /* K3 NOTE scroll auditions the final selected pitch; it must not wait
     * for physical-key release or cause transport note insertion. */
    uint8_t audition=(uint8_t)(track_state.transport.edit_column==0u&&!track_state.transport.playing);
    if(audition&&track_scroll_preview_active){
     track_chip_note_off(&track_chip,track_scroll_preview_channel,track_scroll_preview_note);
     track_scroll_preview_active=0u;
    }
    track_wheel_value(k3);
    if(audition){
     uint8_t lane=track_state.transport.edit_channel;
     const struct track_step *st=&track_state.active.ch[lane][track_state.transport.edit_step];
     if(lane==TRACK_DRUM_CHANNEL){if(st->note<TRACK_DRUM_COUNT)track_drum_trigger(&track_drum,st->note);}
     else if(st->note<128u){
      if(track_chip_note(&track_chip,lane,st->instrument&3u,st->note,100u)){
       track_scroll_preview_channel=lane;track_scroll_preview_note=st->note;
       track_scroll_preview_active=1u;track_scroll_preview_until=now+155u;
      }
     }
    }
   }
   if(preset && track_state.transport.edit_column==0u){
    struct track_step *st=&track_state.active.ch[track_state.transport.edit_channel][track_state.transport.edit_step];
    if(st->note<TRACK_NOTE_STOP)track_preset_last_note=st->note;
    const uint8_t drum=(uint8_t)(track_state.transport.edit_channel==TRACK_DRUM_CHANNEL);
    if(drum&&track_preset_last_note>=TRACK_DRUM_COUNT)track_preset_last_note=0u;
    int32_t index=st->note==TRACK_NOTE_EMPTY?1:st->note==TRACK_NOTE_STOP?2:0;
    index=((index+(preset%3))%3+3)%3;
    st->note=index==1?TRACK_NOTE_EMPTY:index==2?TRACK_NOTE_STOP:track_preset_last_note;
    track_state.dirty=1u;changed=1u;
   }
   if(pressed&(1u<<4)){ /* ENV key: instant step note-off */
    struct track_step *st=&track_state.active.ch[track_state.transport.edit_channel][track_state.transport.edit_step];
    st->note=TRACK_NOTE_STOP;st->chord=0u;track_state.dirty=1u;changed=1u;
   }
   if(sel){int32_t target=(int32_t)track_state.transport.edit_channel+sel;
    if(target<0){track_page=10u;track_punch_edit_row=track_state.transport.edit_step;track_punch_edit_index=0u;}
    else {if(target>=TRACK_CHANNELS)target=TRACK_CHANNELS-1u;track_state.transport.edit_channel=(uint8_t)target;}
   }
   /* OCT-/+ now control the physical keyboard's octave here too.
    * K3 continues to edit instrument presets and NOTE pitch as before. */
   fm1_irq_enable_all();
   /* Physical keyboard: NOTE entry advances exactly one step (15 -> 00).
    * NOTE2 on the drum channel changes only the second hit, no advance.
    * K3 edits still stay on the selected row for precise pitch adjustment. */
   /* SEQ+OP1 copies, SEQ+OP2 pastes a full pattern lane, regardless of
    * selected editor column; no keyboard note gets accidentally entered. */
   if((buttons&(1u<<11)) && !track_state.record_armed){
    uint32_t combo_edges=fm1_input_note_edges();
    uint8_t lane=track_state.transport.edit_channel;
    if(combo_edges&(1u<<1u)){
     fm1_irq_off();(void)track_lane_clip_copy(&beetle_lane_clip,&track_state.active,lane);
     fm1_irq_enable_all();beetle_lane_clip.valid=1u;track_seq_fired=1u;changed=1u;
    }
    if((combo_edges&(1u<<3u)) && beetle_lane_clip.valid){
     fm1_irq_off();(void)track_lane_clip_paste(&beetle_lane_clip,&track_state.active,lane);
     track_state.dirty=1u;fm1_irq_enable_all();track_seq_fired=1u;changed=1u;
    }
   }
   if(!track_state.record_armed && !(buttons&(1u<<11)) && (track_state.transport.edit_column==0u ||
      (track_state.transport.edit_channel==TRACK_DRUM_CHANNEL && track_state.transport.edit_column==1u))){
    uint32_t keypressed=fm1_input_note_edges();
    if(buttons&(1u<<11))keypressed=0u; /* Chords handled at page level below. */
    for(uint8_t k=0u;k<27u;k++)if(keypressed&(1u<<k)){
     const uint8_t ch=track_state.transport.edit_channel;
     const uint8_t note=(uint8_t)(ch==TRACK_DRUM_CHANNEL?k%TRACK_DRUM_COUNT:
                           beetle_keyboard_pitch(k,native_controls.octave));
     fm1_irq_off();
     if(ch==TRACK_DRUM_CHANNEL && track_state.transport.edit_column==1u){
      track_state.active.ch[ch][track_state.transport.edit_step].chord=(uint8_t)(1u+note);
      track_state.dirty=1u;
     }else (void)track_enter_note(&track_state,note);
     track_preset_last_note=note;
     /* Key audition works while STOPPED; playback itself owns voices during
      * transport. Audio ISR renders these native chip/drum voices. */
     if(!track_state.transport.playing){
      if(ch==TRACK_DRUM_CHANNEL)(void)track_drum_trigger(&track_drum,note);
      else if(track_chip_note(&track_chip,ch,
           track_state.active.ch[ch][(uint8_t)((track_state.transport.edit_step+TRACK_STEPS-1u)%TRACK_STEPS)].instrument,
           note,100u)){
       track_preview_channel[k]=ch;track_preview_note[k]=note;track_preview_mask|=(1u<<k);
      }
     }
     fm1_irq_enable_all();changed=1u;
    }
   }else (void)fm1_input_note_edges();
  }else{
   if(track_page==5u && !track_state.record_armed){
    /* Never write pattern notes in EDIT. Only audition the currently selected
     * instrument; release keys explicitly gate their voices. */
    uint32_t held=fm1_in.notes;
    uint32_t onset=held&~track_edit_prev_keys,offs=track_edit_prev_keys&~held;
    /* Reject contact bounce while keeping held-note sound continuous. */
    for(uint8_t k=0u;k<27u;k++)if(onset&(1u<<k)){
     if(track_edit_last_onset_ms[k] && (uint32_t)(now-track_edit_last_onset_ms[k])<45u)onset&=~(1u<<k);
     else track_edit_last_onset_ms[k]=now?now:1u;
    }
    for(uint8_t k=0u;k<27u;k++)if(offs&(1u<<k)){
     if(track_edit_voice_key[k]<TRACK_CHIP_VOICES){
      const struct track_chip_voice *v=&track_chip.voices[track_edit_voice_key[k]];
      if(v->active&&v->note==track_edit_voice_note[k])
       track_chip_note_off(&track_chip,v->channel,track_edit_voice_note[k]);
     }
     track_edit_voice_key[k]=255u;track_edit_voice_note[k]=255u;
    }
    if(track_state.transport.edit_channel==TRACK_DRUM_CHANNEL){
     for(uint8_t k=0u;k<27u;k++)if(onset&(1u<<k))
      (void)track_drum_trigger(&track_drum,(uint8_t)(k%TRACK_DRUM_COUNT));
    }else{
     const uint8_t ch=track_state.transport.edit_channel;
     const struct track_step *st=&track_state.active.ch[ch][track_state.transport.edit_step];
     for(uint8_t k=0u;k<27u;k++)if(onset&(1u<<k)){
      if(!track_edit_poly){track_chip_silence(&track_chip);
       for(uint8_t j=0u;j<27u;j++){track_edit_voice_key[j]=255u;track_edit_voice_note[j]=255u;}}
      track_edit_order[k]=++track_edit_serial;
      uint8_t pitch=beetle_keyboard_pitch(k,native_controls.octave);
      if(track_chip_note(&track_chip,ch,(uint8_t)(st->instrument&3u),pitch,100u)){
       track_edit_voice_key[k]=track_chip.last_voice_index;track_edit_voice_note[k]=pitch;
      }
     }
    }
    /* Mono last-note priority with return to previously held note. */
    if(!track_edit_poly&&offs&&!(onset)&&held&&track_state.transport.edit_channel!=TRACK_DRUM_CHANNEL){
     uint8_t newest=255u;uint32_t latest=0u;
     for(uint8_t j=0u;j<27u;j++)if((held&(1u<<j))&&track_edit_order[j]>=latest){latest=track_edit_order[j];newest=j;}
     if(newest<27u && track_edit_voice_key[newest]==255u){const struct track_step *st=&track_state.active.ch[track_state.transport.edit_channel][track_state.transport.edit_step];
      track_chip_silence(&track_chip);
      for(uint8_t j=0u;j<27u;j++){track_edit_voice_key[j]=255u;track_edit_voice_note[j]=255u;}
      uint8_t pitch=beetle_keyboard_pitch(newest,native_controls.octave);
      if(track_chip_note(&track_chip,track_state.transport.edit_channel,(uint8_t)(st->instrument&3u),pitch,100u)){
       track_edit_voice_key[newest]=track_chip.last_voice_index;track_edit_voice_note[newest]=pitch;
      }
     }
    }
    track_edit_prev_keys=held;
    (void)fm1_input_note_edges();
    if(onset||offs)changed=1u;
   }else{
    if(track_edit_prev_keys){track_audio_all_off(&native_audio);track_edit_prev_keys=0u;
     for(uint8_t j=0u;j<27u;j++){track_edit_voice_key[j]=255u;track_edit_voice_note[j]=255u;}}
    if(track_page==6u)mixer_key_pressed=fm1_input_note_edges();
    else (void)fm1_input_note_edges();
   }
   if(k4&&track_page==9u){fm1_irq_off();track_set_bpm(&track_state,(uint16_t)((int32_t)track_state.transport.bpm+k4));fm1_irq_enable_all();}
   if(track_page==9u){
    if(k1){int32_t row=(int32_t)track_song.edit_row+k1;
     if(row<0)row=0;if(row>=track_song.count)row=track_song.count-1u;
     track_song.edit_row=(uint8_t)row;
    }
    if(k2){int32_t col=(int32_t)track_song.edit_column+k2;
     if(col<0)col=0;if(col>1)col=1;track_song.edit_column=(uint8_t)col;}
    if(k3){fm1_irq_off();(void)track_song_change(&track_song,track_song.edit_row,track_song.edit_column,k3);track_state.dirty=1u;fm1_irq_enable_all();}
    if(pressed&2u){fm1_irq_off();(void)track_song_insert(&track_song,track_song.edit_row);track_state.dirty=1u;fm1_irq_enable_all();}
    if(pressed&1u){fm1_irq_off();(void)track_song_delete(&track_song,track_song.edit_row);track_state.dirty=1u;fm1_irq_enable_all();}
    if(pressed&(1u<<3)){
     fm1_irq_off();track_song.enabled^=1u;fm1_irq_enable_all();changed=1u;
    }
   }
   if(track_page==10u){
    if(sel>0){track_page=0u;track_state.transport.edit_channel=0u;changed=1u;}
    struct track_song_clip *clip=&track_song.clips[track_state.pattern_slot];
    if(k1){int32_t row=(int32_t)track_punch_edit_row+k1;
     if(row<0)row=0;if(row>=TRACK_STEPS)row=TRACK_STEPS-1u;
     track_punch_edit_row=(uint8_t)row;track_punch_edit_index=0u;
    }
    uint8_t positions[TRACK_SONG_FX_EVENTS],count=0u;
    for(uint8_t i=0u;i<clip->count;i++)if(clip->data[i].step==track_punch_edit_row)positions[count++]=i;
    if(k2){int32_t idx=(int32_t)track_punch_edit_index+k2;if(idx<0)idx=0;
     if(count && idx>=count)idx=count-1u;track_punch_edit_index=(uint8_t)idx;}
    if(count&&track_punch_edit_index>=count)track_punch_edit_index=(uint8_t)(count-1u);
    if(k3&&count){struct track_song_event *ev=&clip->data[positions[track_punch_edit_index]];
     int32_t fx=(int32_t)ev->fx+k3;ev->fx=(uint8_t)((fx%8+8)%8);track_state.dirty=1u;}
    if((pressed&2u)&&clip->count<TRACK_SONG_FX_EVENTS){
     struct track_song_event *ev=&clip->data[clip->count++];
     ev->step=track_punch_edit_row;ev->fx=0u;ev->down=1u;ev->param=255u;
     track_punch_edit_index=count;track_state.dirty=1u;
    }
    if((pressed&1u)&&count){uint8_t index=positions[track_punch_edit_index];
     for(uint8_t i=index;i+1u<clip->count;i++)clip->data[i]=clip->data[i+1u];
     clip->count--;track_punch_edit_index=0u;track_state.dirty=1u;
    }
    if((pressed&(1u<<3))&&count){
     /* Clear all recorded Punch-In events on this step, not just the selected event. */
     for(int i=(int)clip->count-1;i>=0;i--){
      if(clip->data[i].step==track_punch_edit_row){
       for(uint8_t j=(uint8_t)i;j+1u<clip->count;j++)clip->data[j]=clip->data[j+1u];
       clip->count--;}
     }
     track_punch_edit_index=0u;track_state.dirty=1u;
    }
   }
   if(track_page==3u){
    if(k2&&track_state.transport.edit_channel==TRACK_DRUM_CHANNEL)track_fx_drum_lane^=1u;
    if(k1)track_fx_cursor=(uint8_t)((int32_t)track_fx_cursor+k1>0?1u:0u);
    if(k3){
     fm1_irq_off();
     struct track_step *st=&track_state.active.ch[track_state.transport.edit_channel][track_state.transport.edit_step];
     if(track_fx_cursor==0u){int32_t v=(int32_t)st->effect+k3;
      uint8_t next=(uint8_t)((v%TRACK_FX_COUNT+TRACK_FX_COUNT)%TRACK_FX_COUNT);
      if(next==TRACK_FX_VOLUME && st->effect!=TRACK_FX_VOLUME)st->value=100u;
      if(next==TRACK_FX_DRUM_PITCH && st->effect!=TRACK_FX_DRUM_PITCH)st->value=12u;
      st->effect=next;
     }else{int32_t v=(int32_t)st->value+k3;
      if(v<0)v=0;if(v>(st->effect==TRACK_FX_DRUM_PITCH?24:100))v=st->effect==TRACK_FX_DRUM_PITCH?24:100;st->value=(uint8_t)v;}
     track_state.dirty=1u;fm1_irq_enable_all();
    }
   }
   if(track_page==5u){
    if(k2){int32_t b=(int32_t)track_osc_bank+k2;
     track_osc_bank=(uint8_t)((b%4+4)%4);track_osc_cursor=0u;}
    if(k1){int32_t v=(int32_t)track_osc_cursor+k1;
     uint8_t max_row=track_osc_bank==3u?6u:track_osc_bank?3u:7u;
     if(v<0)v=0;if(v>max_row)v=max_row;track_osc_cursor=(uint8_t)v;}
    if(track_osc_bank==0u && track_osc_cursor==0u){
     int32_t accum=(int32_t)track_type_queued+k3;
     if(accum>32)accum=32;if(accum< -32)accum=-32;
     track_type_queued=(int16_t)accum;
     if(track_type_queued>0){k3=1;track_type_queued--;changed=1u;}
     else if(track_type_queued<0){k3=-1;track_type_queued++;changed=1u;}
     else k3=0;
    }else track_type_queued=0;
    if(k3){fm1_irq_off();
     struct track_step *st=&track_state.active.ch[track_state.transport.edit_channel][track_state.transport.edit_step];
     uint8_t ch=track_state.transport.edit_channel;
     struct track_instrument_ui_values *ui=&track_ui_inst[st->instrument&3u];
     int32_t v=0;
     if(track_osc_bank){
      struct track_va_patch *p=&track_chip.patches[st->instrument&3u];
      if(track_osc_bank==1u||track_osc_bank==2u){
       uint8_t b=(uint8_t)(track_osc_bank==2u);
       if(track_osc_cursor==0u){
        uint8_t *wave=b?&p->wave_b:&p->wave_a;
        *wave=(uint8_t)(((int)*wave+k3%5+5)%5);
       }else if(track_osc_cursor==1u){
        int8_t *coarse=b?&p->coarse_b:&p->coarse;
        v=(int32_t)*coarse+k3;if(v< -24)v=-24;if(v>24)v=24;*coarse=(int8_t)v;
       }else if(track_osc_cursor==2u){
        int8_t *fine=b?&p->fine_b:&p->fine;
        v=(int32_t)*fine+k3*5;if(v< -100)v=-100;if(v>100)v=100;*fine=(int8_t)v;
       }else{
        uint8_t *width=b?&p->pulse_width_b:&p->pulse_width;
        v=(int32_t)*width+k3*5;if(v<5)v=5;if(v>95)v=95;*width=(uint8_t)v;
       }
      }else{
       uint8_t *target=track_osc_cursor==0u?&p->detune:track_osc_cursor==1u?&p->mix:
          track_osc_cursor==2u?&p->morph:track_osc_cursor==3u?&p->drive:
          track_osc_cursor==4u?&p->sub:track_osc_cursor==5u?&p->noise:&p->fm_depth;
       v=(int32_t)*target+k3*5;if(v<0)v=0;if(v>100)v=100;*target=(uint8_t)v;
      }
     }else if(track_osc_cursor==0u){v=(int32_t)track_chip.osc_type[st->instrument&3u]+k3;
      track_chip.osc_type[st->instrument&3u]=(uint8_t)((v+(int32_t)TRACK_CHIP_TYPES)%TRACK_CHIP_TYPES);
     }else if(track_osc_cursor>=1u&&track_osc_cursor<=4u){
      uint8_t *ptr=track_osc_cursor==1u?&ui->attack:track_osc_cursor==2u?&ui->decay:
       track_osc_cursor==3u?&ui->sustain:&ui->release;
      v=(int32_t)*ptr+k3;if(v<0)v=0;if(v>99)v=99;*ptr=(uint8_t)v;
      if(track_osc_cursor==1u)track_chip.env_attack[st->instrument&3u]=(uint8_t)v;
      else if(track_osc_cursor==2u)track_chip.env_decay[st->instrument&3u]=(uint8_t)v;
      else if(track_osc_cursor==3u)track_chip.env_sustain[st->instrument&3u]=(uint8_t)v;
      else track_chip.env_release[st->instrument&3u]=(uint8_t)v;
     }else if(track_osc_cursor==5u){
      v=(int32_t)ui->duty+k3;if(v<1)v=1;if(v>99)v=99;ui->duty=(uint8_t)v;
      track_chip.duty[ch]=(uint8_t)(v<19?0:v<38?1:v<63?2:3);
     }else if(track_osc_cursor==6u){
      v=(int32_t)ui->cutoff+k3*100;if(v<100)v=100;if(v>18000)v=18000;ui->cutoff=(uint16_t)v;
      track_chip.cutoff[ch]=(uint8_t)(v/75u>250u?250u:v/75u);
     }else if(track_osc_cursor==7u){
      v=(int32_t)ui->volume+k3;if(v<0)v=0;if(v>100)v=100;ui->volume=(uint8_t)v;
      /* This also changes current channel loudness until project storage is available. */
      track_mix.ch[ch].volume=(uint8_t)v;
     }
     /* Live edits update ALL active voices using this shared patch; waveform,
      * tuning, FM and ADSR changes never require a fresh keypress. */
     track_chip_refresh_instrument(&track_chip,(uint8_t)(st->instrument&3u));
     track_state.dirty=1u;fm1_irq_enable_all();}
   }
   if(track_page==16u){
    if(sel){int32_t v=(int32_t)track_drum_component+sel;if(v<0)v=0;if(v>=TRACK_DRUM_COUNT)v=TRACK_DRUM_COUNT-1;track_drum_component=(uint8_t)v;}
    if(k1){int32_t v=(int32_t)track_drum_field+k1;if(v<0)v=0;if(v>8)v=8;track_drum_field=(uint8_t)v;}
    if(k3){
     struct track_drum_timbre *p=&track_drum.components[track_drum_component];
     uint8_t *fields[]={&p->attack,&p->decay,&p->sustain,&p->release,0,&p->eq_low,&p->eq_mid,&p->eq_high,&p->gain};
     int32_t v;if(track_drum_field==4u){v=(int32_t)p->pitch+k3;if(v< -36)v=-36;if(v>36)v=36;p->pitch=(int8_t)v;}
     else{v=(int32_t)*fields[track_drum_field]+k3;if(v<0)v=0;if(v>100)v=100;*fields[track_drum_field]=(uint8_t)v;}
     track_state.dirty=1u;
    }
    if(pressed&(1u<<3)){track_drum_component_default(&track_drum.components[track_drum_component],track_drum_component);track_state.dirty=1u;}
   }
   if(track_page==17u){
    if(sel){int32_t v=(int32_t)track_master_focus+sel;if(v<0)v=0;if(v>2)v=2;track_master_focus=(uint8_t)v;}
    uint8_t fx=track_master_cfg.order[track_master_focus];
    if(pressed&(1u<<4)){track_master_cfg.enabled[fx]^=1u;track_state.dirty=1u;}
    if(k4 && (buttons&(1u<<11))){
      int32_t j=(int32_t)track_master_focus+(k4>0?1:-1);
      if(j>=0&&j<3){uint8_t x=track_master_cfg.order[track_master_focus];track_master_cfg.order[track_master_focus]=track_master_cfg.order[j];track_master_cfg.order[j]=x;track_master_focus=(uint8_t)j;track_state.dirty=1u;}
     }
    if(fx==0u){int32_t v;int32_t d[4]={k1,k2,k3,0};
     /* K1..K4 edit each band. OCT- changes Q/width; OCT+ shifts center.
      * Hold SEQ with K4 to reorder effect blocks instead. */
     if(!(buttons&(1u<<11)))d[3]=k4;
     for(uint8_t i=0u;i<4u;i++)if(d[i]){
      uint8_t *field=(buttons&1u)?&track_master_cfg.eq_width[i]:
         (buttons&2u)?&track_master_cfg.eq_freq[i]:&track_master_cfg.eq[i];
      v=(int32_t)*field+d[i];if(v<0)v=0;if(v>100)v=100;*field=(uint8_t)v;track_state.dirty=1u;
     }
    }else{
     uint8_t *f0=fx==1u?&track_master_cfg.comp_threshold:&track_master_cfg.reverb_wet;
     uint8_t *f1=fx==1u?&track_master_cfg.comp_ratio:&track_master_cfg.reverb_feedback;
     uint8_t *f2=fx==1u?&track_master_cfg.comp_gain:&track_master_cfg.reverb_tone;
     uint8_t *f3=fx==1u?&track_master_cfg.comp_attack:&track_master_cfg.reverb_spread;
     uint8_t *fields[4]={f0,f1,f2,f3};int32_t d[4]={k1,k2,k3,(buttons&(1u<<11))?0:k4};
     for(uint8_t i=0u;i<4u;i++)if(d[i]){int32_t v=(int32_t)*fields[i]+d[i];if(v<0)v=0;if(v>100)v=100;*fields[i]=(uint8_t)v;track_state.dirty=1u;}
    }
   }
   if(track_page==6u){
    if(k1||k2)track_mixer_move(&track_mix,k1,k2);
    if(k3){fm1_irq_off();if(track_mixer_change(&track_mix,k3))track_state.dirty=1u;fm1_irq_enable_all();}
    /* Six left-most physical black keys are direct lane-mute toggles.
     * Press edges only, not continuously held keys. Disabled for other pages. */
    if(mixer_key_pressed){
     fm1_irq_off();
     for(uint8_t k=0u;k<27u;k++)if(mixer_key_pressed&(1u<<k)){
      int8_t lane=beetle_black_mute_lane(k);
      if(lane>=0){track_mix.ch[(uint8_t)lane].mute^=1u;track_state.dirty=1u;}
     }
     fm1_irq_enable_all();
    }
   }
   if(track_page==1u && k4){
    int32_t v=(int32_t)track_state.project_slot+k4;
    if(v<0)v=0;if(v>=(int32_t)TRACK_PROJECTS)v=TRACK_PROJECTS-1;track_state.project_slot=(uint8_t)v;
   }
   if(track_page==1u && k1){
    int32_t m=(int32_t)track_menu_index+k1;if(m<0)m=0;if(m>4)m=4;track_menu_index=(uint8_t)m;
   }
   if(track_page==1u && (pressed&(1u<<3))){
    if(track_menu_index==0u){
     if(track_named){track_name_begin(&track_editor,track_title);track_complete_save();}
     else track_begin_save();
    }else if(track_menu_index==1u){
     /* SAVE AS always opens the title editor. K4 can select any of 12 slots. */
     if(track_state.project_slot+1u<TRACK_PROJECTS)track_state.project_slot++;
     track_overwrite_wait=0u;track_begin_save();
    }else if(track_menu_index==4u){
     track_help_offset=0u;track_page=19u;changed=1u;
    }else if(track_menu_index==3u){
     if(track_state.dirty){track_unsaved_action=2u;track_page=13u;}
     else beetle_project_new_session();
    }else{
     if(track_state.dirty){track_unsaved_action=1u;track_page=13u;}
     else beetle_project_open_load_browser();
    }
   }
   if(track_page==7u){
    if(k1)track_state.harmony.root=(uint8_t)(((int32_t)track_state.harmony.root+k1%12+12)%12);
    if(k2)track_state.harmony.mode=(uint8_t)(((int32_t)track_state.harmony.mode+k2%9+9)%9);
    if(k3)track_state.harmony.keylock=(uint8_t)(((int32_t)track_state.harmony.keylock+k3%3+3)%3);
   }
  }
 }
 /* Song playback row follows actual sequencer position, independent of STEP. */
 if(track_song_follow_active){
  if(track_state.transport.playing && track_song.enabled){
   track_song.edit_row=track_song.position;
  }else{
   track_song.edit_row=track_song_preplay_row;
   track_song_follow_active=0u;
  }
 }
 /* KNOB4 controls BPM outside the name overlay only. */
 if(k4 && track_page==0u){fm1_irq_off();track_set_bpm(&track_state,(uint16_t)((int32_t)track_state.transport.bpm+k4));fm1_irq_enable_all();}
 track_fm1_leds();
 /* Full LCD repaint only on visible step/content change. This prevents
  * 30fps whole-screen DMA from stealing the 44.1kHz audio deadline. */
 static uint8_t previous_play_step=255u;
 uint8_t play_step=track_state.transport.play_step;
 if(changed || (track_state.transport.playing && play_step!=previous_play_step) ||
    now-track_last_paint>=((int32_t)(track_boot_splash_until-now)>0?130u:
       (track_page==6u&&track_state.transport.playing)?100u:750u)){
  screen();track_last_paint=now;
 }
 previous_play_step=play_step;
}
#endif
