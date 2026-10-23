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
#include "beetle_settings_nav.h"
#include "track_lane_clipboard.h"
#include "beetle_step_selection.h"
#include "beetle_pattern_edit.h"
#include "track_ui_controls.h"
#include "track_mixer.h"
#include "track_swing.h"
#include "beetle_step_groove.h"
#include "beetle_groove_tools.h"
#include "beetle_quantized_mute.h"
#include "beetle_live_mixer.h"
#include "beetle_drum_step_stop.h"
#include "beetle_performance_snapshots.h"
#include "beetle_mute_groups.h"
#include "track_punch_events.h"
#include "track_song.h"
#include "beetle_song_rows.h"
#include "beetle_battery_policy.h"
#include "beetle_mono_audition.h"
#include "beetle_reverse_key.h"
#include "beetle_led_ripple.h"
#include "track_drum.h"
#if defined(BEETLE_NATIVE_V2) && BEETLE_NATIVE_V2
#include "beetle_bd4_live.h"
#endif
#include "track_master_fx.h"
#include "track_project_store.h"
#include "beetle_instrument_library.h"
#include "track_journal.h"
#define TRACK_PINK 0xF81Au
#define TRACK_DATA_PINK 0xF81Au
#define TRACK_BRIGHT 0xFDDEu
#define TRACK_ROW_BG 0x3005u
#define TRACK_WHITE 0xFFFFu
#define TRACK_DARK 0x0000u
#include "beetle_keyboard_octave.h"
#include "beetle_help.h"
#include "beetle_pattern_length_shortcut.h"
#include "beetle_input_safety.h"
#include "beetle_punch_timing.h"
#include "beetle_playhead_update.h"
/* RC: relocate only the always-initialized sequencer state from the
 * near-full 96KiB BSS to the established separately guarded RAM pool.
 * track_init() initializes every field before the audio ISR is started. */
static struct track_engine track_state __attribute__((section(".pool"),aligned(4)));
static struct track_drum_engine track_drum __attribute__((section(".beetle_song"),aligned(16)));
static struct track_master_settings track_master_cfg;
static struct track_master_fx_state track_master_dsp __attribute__((section(".beetle_song"),aligned(64)));
static uint8_t track_drum_component,track_drum_field;
static uint8_t track_master_focus;
static uint8_t track_master_chain_edit;
/* All six lanes are mixed with independent stereo PAN. The post-master
 * stereo output is passed unchanged to USB UAC and analogue DMA. */
/* Capture buffers are pre-MASTER, six discrete PCM16 lanes. ALNK0 only.
 * Muted channels are silent; panning is intentionally NOT baked into mono stems.
 * The normal stereo recording remains post-MASTER and keeps pan. */
/* track_mix is declared before audio_target.c so ISR can render it. */
static struct track_punch_events track_rec_events;
/* 48 patterns and 64 song entries live in RAM and are journalled to flash on SAVE. */
/* TRACK 0.8: place 48-pattern RAM song bank in large POOL to keep the
 * 96 KiB low-RAM section within the FM1 linker memory map. */
static struct track_song_bank track_song __attribute__((section(".beetle_song"),aligned(64)));
static uint16_t track_live_punch_mask,track_replay_punch_mask;
/* REC is a TRACKER note-write switch, ON by default; Punch recording has
 * its own page-scoped switch. Both controls live outside the audio ISR. */
static uint8_t track_punch_rec_armed;
/* Copy/paste song rows; not serialized and does not consume pattern clipboard. */
static struct beetle_song_rows_clip track_song_rows_clip __attribute__((section(".beetle_edit_scratch"),aligned(4)));
static uint8_t track_song_range_on,track_song_range_anchor,track_song_range_end;
static uint8_t track_song_sel_was_range;
/* Last stable battery sample. Sampled infrequently in the main loop only. */
static volatile uint8_t track_battery_low20;
static volatile uint8_t track_battery_ui_dirty;
static uint16_t track_battery_adc_filtered;
static uint8_t track_battery_valid;

/* REC only saves fresh physical down edges after PLAY. Orphan OFF events are ignored.
 * Preserve the source clip even when Song Chains advances during a hold. */
static uint16_t track_recorded_punch_mask;
static uint8_t track_recorded_punch_source[16];
static uint16_t track_recorded_punch_edited[TRACK_PATTERNS];
/* Saved mixer-style Punch preset is separate from temporary playback automation. */
static uint8_t track_punch_base_param[13];
/* PUNCH 2 now Ring Mod: persisted wet intensity 0..100, default 20. */
static uint8_t track_sel_tracker_ready; /* only a fresh tracker-only SEL rising edge can delete */
static uint8_t track_punch_saved_value(uint8_t fx);
static void track_sync_punch_params(void);
static uint8_t track_punch_focus=255u;
static uint8_t track_punch_held_order[16],track_punch_held_count;
static uint8_t track_punch_page=0u; /* most recently depressed FX key */
static uint32_t track_seq_since;
static uint8_t track_seq_fired;
static struct track_lane_clipboard beetle_lane_clip;
/* Copy/paste sparse FX alongside lane notes. Saved IDs use row offsets and
 * NOTE2 bit; the destination pattern/lane are chosen when pasting. */
static struct track_extra_fx beetle_lane_fx_copy[TRACK_EXTRA_FX_CAP] __attribute__((section(".beetle_usb_stems_ram"),aligned(4)));
static uint8_t beetle_lane_fx_count;
static void beetle_lane_fx_capture(uint8_t lane,uint8_t first,uint8_t last){
 beetle_lane_fx_count=0u;
 for(uint8_t i=0u;i<track_song.extras.count;i++){
  const struct track_extra_fx *f=&track_song.extras.data[i];
  uint8_t src_lane=(uint8_t)((f->id&127u)/TRACK_STEPS),row=(uint8_t)((f->id&127u)%TRACK_STEPS);
  if(f->pattern!=track_state.pattern_slot||src_lane!=lane||row<first||row>last)continue;
  if(beetle_lane_fx_count>=TRACK_EXTRA_FX_CAP)break;
  struct track_extra_fx item=*f;
  item.pattern=0u;item.id=(uint8_t)(row-first+(f->id&128u));
  beetle_lane_fx_copy[beetle_lane_fx_count++]=item;
 }
}
/* Preflight count before modifying note cells: reject a too-large paste atomically. */
static uint8_t beetle_lane_fx_can_paste(uint8_t lane,uint8_t first,uint8_t count){
 uint8_t old=0u,needed=0u;
 for(uint8_t i=0u;i<track_song.extras.count;i++){
  const struct track_extra_fx *f=&track_song.extras.data[i];
  uint8_t src_lane=(uint8_t)((f->id&127u)/TRACK_STEPS),row=(uint8_t)((f->id&127u)%TRACK_STEPS);
  if(f->pattern==track_state.pattern_slot&&src_lane==lane&&row>=first&&row<first+count)old++;
 }
 for(uint8_t i=0u;i<beetle_lane_fx_count;i++)
  if((beetle_lane_fx_copy[i].id&127u)<count)needed++;
 return (uint8_t)(track_song.extras.count-old+needed<=TRACK_EXTRA_FX_CAP);
}
static void beetle_lane_fx_paste(uint8_t lane,uint8_t first,uint8_t count){
 for(uint8_t row=first;row<first+count;row++){
  track_extra_clear(&track_song,track_state.pattern_slot,track_extra_key(lane,row,0u));
  if(lane==TRACK_DRUM_CHANNEL)track_extra_clear(&track_song,track_state.pattern_slot,track_extra_key(lane,row,1u));
 }
 for(uint8_t i=0u;i<beetle_lane_fx_count;i++){
  const struct track_extra_fx *f=&beetle_lane_fx_copy[i];
  uint8_t offset=(uint8_t)(f->id&127u);if(offset>=count)continue;
  uint8_t note2=(uint8_t)((f->id&128u)!=0u);
  uint8_t key=track_extra_key(lane,(uint8_t)(first+offset),note2);
  uint8_t slot=track_extra_slot(f);
  (void)track_extra_add(&track_song,track_state.pattern_slot,key,slot,track_extra_effect(f),f->value);
 }
}

static uint8_t beetle_clear_lane_yes=0u;
static uint8_t beetle_range_on,beetle_range_anchor,beetle_range_end;
static uint8_t beetle_sel_defer,beetle_sel_was_range,beetle_sel_delete_range;
/* Large history records live in pooled RAM, never the ISR stack. */
static struct beetle_pattern_editor beetle_ed __attribute__((section(".beetle_usb_stems_ram"),aligned(16)));
/* Save POOL for the audio ISR by using the remaining small non-ISR RAM for
 * temporary clipboard/scratch. Distinct arrays, never alias playing voices. */
static struct beetle_pattern_image beetle_edit_pending __attribute__((section(".beetle_usb_stems_ram"),aligned(16)));
static struct beetle_pattern_image beetle_edit_whole __attribute__((section(".beetle_usb_stems_ram"),aligned(16)));
static struct track_step beetle_edit_block[TRACK_STEPS] __attribute__((section(".beetle_usb_stems_ram"),aligned(16)));
/* Encoder audition: preview auto-releases; never writes a pattern. */
static uint8_t track_scroll_preview_active,track_scroll_preview_channel,track_scroll_preview_note;
static uint32_t track_scroll_preview_until;
static uint32_t track_old_note_mask;
static struct track_name_editor track_editor;
static char track_title[TRACK_NAME_MAX+1u];
static uint8_t track_named,track_page,track_menu_index,track_fx_cursor,track_osc_cursor,track_osc_bank;
static struct beetle_settings_nav track_settings_nav; /* FM1 physical SEL press/release transaction */
static uint8_t track_fx_slot; /* FX1/FX2/FX3, chosen in PER STEP FX */
static uint32_t track_fx_full_until; /* visible full-capacity overlay, no destructive writes */
/* UI-only notification: never retries writes or modifies NOR when full. */
static uint32_t track_storage_full_until;
static uint8_t track_storage_full_reason; /* 1=no project slot; 2=record/flash capacity */
static void track_show_storage_full(uint8_t reason){
 track_storage_full_reason=reason;
 track_storage_full_until=etch_ms+3200u;
}
/* Reverse scale search is transient UI state. No new project fields/schema. */
static struct beetle_reverse_report track_reverse_report __attribute__((section(".pool"),aligned(4)));
static uint8_t track_reverse_index;
static void track_reverse_open(void){
 beetle_reverse_analyse(&track_song,&track_state,&track_reverse_report);
 track_reverse_index=0u;track_page=32u;
}
/* V0.81: boot-only chooser, save origin, and non-destructive first-free allocation. */
static uint8_t track_boot_menu_choice,track_save_return_page,track_new_project_full,track_load_from_boot;
/* The original pitch belongs to the currently focused cell, not a global
 * pitch borrowed from another step. Zero means no restorable note. */
static uint8_t track_preset_focus_pattern,track_preset_focus_channel,track_preset_focus_step;
static uint8_t track_preset_focus_column,track_preset_focus_original,track_preset_focus_valid;
static uint8_t track_rc_tab; /* V0.73: read-only RC AUDIO/MIDI diagnostics */
static uint8_t track_help_offset; /* Chapter-local scroll, never saved in project. */
static uint8_t track_help_chapter;
static struct beetle_load_input_gate track_load_input_guard; /* legacy SEL-delete fence */
static struct beetle_io_input_gate track_io_input_guard; /* ALL controls during and after disk I/O */
static uint8_t track_audio_started,track_prev_notes[TRACK_CHANNELS][5];
/* Called only in main, with IRQs held, after a hardware audio rearm. */
static void beetle_audio_resume_after_fault(void){
 track_chip_silence(&track_chip);
 track_chip_set_realtime_budget(&track_chip,TRACK_CHIP_VOICES);
 track_drum_stop(&track_drum);
 track_audio_started=0u;
}

static uint8_t track_lane_instrument[TRACK_SYNTH_CHANNELS];
/* V0.83: EDIT focuses a patch for FUTURE notes, independently of the
 * instrument already stored in any existing pattern/song step. */
static uint8_t track_input_instrument[TRACK_CHANNELS];
static uint8_t track_input_explicit[TRACK_CHANNELS]; /* default lane associations are not reserved patches */
static uint8_t track_edit_patch_full; /* legacy display flag, normal edits never allocate slots */

static uint8_t track_prev_note_count[TRACK_CHANNELS];
static uint32_t track_audio_clock,track_last_poll,track_last_paint,track_home_since;
/* Main-thread-only LCD snapshot: never hold IRQs across an SPI transfer. */
static uint8_t track_ui_step_override __attribute__((section(".pool")));
static uint8_t track_ui_step_snapshot __attribute__((section(".pool")));
/* Latch song position alongside play_step across the WHOLE LCD transfer.
 * Audio ISR may advance while the SPI display is still transferring. */
static uint8_t track_ui_song_pos_snapshot __attribute__((section(".pool")));
/* Sample-exact gates; no allocations, no callback from the UI task. */
static uint32_t track_gate_remaining[TRACK_SYNTH_CHANNELS];
static uint8_t track_groove_focus; /* 0 micro, 1 ratchets, 2 probability */
static struct beetle_groove_clock track_groove_clock;
static struct beetle_mute_queue track_mute_queue;
static struct beetle_live_switch track_live_mutes,track_live_solos;
static uint8_t track_live_q_mute=1u,track_live_q_solo=1u;
static void beetle_live_reset(void){beetle_live_clear(&track_live_mutes);beetle_live_clear(&track_live_solos);track_live_q_mute=track_live_q_solo=1u;}
static struct beetle_performance_snapshots track_snapshots;
static uint8_t track_snapshot_slot;
static struct beetle_mute_groups track_groups;
static uint8_t track_group_slot,track_group_lane;
static struct beetle_groove_clip track_groove_clip; /* UI clipboard, not project data */
static uint32_t track_drum_note1_serial, track_drum_note2_serial;
static uint8_t track_preset_last_note2=1u; /* CH6 NOTE2 encoded 1..14 */
static uint8_t track_dyn_focus; /* velocity=0, gate=1 */
static uint8_t track_swing_pending_mask;
static uint32_t track_swing_pending_at[TRACK_CHANNELS];
static uint32_t track_boot_input_unlock_ms,track_last_encoder_ms;
static uint8_t track_last_encoder_id;
static int32_t track_last_encoder_delta;
static uint32_t track_recovered_state_faults;
static uint8_t track_home_fired;
static uint8_t track_song_preplay_row,track_song_follow_active;
/* V0.86: live song mixer is a transient performance screen, not a new saved preference. */
/* V0.13: REC hold opens the current-pattern punch event editor. */
static uint32_t track_rec_since;
static uint8_t track_rec_long,track_punch_edit_row,track_punch_edit_index;
static uint8_t track_fx_drum_lane,track_edit_poly;
static int16_t track_type_queued;
/* Global live keyboard octave is native_controls.octave. Shared by EDIT and
 * TRACKER, never written into project notes unless a note is entered.
 * Using chord-aware key-up handling prevents OCT-+OCT+ from shifting pitch. */
static struct etch_octave_chord track_key_oct_chord;
/* Latches SEQ+OCT chord at button-down; consumes key release separately
 * from the global keyboard-octave control. */
static uint8_t track_step_oct_held;
static uint8_t track_preset_last_note=60u;
static uint32_t track_boot_splash_until;
static uint8_t track_slot_seen[TRACK_PROJECTS],track_overwrite_wait;
static int track_storage_result=TRACK_JOURNAL_HARDWARE_LOCKED;
static void track_library_capture_active(uint8_t id,struct track_store_instrument *v);
static void track_library_apply_active(uint8_t id,const struct track_store_instrument *v);
/* V1.3.2: instrument numbers are FIXED shared patch identities, not a
 * copy-on-write allocator. Editing INST 3 MUST always edit patch 3, including
 * notes already referring to it in another pattern or Song chain. */
static uint8_t track_edit_prepare_patch(uint8_t channel){
 if(channel>=TRACK_SYNTH_CHANNELS)return 0u;
 if(track_input_instrument[channel]>=TRACK_CHIP_SLOTS)
  track_input_instrument[channel]=(uint8_t)(channel%TRACK_CHIP_SLOTS);
 track_input_explicit[channel]=1u;
 track_edit_patch_full=0u;
 return 1u;
}
/* Focus the numbered patch belonging to an existing saved note. When the
 * focused step is empty/STP, retain the user's explicit channel patch choice.
 * Never implicitly rewrite the saved note's instrument reference. */
static void track_edit_focus_step(uint8_t channel){
 if(channel>=TRACK_SYNTH_CHANNELS)return;
 const struct track_step *st=&track_state.active.ch[channel][track_state.transport.edit_step];
 if(st->note<TRACK_NOTE_STOP && st->instrument<TRACK_CHIP_SLOTS){
  track_input_instrument[channel]=st->instrument;
  track_input_explicit[channel]=1u;
 }
}
static uint32_t track_storage_confirm_since;
/* RC1.0.5 full-project-volume reset: default answer NO on both screens. */
static uint8_t track_wipe_yes;
static int track_wipe_result;

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
static uint8_t track_preview_channel[27] __attribute__((section(".usb_audio_ram")));
static uint8_t track_preview_note[27] __attribute__((section(".usb_audio_ram")));
static uint32_t track_edit_prev_keys;
static uint8_t track_edit_voice_key[27],track_edit_voice_note[27];
static struct beetle_mono_audition track_mono_keys __attribute__((section(".pool")));
static uint32_t track_edit_mono_birth;
static uint8_t track_edit_mono_pitch,track_edit_mono_channel;
/* Four shared instrument patches for the five synth lanes.
 * Native RAM-only instrument editor parameters. These follow the WebSimulator
 * 8-row UI. TYPE/DUTY/CUTOFF and channel level are implemented in DSP/mixer;
 * ADSR time/value parameters remain visible/editable but their DSP wiring
 * is tracked separately (not claimed functional on FM1 in this release). */
struct track_instrument_ui_values {
 uint8_t attack,decay,sustain,release,duty,volume;
 uint16_t cutoff;
};
static struct track_instrument_ui_values track_ui_inst[TRACK_CHIP_SLOTS] __attribute__((section(".pool"),aligned(4)));
/* V1.3.2: patch parameters must follow the INST referenced by the note,
 * not the lane index. AUDITION and sequencer paths share this mapping.
 * Default patch volume 65 is unity, retaining older project loudness. */
static uint8_t track_patch_volume_velocity(uint8_t inst,uint8_t velocity){
 if(inst>=TRACK_CHIP_SLOTS)return velocity;
 uint32_t scaled=((uint32_t)velocity*track_ui_inst[inst].volume+32u)/65u;
 return (uint8_t)(scaled>127u?127u:scaled);
}
static void track_patch_live_channel(uint8_t channel,uint8_t inst){
 if(channel>=TRACK_SYNTH_CHANNELS||inst>=TRACK_CHIP_SLOTS)return;
 const struct track_instrument_ui_values *p=&track_ui_inst[inst];
 track_chip.duty[channel]=(uint8_t)(p->duty<19u?0u:p->duty<38u?1u:p->duty<63u?2u:3u);
 track_chip.cutoff[channel]=(uint8_t)(p->cutoff/75u>250u?250u:p->cutoff/75u);
}

static struct beetle_instrument_library track_library __attribute__((section(".beetle_song"),aligned(4)));
static uint8_t track_library_slot,track_library_action,track_library_target,track_library_copy_source;
static struct track_store_instrument track_type_presets[TRACK_CHIP_SLOTS][TRACK_CHIP_TYPES]
 __attribute__((section(".usb_audio_ram"),aligned(4)));
static uint16_t track_type_valid[TRACK_CHIP_SLOTS];
static uint8_t track_library_notice; /* 1 STORED, 2 LOADED, 3 COPIED, 4 EMPTY, 5 RENAMED */
static struct track_name_editor track_library_name_editor __attribute__((section(".beetle_song"),aligned(4)));
/* Capture and apply one complete instrument patch. Does not alter other presets. */
static void track_library_capture_active(uint8_t id,struct track_store_instrument *v){
 id&=(TRACK_CHIP_SLOTS-1u);v->type=track_chip.osc_type[id];v->attack=track_ui_inst[id].attack;
 v->decay=track_ui_inst[id].decay;v->sustain=track_ui_inst[id].sustain;
 v->release=track_ui_inst[id].release;v->duty=track_ui_inst[id].duty;
 v->volume=track_ui_inst[id].volume;v->cutoff=track_ui_inst[id].cutoff;
 v->va=track_chip.patches[id];
}
static void track_library_apply_active(uint8_t id,const struct track_store_instrument *v){
 id&=(TRACK_CHIP_SLOTS-1u);track_chip.osc_type[id]=v->type;track_chip.patches[id]=v->va;
 track_ui_inst[id].attack=v->attack;track_ui_inst[id].decay=v->decay;
 track_ui_inst[id].sustain=v->sustain;track_ui_inst[id].release=v->release;
 track_ui_inst[id].duty=v->duty;track_ui_inst[id].volume=v->volume;track_ui_inst[id].cutoff=v->cutoff;
 track_chip.env_attack[id]=v->attack;track_chip.env_decay[id]=v->decay;
 track_chip.env_sustain[id]=v->sustain;track_chip.env_release[id]=v->release;
 /* Duty and cutoff are channel arrays, not patch arrays. Playback
  * copies the active patch values onto the appropriate lane. */
 if(id<TRACK_SYNTH_CHANNELS){track_chip.duty[id]=(uint8_t)(v->duty<19u?0u:v->duty<38u?1u:v->duty<63u?2u:3u);
 track_chip.cutoff[id]=(uint8_t)(v->cutoff/75u>250u?250u:v->cutoff/75u);}
 track_chip_refresh_instrument(&track_chip,id);
}

static void track_type_clone(uint8_t to,uint8_t from){
 track_type_valid[to]=track_type_valid[from];
 for(uint8_t t=0u;t<TRACK_CHIP_TYPES;t++)if(track_type_valid[from]&(1u<<t))
  track_type_presets[to][t]=track_type_presets[from][t];
}
/* Preserve every waveform TYPE independently; changes to TYPE never
 * inherit destructive ADSR edits from the previously selected TYPE. */
static void track_type_capture(uint8_t id){
 struct track_store_instrument v;
 id&=(TRACK_CHIP_SLOTS-1u);track_library_capture_active(id,&v);
 if(v.type<TRACK_CHIP_TYPES){
  track_type_presets[id][v.type]=v;
  track_type_valid[id]|=(uint16_t)(1u<<v.type);
 }
}
static void track_type_switch(uint8_t id,uint8_t next){
 struct track_store_instrument v;
 id&=(TRACK_CHIP_SLOTS-1u);next=(uint8_t)(next%TRACK_CHIP_TYPES);
 track_type_capture(id);
 if(track_type_valid[id]&(1u<<next)){
  track_library_apply_active(id,&track_type_presets[id][next]);
 }else{
  track_library_capture_active(id,&v);
  v.type=next;
  /* A freshly encountered waveform starts with factory ADSR and controls. */
  v.attack=1u;v.decay=14u;v.sustain=100u;v.release=16u;
  v.duty=50u;v.cutoff=7000u;v.volume=65u;
  track_library_apply_active(id,&v);
  track_type_capture(id);
 }
}


/* V0.70: heterogeneous, chronological Undo/Redo using V0.56 pattern snapshot
 * allocations. Transient RAM only; history never enters a TRK1 project. */
struct beetle_instrument_undo_state {struct track_store_instrument patch;uint8_t lane_volume;};
struct beetle_library_undo_state {struct track_store_instrument patch;char name[BEETLE_LIBRARY_NAME];uint8_t valid;};
#define BEETLE_SONG_EDIT_BYTES (sizeof(track_song.chain)+7u)
static void track_history_library_capture(uint8_t slot,struct beetle_library_undo_state *state){
 state->patch=track_library.patch[slot];
 for(uint8_t i=0u;i<BEETLE_LIBRARY_NAME;i++)state->name[i]=track_library.name[slot][i];
 state->valid=(uint8_t)((track_library.valid>>slot)&1u);
}
static void track_history_library_apply(uint8_t slot,const struct beetle_library_undo_state *state){
 track_library.patch[slot]=state->patch;
 for(uint8_t i=0u;i<BEETLE_LIBRARY_NAME;i++)track_library.name[slot][i]=state->name[i];
 if(state->valid)track_library.valid|=(uint8_t)(1u<<slot);
 else track_library.valid&=(uint8_t)~(1u<<slot);
}
static void track_history_instrument_capture(uint8_t id,uint8_t ch,struct beetle_instrument_undo_state *state){
 track_library_capture_active(id,&state->patch);state->lane_volume=track_mix.ch[ch].volume;
}
static void track_history_instrument_apply(uint8_t id,uint8_t ch,const struct beetle_instrument_undo_state *state){
 track_library_apply_active(id,&state->patch);track_mix.ch[ch].volume=state->lane_volume;
}
static void track_history_begin_instrument(uint8_t id,uint8_t ch){
 if(track_state.transport.playing)return;
 struct beetle_instrument_undo_state current;track_history_instrument_capture(id,ch,&current);
 (void)beetle_edit_begin_bytes(&beetle_ed,BEETLE_EDIT_INSTRUMENT,
   (uint8_t)((id&7u)|((ch&7u)<<3)),&current,sizeof(current));
}
static void track_history_commit_instrument(uint8_t id,uint8_t ch){
 if(track_state.transport.playing)return;
 struct beetle_instrument_undo_state current;track_history_instrument_capture(id,ch,&current);
 (void)beetle_edit_commit_bytes(&beetle_ed,BEETLE_EDIT_INSTRUMENT,
   (uint8_t)((id&7u)|((ch&7u)<<3)),&current,sizeof(current));
}
static void track_history_begin_library(uint8_t slot){
 if(track_state.transport.playing||slot>=BEETLE_LIBRARY_SLOTS)return;
 struct beetle_library_undo_state current;track_history_library_capture(slot,&current);
 (void)beetle_edit_begin_bytes(&beetle_ed,BEETLE_EDIT_LIBRARY,slot,&current,sizeof(current));
}
static void track_history_commit_library(uint8_t slot){
 if(track_state.transport.playing||slot>=BEETLE_LIBRARY_SLOTS)return;
 struct beetle_library_undo_state current;track_history_library_capture(slot,&current);
 (void)beetle_edit_commit_bytes(&beetle_ed,BEETLE_EDIT_LIBRARY,slot,&current,sizeof(current));
}
static uint8_t track_history_step(uint8_t redo){
 if(track_state.transport.playing||beetle_ed.pending_valid)return 0u;
 if(redo?beetle_ed.cursor>=beetle_ed.count:beetle_ed.cursor==0u)return 0u;
 const uint8_t pos=(uint8_t)(redo?beetle_ed.cursor:beetle_ed.cursor-1u);
 struct beetle_history_entry *entry=&beetle_ed.history[pos];
 if(entry->kind==BEETLE_EDIT_PATTERN){
  return (uint8_t)(redo?beetle_edit_redo(&beetle_ed,&track_song,&track_state):
     beetle_edit_undo(&beetle_ed,&track_song,&track_state));
 }
 uint8_t ok=0u;
 if(entry->kind==BEETLE_EDIT_MIXER){
  beetle_mute_cancel(&track_mute_queue);
  ok=beetle_edit_swap_bytes(&beetle_ed,pos,&track_mix,sizeof(track_mix));
 }else if(entry->kind==BEETLE_EDIT_SONG){
  ok=beetle_edit_swap_bytes(&beetle_ed,pos,&track_song.chain[0],BEETLE_SONG_EDIT_BYTES);
  if(track_song.count && track_song.edit_row>=track_song.count)track_song.edit_row=(uint8_t)(track_song.count-1u);
 }else if(entry->kind==BEETLE_EDIT_MASTER){
  ok=beetle_edit_swap_bytes(&beetle_ed,pos,&track_master_cfg,sizeof(track_master_cfg));
 }else if(entry->kind==BEETLE_EDIT_INSTRUMENT){
  uint8_t id=(uint8_t)(entry->pattern_index&7u),ch=(uint8_t)((entry->pattern_index>>3u)&7u);
  if(ch<TRACK_CHANNELS){
   struct beetle_instrument_undo_state current;track_history_instrument_capture(id,ch,&current);
   ok=beetle_edit_swap_bytes(&beetle_ed,pos,&current,sizeof(current));
   if(ok)track_history_instrument_apply(id,ch,&current);
  }
 }else if(entry->kind==BEETLE_EDIT_LIBRARY && entry->pattern_index<BEETLE_LIBRARY_SLOTS){
  struct beetle_library_undo_state current;
  track_history_library_capture(entry->pattern_index,&current);
  ok=beetle_edit_swap_bytes(&beetle_ed,pos,&current,sizeof(current));
  if(ok)track_history_library_apply(entry->pattern_index,&current);
 }
 if(ok){beetle_ed.cursor=(uint8_t)(redo?pos+1u:pos);track_state.dirty=1u;}
 return ok;
}

#if BEETLE_NATIVE_STORAGE
/* Scratch and project staging MUST NOT be placed on the tiny firmware stack.
 * This memory is never touched from an audio ISR and is reused for loads. */
static struct track_journal_status beetle_last_journal;
static void beetle_project_capture(uint8_t slot){
 track_song_capture(&track_song,&track_state);
 /* The live engine is bound to stable metadata, not reused flash scratch. */
 beetle_staged.oneshots=beetle_live_oneshots;
 beetle_staged.song=track_song;beetle_staged.mixer=track_mix;beetle_staged.snapshots=track_snapshots;beetle_staged.groups=track_groups;beetle_staged.library=track_library;
 for(uint8_t i=0u;i<TRACK_DRUM_COUNT;i++)beetle_staged.drum[i]=track_drum.components[i];
 beetle_staged.master=track_master_cfg;
 for(uint8_t i=0u;i<13u;i++)beetle_staged.punch_param[i]=track_punch_base_param[i];
 for(uint8_t i=0u;i<TRACK_CHIP_SLOTS;i++){track_type_capture(i);beetle_staged.type_valid[i]=track_type_valid[i];
  for(uint8_t t=0u;t<TRACK_CHIP_TYPES;t++)if(track_type_valid[i]&(1u<<t))
   beetle_staged.type_presets[i][t]=track_type_presets[i][t];}
 beetle_staged.harmony=track_state.harmony;beetle_staged.bpm=track_state.transport.bpm;
 beetle_staged.slot=slot;
 for(uint8_t i=0u;i<12u;i++)beetle_staged.title[i]=track_title[i]?track_title[i]:' ';
 beetle_staged.title[12]=0;
 for(uint8_t i=0u;i<TRACK_CHIP_SLOTS;i++){
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
 track_song=beetle_staged.song;track_mix=beetle_staged.mixer;track_snapshots=beetle_staged.snapshots;track_snapshot_slot=0u;track_groups=beetle_staged.groups;track_group_slot=track_group_lane=0u;track_library=beetle_staged.library;track_library_slot=track_library_action=track_library_target=track_library_notice=0u;
 for(uint8_t i=0u;i<TRACK_DRUM_COUNT;i++)track_drum.components[i]=beetle_staged.drum[i];
 beetle_live_oneshots=beetle_staged.oneshots;
 track_drum_bind_oneshots(&track_drum,&beetle_live_oneshots);
 track_master_cfg=beetle_staged.master;
 for(uint8_t i=0u;i<TRACK_CHIP_SLOTS;i++){
  track_type_valid[i]=beetle_staged.type_valid[i];
  for(uint8_t t=0u;t<TRACK_CHIP_TYPES;t++)if(track_type_valid[i]&(1u<<t))
    track_type_presets[i][t]=beetle_staged.type_presets[i][t];
 }
 for(uint8_t i=0u;i<13u;i++)track_punch_base_param[i]=beetle_staged.punch_param[i];
 /* Stored Ring Mod intensity is restored without transient overrides. */
 track_live_punch_mask=track_replay_punch_mask=0u;beetle_live_reset();track_sync_punch_params();
 track_state.harmony=beetle_staged.harmony;
 track_state.transport.bpm=beetle_staged.bpm;
 track_state.transport.playing=0u;track_state.transport.play_step=0u;
 track_state.pattern_slot=0u;track_state.active=track_song.patterns[0];
 track_state.project_slot=beetle_staged.slot;
 track_chip_silence(&track_chip);track_drum_stop(&track_drum);
 for(uint8_t c=0;c<TRACK_SYNTH_CHANNELS;c++){track_lane_instrument[c]=c;track_input_instrument[c]=c;}
 track_input_instrument[TRACK_DRUM_CHANNEL]=0u;for(uint8_t ch=0u;ch<TRACK_CHANNELS;ch++)track_input_explicit[ch]=0u;track_edit_patch_full=0u;
 for(uint8_t i=0u;i<TRACK_CHIP_SLOTS;i++){
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
  if(i<TRACK_SYNTH_CHANNELS){track_chip.duty[i]=(uint8_t)(v->duty<19u?0u:v->duty<38u?1u:v->duty<63u?2u:3u);
  track_chip.cutoff[i]=(uint8_t)(v->cutoff/75u>250u?250u:v->cutoff/75u);}
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
 track_drum.step_gain=100u;track_drum_note1_serial=track_drum_note2_serial=0u;beetle_groove_reset(&track_groove_clock);beetle_mute_cancel(&track_mute_queue);track_groove_clip.valid=0u;
 for(uint8_t i=0u;i<TRACK_NAME_MAX;i++)track_title[i]=beetle_staged.title[i];
 track_title[TRACK_NAME_MAX]=0;track_named=1u;track_slot_seen[track_state.project_slot]=1u;track_session_saved_slot=track_state.project_slot;
 track_state.dirty=0u;track_groove_clip.valid=0u;
}
#if defined(BEETLE_NATIVE_V2) && BEETLE_NATIVE_V2
#include "beetle_storage_v2_reader.h"
#include "beetle_storage_v2_writer.h"
#include "beetle_project_pack_v2.h"
/* V2 is ACTIVATED ONLY if the current 288KiB volume already contains a
 * CRC-verified B2SB index. No automatic legacy partition conversion.
 * All large work buffers borrow the existing 36KiB project stage area,
 * AFTER the staged state has been serialized. No audio RAM is allocated. */
_Static_assert(sizeof(struct track_store_project)>=25600u,"V2 scratch workspace needs 25KiB");
static uint8_t *beetle_v2_workspace(void){return (uint8_t *)(void *)&beetle_staged;}
static int beetle_v2_mode(void){
 int owner=beetle_hal_probe_ownership_ui();
 if(owner!=BEETLE_CLAIM_EXISTING)return owner==BEETLE_CLAIM_BLANK?0:-1;
 int is_v2=beetle_hal_v2_marker_prefix();
 if(is_v2<0)return -1;
 if(!is_v2)return 0; /* only legacy TRKJ is accepted under legacy marker */
 struct beetle_v2_device d;
 if(beetle_v2_open(&d,beetle_v2_hal_read,0,beetle_codec_buffer))return -1;
 struct beetle_v2_domains dom;
 return beetle_v2_audit_domains(&d,&dom,beetle_codec_buffer)==0?1:-1;
}
static int beetle_v2_load_staged(uint8_t slot){
 uint8_t *work=beetle_v2_workspace();
 struct beetle_v2_device d;
 if(beetle_v2_open(&d,beetle_v2_hal_read,0,work))return TRACK_JOURNAL_FOREIGN;
 uint32_t decoded=0u;
 int rc=beetle_v2_project_read(&d,slot,beetle_codec_buffer,sizeof(beetle_codec_buffer),
  &decoded,work,work+8192u,12272u);
 if(rc==0)return TRACK_JOURNAL_NO_PROJECT;
 /* V2 index / decompression used the stage bytes as scratch. Restore the
  * pointer fields before the V24 decoder touches the one-shot PCM arrays. */
 beetle_staged.oneshots.first=beetle_oneshots_first;
 beetle_staged.oneshots.second=beetle_oneshots_second;
 if(rc!=1||!decoded||track_store_decode(&beetle_staged,beetle_codec_buffer,decoded))
  return TRACK_JOURNAL_INVALID;
 beetle_last_journal.payload_bytes=decoded;
 beetle_last_journal.generation=d.generation;
 return TRACK_JOURNAL_OK;
}
static int beetle_v2_save_staged(void){
 uint8_t slot=beetle_staged.slot; /* preserve before overwriting staging scratch */
 uint8_t *work=beetle_v2_workspace();uint32_t encoded=0u;
 if(track_store_encode(&beetle_staged,beetle_codec_buffer,
     sizeof(beetle_codec_buffer),&encoded))return TRACK_JOURNAL_CAPACITY;
 /* Project stage may be discarded after serialization; audio engine has
  * already captured musical data separately and keeps playback paused. */
 size_t packed=0;
 if(beetle_pack_project_v2(beetle_codec_buffer,encoded,work+12288u,12272u,
     &packed,(uint16_t *)(void *)(work+8192u)))return TRACK_JOURNAL_CAPACITY;
 struct beetle_v2_writer w;
 if(beetle_v2_writer_open(&w,beetle_v2_hal_read,beetle_v2_hal_program,
  beetle_v2_hal_erase,0,work,work+4096u))return TRACK_JOURNAL_FOREIGN;
 beetle_write_permit=1u;
 int rc=beetle_v2_write_project(&w,slot,
    work+12288u,(uint32_t)packed,work,work+4096u,work+24576u);
 beetle_write_permit=0u;
 return rc==0?TRACK_JOURNAL_OK:rc==-2?TRACK_JOURNAL_CAPACITY:TRACK_JOURNAL_IO;
}
#endif
static int beetle_project_load_staged(uint8_t slot){
#if defined(BEETLE_NATIVE_V2) && BEETLE_NATIVE_V2
 int mode=beetle_v2_mode();
 if(mode<0)return TRACK_JOURNAL_FOREIGN;
 if(mode==1)return beetle_v2_load_staged(slot);
#endif
 beetle_staged.oneshots.first=beetle_oneshots_first;
 beetle_staged.oneshots.second=beetle_oneshots_second;
 return track_journal_load(&beetle_hal_journal_io,slot,&beetle_staged,
     beetle_codec_buffer,sizeof(beetle_codec_buffer),&beetle_last_journal);
}
static int beetle_project_preview_slot_internal(uint8_t slot,char title[13]){
#if defined(BEETLE_NATIVE_V2) && BEETLE_NATIVE_V2
 int mode=beetle_v2_mode();
 if(mode<0)return TRACK_JOURNAL_FOREIGN;
 if(mode==1){
  uint8_t *work=beetle_v2_workspace();struct beetle_v2_device d;uint32_t n=0;
  if(beetle_v2_open(&d,beetle_v2_hal_read,0,work))return TRACK_JOURNAL_FOREIGN;
  int rc=beetle_v2_project_read(&d,slot,beetle_codec_buffer,
    sizeof(beetle_codec_buffer),&n,work,work+8192u,12272u);
  if(rc==0)return TRACK_JOURNAL_NO_PROJECT;
  if(rc!=1||n<36u)return TRACK_JOURNAL_INVALID;
  memcpy(title,beetle_codec_buffer+24u,12u);title[12]=0;
  return TRACK_JOURNAL_OK;
 }
#endif
 return track_journal_preview(&beetle_hal_journal_io,slot,title);
}
static int beetle_project_preview_slot(uint8_t slot,char title[13]){
#if defined(BEETLE_NATIVE_V2) && BEETLE_NATIVE_V2
 /* Project browser must never overwrite a live sample's work buffer. */
 if(track_state.transport.playing)return TRACK_JOURNAL_HARDWARE_LOCKED;
 fm1_irq_off();track_drum_stop(&track_drum);fm1_irq_enable_all();
#endif
 int result=beetle_project_preview_slot_internal(slot,title);
#if defined(BEETLE_NATIVE_V2) && BEETLE_NATIVE_V2
 beetle_bd4_live_bind(beetle_v2_workspace(),sizeof(beetle_staged));
#endif
 return result;
}
static int beetle_project_delete_slot(uint8_t slot){
#if defined(BEETLE_NATIVE_V2) && BEETLE_NATIVE_V2
 int mode=beetle_v2_mode();
 if(mode<0)return TRACK_JOURNAL_FOREIGN;
 if(mode==1){
  uint8_t *work=beetle_v2_workspace();struct beetle_v2_writer w;
  if(beetle_v2_writer_open(&w,beetle_v2_hal_read,beetle_v2_hal_program,
   beetle_v2_hal_erase,0,work,work+4096u))return TRACK_JOURNAL_FOREIGN;
  beetle_write_permit=1u;
  int rc=beetle_v2_remove_project(&w,slot,work,work+4096u,work+24576u);
  beetle_write_permit=0u;
  return rc==0?TRACK_JOURNAL_OK:TRACK_JOURNAL_IO;
 }
#endif
 return track_journal_delete(&beetle_hal_journal_io,slot);
}
static int beetle_project_save_staged_internal(void){
#if defined(BEETLE_NATIVE_V2) && BEETLE_NATIVE_V2
 int mode=beetle_v2_mode();
 if(mode<0)return TRACK_JOURNAL_FOREIGN;
 if(mode==1)return beetle_v2_save_staged();
#endif
 /* HAL write permit is held only for this verified transaction. */
 beetle_write_permit=1u;
 int rc=track_journal_save(&beetle_hal_journal_io,beetle_staged.slot,
      &beetle_staged,beetle_codec_buffer,sizeof(beetle_codec_buffer),&beetle_last_journal);
 beetle_write_permit=0u;
 return rc;
}
static int beetle_project_save_staged(void){
#if defined(BEETLE_NATIVE_V2) && BEETLE_NATIVE_V2
 beetle_bd4_live_invalidate();
#endif
 int result=beetle_project_save_staged_internal();
#if defined(BEETLE_NATIVE_V2) && BEETLE_NATIVE_V2
 beetle_bd4_live_bind(beetle_v2_workspace(),sizeof(beetle_staged));
#endif
 return result;
}
static int beetle_project_load_slot(uint8_t slot){
 int owner=beetle_hal_probe_ownership_ui();
 if(owner!=BEETLE_CLAIM_EXISTING)return owner==BEETLE_CLAIM_FOREIGN?TRACK_JOURNAL_FOREIGN:TRACK_JOURNAL_NO_PROJECT;
 int rc=beetle_project_load_staged(slot);
 if(rc==TRACK_JOURNAL_OK){fm1_irq_off();beetle_project_apply();fm1_irq_enable_all();}
#if defined(BEETLE_NATIVE_V2) && BEETLE_NATIVE_V2
 beetle_bd4_live_bind(beetle_v2_workspace(),sizeof(beetle_staged));
#endif
 return rc;
}
#endif
/* Called under the audio IRQ lock (or in its audio callback). Replay takes
 * precedence only if the FX is not physically held; knob moves remain live.
 * Never overwrite user presets with recorded playback values. */
static void track_punch_record_reset(void){
 memset(track_recorded_punch_edited,0,sizeof(track_recorded_punch_edited));
 track_recorded_punch_mask=0u;
 for(uint8_t i=0u;i<16u;i++)track_recorded_punch_source[i]=TRACK_PATTERNS;
}
/* Called with IRQs off, on explicit STOP. Any captured down must have a
 * matching recorded up; a stopped take cannot become an indefinite loop. */
static void track_punch_record_finish(void){
 {
  for(uint8_t fx=0u;fx<16u;fx++)if(track_recorded_punch_mask&(1u<<fx)){
   uint8_t src=track_recorded_punch_source[fx];
   uint8_t next=(uint8_t)((track_state.transport.play_step+1u)%TRACK_STEPS);
   if(src<TRACK_PATTERNS && track_song_record_value(&track_song,src,next,fx,0u,255u)==0)
    track_state.dirty=1u;
  }
 }
 track_punch_record_reset();
}
/* When a held, recorded key crosses a Song Chain row boundary, end the old
 * source and explicitly start the new CH1 pattern. No ghost key from a
 * previous row can persist; non-recorded pre-PLAY keys are never inserted. */
static void track_punch_record_song_transition(void){
 if(!track_punch_rec_armed || !track_song.enabled)return;
 uint8_t source=track_song_punch_pattern(&track_song,&track_state);
 for(uint8_t fx=0u;fx<16u;fx++){
  uint16_t flag=(uint16_t)(1u<<fx);
  uint8_t old=track_recorded_punch_source[fx];
  if(old==source)continue;
  if(old<TRACK_PATTERNS && (track_recorded_punch_mask&flag)){
   if(track_song_record_value(&track_song,old,0u,fx,0u,255u)==0)track_state.dirty=1u;
  }
  track_recorded_punch_mask&=(uint16_t)~flag;
  track_recorded_punch_source[fx]=TRACK_PATTERNS;
  if(source<TRACK_PATTERNS && (track_live_punch_mask&flag)){
   if(!(track_recorded_punch_edited[source]&flag)){
    track_song_clear_punch_fx(&track_song,source,fx);
    track_recorded_punch_edited[source]|=flag;
   }
   if(track_song_record_value(&track_song,source,0u,fx,1u,
                track_punch_saved_value(fx))==0){
    track_recorded_punch_mask|=flag;
    track_recorded_punch_source[fx]=source;
    track_state.dirty=1u;
   }
  }
 }
}
static uint8_t track_punch_saved_value(uint8_t fx){
 return fx<5u?track_punch_base_param[fx]:fx>=8u&&fx<16u?track_punch_base_param[fx-3u]:255u;
}
static void track_sync_punch_params(void){
 for(uint8_t fx=0u;fx<16u;fx++){
  uint8_t value=track_punch_saved_value(fx);
  if(value==255u)continue;
  if(!(track_live_punch_mask&(1u<<fx)) && (track_replay_punch_mask&(1u<<fx)))
   value=track_song_effect_parameter(&track_song,track_song_punch_pattern(&track_song,&track_state),
           track_state.transport.play_step,fx,value);
  track_chip.punch_param[fx]=value;
 }
 track_chip.punch_focus=track_punch_focus;
}
static void track_ui_inst_reset(void){
 for(uint8_t i=0u;i<TRACK_CHIP_SLOTS;i++)track_type_valid[i]=0u;
 for(uint8_t i=0u;i<TRACK_CHIP_SLOTS;i++){
  track_ui_inst[i].attack=(uint8_t)(i==5u?18u:1u);
  track_ui_inst[i].decay=(uint8_t)(i==4u?25u:i==5u?32u:14u);
  track_ui_inst[i].sustain=(uint8_t)(i==4u?70u:i==5u?80u:100u);
  track_ui_inst[i].release=(uint8_t)(i==4u?18u:i==5u?48u:16u);
  track_ui_inst[i].duty=50u;track_ui_inst[i].cutoff=(uint16_t)(i==4u?3200u:i==5u?8400u:7000u);
  track_ui_inst[i].volume=65u;
  track_chip.env_attack[i]=track_ui_inst[i].attack;track_chip.env_decay[i]=track_ui_inst[i].decay;
  track_chip.env_sustain[i]=track_ui_inst[i].sustain;track_chip.env_release[i]=track_ui_inst[i].release;
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
 /* RC1.0.4: reverse-key search is page 32. Do not mistake it for corrupt state. */
 if(!beetle_ui_page_valid(track_page)){track_page=0u;faulty=1u;}
 /* Live Mixer is exclusively a Song Mode performance page. If a project
  * transition disables SONG, return before processing any mixer key. */
 if(!track_song.enabled){if(track_page==30u)track_page=9u;
  /* Audio must never observe stale live solo/mute overrides once SONG is off. */
  beetle_live_clear(&track_live_solos);beetle_live_clear(&track_live_mutes);}
 if(track_state.transport.edit_channel>=TRACK_CHANNELS){track_state.transport.edit_channel=0u;faulty=1u;}
 /* Hidden rows must never remain focusable after load/restore. Not a
  * storage fault: shortening a pattern legitimately leaves data in RAM. */
 if(track_state.transport.edit_step>=track_state.active.length)
  track_state.transport.edit_step=(uint8_t)(track_state.active.length-1u);
 if(track_state.transport.edit_column>=TRACK_NAV_COLUMNS){track_state.transport.edit_column=0u;faulty=1u;}
 if(track_state.pattern_slot>=TRACK_PATTERNS){track_state.pattern_slot=0u;faulty=1u;}
 if(track_song.count==0u||track_song.count>TRACK_SONG_ENTRIES){
  track_song.count=1u;track_song.chain[0].pattern=0u;track_song.chain[0].repeats=1u;for(uint8_t c=0u;c<TRACK_CHANNELS;c++)track_song.chain[0].lane[c]=0u;faulty=1u;
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
  st->note=track_note_scroll_keylocked(st->note,detents,track_state.transport.edit_channel==TRACK_DRUM_CHANNEL,&track_state.harmony);
  break;
 case 1:
  if(track_state.transport.edit_channel==TRACK_DRUM_CHANNEL){
   v=(int32_t)st->chord+detents;if(v<0)v=15;if(v>15)v=0;st->chord=(uint8_t)v;
  }else{v=(int32_t)st->instrument+detents;if(v<0)v=0;if(v>=TRACK_CHIP_SLOTS)v=TRACK_CHIP_SLOTS-1u;
   if(beetle_range_on)(void)track_set_instrument_range(&track_state,track_state.transport.edit_channel,
      beetle_range_anchor,beetle_range_end,(uint8_t)v);
   else st->instrument=(uint8_t)v;}
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
   st->effect=next;
   if(!next)track_extra_clear(&track_song,track_state.pattern_slot,track_extra_key(track_state.transport.edit_channel,track_state.transport.edit_step,0u));}break;
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
 track_init(&track_state);track_state.record_armed=1u;track_punch_rec_armed=0u;
 track_song_range_on=track_song_sel_was_range=0u;track_song_rows_clip.valid=0u;
 track_battery_low20=track_battery_valid=track_battery_ui_dirty=0u;track_battery_adc_filtered=0u;beetle_mute_cancel(&track_mute_queue);beetle_live_reset();beetle_snapshots_init(&track_snapshots);track_snapshot_slot=0u;beetle_groups_init(&track_groups);track_group_slot=track_group_lane=0u;track_mixer_init(&track_mix);track_drum_init(&track_drum);
 track_master_default(&track_master_cfg);track_drum_component=0u;track_drum_field=0u;
 track_master_focus=0u;track_master_chain_edit=0u;
 for(uint32_t z=0u;z<sizeof(track_master_dsp)/sizeof(uint32_t);z++)((uint32_t*)&track_master_dsp)[z]=0u;
 track_master_dsp_init(&track_master_dsp);track_punch_clear(&track_rec_events);
 track_song_init(&track_song,&track_state);track_chip_init(&track_chip);
 beetle_ed.pending=&beetle_edit_pending;beetle_ed.whole_clip=&beetle_edit_whole;
 beetle_ed.block=beetle_edit_block;beetle_edit_reset(&beetle_ed);
 for(uint8_t k=0u;k<13u;k++)track_punch_base_param[k]=track_chip.punch_param[k<5u?k:k+3u];
 track_chip_fx_reset();track_ui_inst_reset();beetle_library_init(&track_library);track_library_slot=track_library_action=track_library_target=track_library_copy_source=track_library_notice=0u;track_drum_init(&track_drum);
#if BEETLE_NATIVE_STORAGE
 memset(&beetle_staged.oneshots,0,sizeof(beetle_staged.oneshots));
 beetle_staged.oneshots.first=beetle_oneshots_first;
 beetle_staged.oneshots.second=beetle_oneshots_second;
 memset(beetle_oneshots_first,0,sizeof(beetle_oneshots_first));
 memset(beetle_oneshots_second,0,sizeof(beetle_oneshots_second));
 beetle_live_oneshots=beetle_staged.oneshots;
 track_drum_bind_oneshots(&track_drum,&beetle_live_oneshots);
#endif
 track_old_note_mask=0;track_page=29u;track_boot_menu_choice=0u;track_load_from_boot=0u;track_save_return_page=0u;track_new_project_full=0u;track_preset_focus_valid=0u;track_rc_tab=0u;track_named=0u;
 track_last_encoder_ms=0u;track_last_encoder_id=0u;track_last_encoder_delta=0;track_recovered_state_faults=0u;
 track_live_punch_mask=0u;track_replay_punch_mask=0u;track_punch_focus=255u;track_punch_held_count=0u;track_seq_since=0u;track_seq_fired=0u; /* lane clipboard persists after SEQ release */
 track_last_poll=0u;track_last_paint=0u;track_boot_input_unlock_ms=etch_ms+550u;track_home_since=0u;track_home_fired=0u;track_song_follow_active=0u;track_song_preplay_row=0u;
 track_scroll_preview_active=0u;track_scroll_preview_until=0u;
 track_audio_clock=0u;track_audio_started=0u;track_swing_pending_mask=0u;for(uint8_t ci=0;ci<TRACK_CHANNELS;ci++)track_swing_pending_at[ci]=0u;track_preview_mask=0u;
 for(uint8_t k=0u;k<27u;k++){track_preview_channel[k]=255u;track_preview_note[k]=255u;}
 track_type_queued=0;track_rec_since=0u;track_rec_long=0u;track_punch_edit_row=0u;track_punch_edit_index=0u;
 track_preset_last_note=60u;track_preset_last_note2=1u;beetle_groove_reset(&track_groove_clock);track_drum_note1_serial=track_drum_note2_serial=0u;track_boot_splash_until=etch_ms+1320u;
 track_fx_drum_lane=0u;track_fx_slot=0u;track_fx_full_until=0u;track_storage_full_until=0u;track_storage_full_reason=0u;track_edit_poly=0u;track_key_oct_chord.last=0u;track_key_oct_chord.combo_latched=0u;
 native_controls.octave=0;track_edit_prev_keys=0u;track_edit_mono_birth=0u;beetle_mono_reset(&track_mono_keys);track_overwrite_wait=0u;
 for(uint8_t i=0u;i<TRACK_PROJECTS;i++)track_slot_seen[i]=0u;
 track_storage_result=TRACK_JOURNAL_HARDWARE_LOCKED;track_storage_confirm_since=0u;track_unsaved_action=0u;track_session_saved_slot=255u;track_saved_toast_until=0u;track_storage_busy_kind=0u;beetle_io_input_begin(&track_io_input_guard,etch_ms);track_io_input_guard.active=0u;
 track_storage_progress_done=0u;track_storage_progress_total=0u;track_storage_just_initialized=0u;track_storage_phase=0u;
 track_load_scan=0u;track_load_feedback=TRACK_JOURNAL_NO_PROJECT;
 for(uint8_t i=0u;i<TRACK_PROJECTS;i++){
  track_load_status[i]=0u;
  for(uint8_t j=0u;j<=TRACK_NAME_MAX;j++)track_load_titles[i][j]=0;
 }
#if BEETLE_NATIVE_STORAGE
 /* Boot preflight only READS flash ownership. No auto-claim/erase/program. */
 beetle_storage_last_audit=beetle_hal_probe_ownership_ui();
 track_storage_result=beetle_storage_last_audit!=BEETLE_CLAIM_EXISTING && beetle_storage_last_audit>=BEETLE_CLAIM_FOREIGN?TRACK_JOURNAL_FOREIGN:
                      beetle_storage_last_audit<0?TRACK_JOURNAL_IO:TRACK_JOURNAL_NO_PROJECT;
#endif
 for(uint8_t k=0u;k<27u;k++){track_edit_voice_key[k]=255u;track_edit_voice_note[k]=255u;}
 for(uint8_t c=0;c<TRACK_CHANNELS;c++)track_prev_note_count[c]=0u;
 for(uint8_t c=0;c<TRACK_SYNTH_CHANNELS;c++){track_lane_instrument[c]=c;track_input_instrument[c]=c;}
 track_input_instrument[TRACK_DRUM_CHANNEL]=0u;for(uint8_t ch=0u;ch<TRACK_CHANNELS;ch++)track_input_explicit[ch]=0u;track_edit_patch_full=0u;
 for(uint8_t i=0;i<TRACK_NAME_MAX+1u;i++)track_title[i]=0;
 /* Disable ETCH's built-in ARP/sequence; TRACK drives its notes in ALNK0. */
 native_controls.arp_mode=0u;
 native_audio.perf_last_seq=0u;
 native_audio.voice_limit=8u;
#if defined(BEETLE_NATIVE_V2) && BEETLE_NATIVE_V2
 /* Bind after boot metadata and flash ownership preflight have finished
  * using project scratch. Otherwise boot overwrites live runtime header. */
 beetle_bd4_live_bind(beetle_v2_workspace(),sizeof(beetle_staged));
#endif
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
/* Follow the lane's SOURCE pattern in Song Mode. A composed Song row may
 * combine six patterns, so the bounded FX pool must never be merged into one. */
static const struct track_extra_fx *track_audio_extra(uint8_t lane,uint8_t row,uint8_t note2,uint8_t slot){
 uint8_t pat=track_state.pattern_slot;
 if(track_song.enabled&&track_song.composite_active&&track_song.position<track_song.count){
  uint8_t src=track_song_source(track_song.chain[track_song.position].lane[lane]);
  if(src>=TRACK_PATTERNS)return 0;
  pat=src;
  const struct track_pattern *p=&track_song.patterns[src];
  row=(uint8_t)(row%(p->length?p->length:16u));
 }
 return track_extra_find(&track_song,pat,track_extra_key(lane,row,note2),slot);
}
static void track_audio_fire(struct etch_native_audio *audio,uint8_t lane_mask){
 (void)audio;
 struct track_mixer live_trigger_mix;
 const struct track_mixer *trigger_mix=&track_mix;
 if(track_song.enabled && (track_live_mutes.overridden || track_live_solos.overridden)){
  live_trigger_mix=track_mix;
  for(uint8_t c=0u;c<TRACK_CHANNELS;c++){
   live_trigger_mix.ch[c].mute=beetle_live_value(&track_live_mutes,c,track_mix.ch[c].mute);
   live_trigger_mix.ch[c].solo=beetle_live_value(&track_live_solos,c,track_mix.ch[c].solo);
  }
  trigger_mix=&live_trigger_mix;
 }
 const uint8_t position=track_state.transport.play_step;
 /* Drum channel: two independent sample triggers on the same step. The
  * second hit reuses the former CHD byte (0=empty; 1..16=drum index+1).
  * Oscillator channels remain fully unchanged. */
 const struct track_step *dr=&track_state.active.ch[TRACK_DRUM_CHANNEL][position];
 /* V18 drum NOTE1 and NOTE2 volume apply independently to newly triggered hits. */
 track_drum.step_gain=100u;
 if(!(lane_mask&(1u<<TRACK_DRUM_CHANNEL))){}
 else if(!(track_state.punch_mask&(1u<<7))){
  /* The two drum columns have independent STOP semantics. A STOP in NOTE1
   * must not suppress or stop NOTE2; a STOP in NOTE2 must not stop NOTE1.
   * Remember the individual triggered voice serials, including when the
   * engine reduces a mutually exclusive hi-hat pair to one physical voice. */
  if(dr->note==TRACK_NOTE_STOP && track_drum_note1_serial){
   for(uint8_t i=0u;i<TRACK_DRUM_POLY;i++)if(track_drum.voices[i].active &&
      track_drum.voices[i].sequence==track_drum_note1_serial)track_drum.voices[i].active=0u;
   track_drum_note1_serial=0u;
  }
  if(dr->chord==15u && track_drum_note2_serial){
   for(uint8_t i=0u;i<TRACK_DRUM_POLY;i++)if(track_drum.voices[i].active &&
      track_drum.voices[i].sequence==track_drum_note2_serial)track_drum.voices[i].active=0u;
   track_drum_note2_serial=0u;
  }
  if(!track_groove_clock.lane[TRACK_DRUM_CHANNEL].skip_note &&
     track_mixer_velocity(trigger_mix,TRACK_DRUM_CHANNEL,94u)){
  uint8_t first=(dr->note<TRACK_DRUM_NOTE_COUNT)?dr->note:TRACK_NOTE_EMPTY;
  uint8_t second=(dr->chord>=1u&&dr->chord<=TRACK_DRUM_COUNT)?(uint8_t)(dr->chord-1u):TRACK_NOTE_EMPTY;
  uint32_t serial=track_drum.trigger_serial;
  (void)track_drum_trigger_pair(&track_drum,first,second);
  /* Pair-trigger may deliberately suppress duplicate/mutually exclusive hats.
   * In that case the second voice was not created and cannot be STOPped. */
  if(track_drum.trigger_serial>serial){
   for(uint8_t i=0u;i<TRACK_DRUM_POLY;i++){
    const struct track_drum_voice *v=&track_drum.voices[i];
    if(v->active&&(int32_t)(v->sequence-serial)>0){
     if(first<TRACK_DRUM_NOTE_COUNT && v->component==first &&
        (!track_drum_note1_serial || v->sequence>track_drum_note1_serial))track_drum_note1_serial=v->sequence;
     if(second<TRACK_DRUM_COUNT && v->component==second &&
        (!track_drum_note2_serial || v->sequence>track_drum_note2_serial) &&
        !(first==second && track_drum_note1_serial==v->sequence))track_drum_note2_serial=v->sequence;
    }
   }
  }
  if((track_state.punch_mask&(1u<<4)) && track_drum.trigger_serial>serial){
   uint8_t synth_playing=0u;
   for(uint8_t si=0u;si<track_chip.voice_count;si++)
    if(track_chip.voices[si].active){synth_playing=1u;break;}
   if(!synth_playing && beetle_stutter_index(track_chip.punch_param[4])>=5u)track_chip.punch_position=0u;
  }
  /* Scale only the new drum voices; never change an older hit's tail. */
  uint8_t vel=track_step_velocity(dr),gate=track_step_gate(dr);
  uint32_t period=661500u/(track_state.transport.bpm?track_state.transport.bpm:120u);
  for(uint8_t v=0u;v<TRACK_DRUM_POLY;v++){
   struct track_drum_voice *hit=&track_drum.voices[v];
   if(hit->active && (int32_t)(hit->sequence-serial)>0){
    /* Pair trigger is first-then-second, except hat choke reduces it to one.
     * Identify the actual fired NOTE2 voice without touching a prior note tail. */
    uint8_t second_hit=(uint8_t)(first>=TRACK_DRUM_NOTE_COUNT ||
      (second<TRACK_DRUM_COUNT && hit->sequence!=serial+1u) ||
      (first==3u && second==2u));
    const struct track_note2_fx *f2=&track_state.active.note2_fx[position];
    uint8_t fx=second_hit?f2->effect:dr->effect;
    uint8_t value=second_hit?f2->value:dr->value;
    if(fx==TRACK_FX_DRUM_PITCH){
      int32_t pitch=(int32_t)hit->pitch_semitones+(int32_t)value-12;
      if(pitch< -36)pitch=-36;if(pitch>36)pitch=36;
      hit->pitch_semitones=(int8_t)pitch;
      /* Pitch->rate conversion uses the same drum-voice helper as old pair tuning. */
      track_drum_retune_voice(&track_drum,hit);
    }
    if(fx==TRACK_FX_VOLUME)hit->gain_q8=(uint16_t)((uint32_t)hit->gain_q8*value/100u);
    /* Additional NOTE1/NOTE2 FX are independent for each drum hit. */
    for(uint8_t slot=0u;slot<2u;slot++){
     const struct track_extra_fx *extra=track_audio_extra(TRACK_DRUM_CHANNEL,position,second_hit,slot);
     if(!extra)continue;
     if(track_extra_effect(extra)==TRACK_FX_DRUM_PITCH){
      int32_t pitch=(int32_t)hit->pitch_semitones+(int32_t)extra->value-12;
      if(pitch< -36)pitch=-36;if(pitch>36)pitch=36;
      hit->pitch_semitones=(int8_t)pitch;track_drum_retune_voice(&track_drum,hit);
     }else if(track_extra_effect(extra)==TRACK_FX_VOLUME)
      hit->gain_q8=(uint16_t)((uint32_t)hit->gain_q8*extra->value/100u);
    }
    hit->gain_q8=(uint16_t)((uint32_t)hit->gain_q8*vel/94u);
    if(gate){uint32_t cap=(period*gate)/16u;if(cap<64u)cap=64u;
      if(track_groove_clock.lane[TRACK_DRUM_CHANNEL].spacing<cap)cap=track_groove_clock.lane[TRACK_DRUM_CHANNEL].spacing;
      if(hit->length>cap)hit->length=cap;}
   }
  }
  }
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
  const struct track_extra_fx *extra2=track_audio_extra(c,position,0u,0u);
  const struct track_extra_fx *extra3=track_audio_extra(c,position,0u,1u);
  beetle_step_fx_event(&track_chip,c,s,duty,cutoff,track_state.transport.bpm);
  if(s->note!=TRACK_NOTE_STOP){
   /* FX2/FX3 may start on an empty primary FX1, including an effect-only
    * row applied to sustained voices. Reset old per-step parameters first. */
   if((extra2||extra3)&&!s->effect&&s->note==TRACK_NOTE_EMPTY){
    track_chip.fx_type[c]=0u;track_chip.fx_value[c]=50u;
    for(uint8_t j=0u;j<2u;j++){
     track_chip.fx_extra_type[c][j]=0u;track_chip.fx_extra_value[c][j]=50u;}
    track_chip.duty[c]=duty;track_chip.cutoff[c]=cutoff;
    track_chip.drive[c]=0u;track_chip.cut_after[c]=0xFFFFFFFFu;
    track_chip.step_gain[c]=100u;track_chip.fx_samples[c]=0u;
   }
   beetle_step_fx_multi(&track_chip,c,extra2,extra3,track_state.transport.bpm);
  }
  if(s->note==TRACK_NOTE_STOP){track_chip_release_channel(&track_chip,c);track_gate_remaining[c]=0u;continue;}
  velocity[c]=track_patch_volume_velocity(instruments[c],track_mixer_velocity(trigger_mix,c,track_step_velocity(s)));
  if(s->note==TRACK_NOTE_EMPTY || track_groove_clock.lane[c].skip_note || (track_state.punch_mask&(1u<<7)) || velocity[c]==0u)continue;
  int32_t note=(int32_t)s->note;
  /* V0.83: octaves now apply centrally in chip DSP to all types,
   * including sustained voices and NES NOISE; never double-transpose here.
   * PITCH is also applied inside chip DSP to the original note. */
  if(note<0)note=0;if(note>127)note=127;
  /* EMPTY sustains; STP stops; a NEW note fades out earlier pitches of
   * this SAME lane, avoiding stuck chords across repeated patterns. */
  track_chip_stop_channel(&track_chip,c);
  {uint8_t gate=track_step_gate(s);
   /* 0: preserve legacy legato; 1..15: fraction of this sequencer step.
    * Release is driven by the sample clock, never by LCD refresh. */
   uint32_t period=661500u/(track_state.transport.bpm?track_state.transport.bpm:120u);
   if(period<551u)period=551u;
   track_gate_remaining[c]=gate?(period*(uint32_t)gate)/16u:0u;
   if(gate&&!track_gate_remaining[c])track_gate_remaining[c]=1u;
   if(TRACK_GROOVE_RATCHET(s)>1u && track_gate_remaining[c]>=track_groove_clock.lane[c].spacing)
    track_gate_remaining[c]=track_groove_clock.lane[c].spacing>1u?track_groove_clock.lane[c].spacing-1u:1u;
  }
  lengths[c]=track_chord_keylocked((uint8_t)note,s->chord,&track_state.harmony,pitches[c]);
 }
 /* First guarantee each channel its root, then share the remaining four
  * voices with chord tones. Never let CH1 consume all eight voices. */
 for(uint8_t n=0;n<5u;n++)for(uint8_t c=0;c<TRACK_DRUM_CHANNEL;c++){
  if(n<lengths[c]){
   if(!track_chip_note(&track_chip,c,instruments[c],pitches[c][n],velocity[c]))return;
  }
 }
}
/* Start a step with micro-delayed, probability-gated ratchet events.
 * Existing swing is respected; durations never exceed one step. */
static void track_song_apply_stops(void){
 /* A Song Chain -- behaves like STP at the ROW boundary, not merely a blank
  * step. A deliberately selected empty numbered pattern is allowed to hold.
  * Never clear the whole mixer or other channels. */
 uint8_t stops=track_song_stop_mask(&track_song);
 for(uint8_t c=0;c<TRACK_SYNTH_CHANNELS;c++)if(stops&(1u<<c)){
  track_chip_release_channel(&track_chip,c);track_gate_remaining[c]=0u;
 }
 if(stops&(1u<<TRACK_DRUM_CHANNEL))track_drum_choke_all(&track_drum);
}
static void track_groove_start_step(struct etch_native_audio *audio,uint32_t interval){
 uint32_t swing[TRACK_CHANNELS];
 for(uint8_t c=0;c<TRACK_CHANNELS;c++)swing[c]=
  (track_state.transport.play_step&1u)?track_swing_delay(interval,track_mix.ch[c].swing):0u;
 beetle_groove_schedule(&track_groove_clock,&track_state.active,
    track_state.transport.play_step,interval,swing);
 uint8_t due=beetle_groove_due(&track_groove_clock,0u);
 if(due)track_audio_fire(audio,due);
}
/* Render exact step boundaries even when they fall in the middle of DMA
 * blocks. Timing uses integer sample counts; no allocations or UI in ALNK0. */
static void track_fm1_audio_process(struct etch_native_audio *audio,uint32_t frames,int16_t *dst){
 if(!dst)return;
 beetle_isr_song_position=(uint8_t)(track_song.enabled&&track_song.composite_active?
    track_song.position+1u:0u);
#if ETCH_NATIVE_UAC
 beetle_stem_capture=(uac_multitrack && uac.feed)?track_usb_stem_pcm:0;
 beetle_stem_offset=0u;
#endif
 struct track_mixer live_audio_mix;
 const struct track_mixer *render_mix=&track_mix;
 if(track_song.enabled && (track_live_mutes.overridden || track_live_solos.overridden)){
  live_audio_mix=track_mix;
  for(uint8_t c=0u;c<TRACK_CHANNELS;c++){
   live_audio_mix.ch[c].mute=beetle_live_value(&track_live_mutes,c,track_mix.ch[c].mute);
   live_audio_mix.ch[c].solo=beetle_live_value(&track_live_solos,c,track_mix.ch[c].solo);
  }
  render_mix=&live_audio_mix;
 }
 if(!track_state.transport.playing){
  track_song_restore(&track_song,&track_state);
  if(track_audio_started){track_audio_all_off(audio);track_audio_started=0u;}
  track_audio_clock=0u;track_swing_pending_mask=0u;
  for(uint8_t ci=0u;ci<TRACK_SYNTH_CHANNELS;ci++)track_gate_remaining[ci]=0u;
  track_chip_set_punch_octave(&track_chip,track_state.punch_mask);
  track_chip_render(&track_chip,render_mix,dst,frames);
  track_drum_render(&track_drum,render_mix,dst,frames);track_chip.punch_bpm=track_state.transport.bpm;track_chip_process_punch(&track_chip,track_state.punch_mask,dst,frames);track_master_process(&track_master_dsp,&track_master_cfg,dst,frames);return;
 }
 if(!track_audio_started){
  track_audio_started=1u;track_audio_clock=0u;
  if(track_song.enabled&&track_song.composite_active)track_song_apply_stops();
  track_replay_punch_mask=track_song_effect_mask_boundary(&track_song,track_song_punch_pattern(&track_song,&track_state),0u,0u);
  track_sync_punch_params();
  track_state.punch_mask=(uint16_t)(track_live_punch_mask|track_replay_punch_mask);
  track_chip_begin_step(&track_chip);
  track_swing_pending_mask=0u;
  uint32_t period=661500u/(track_state.transport.bpm?track_state.transport.bpm:120u);
  if(period<551u)period=551u;
  track_groove_start_step(audio,period);
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
  uint8_t due=beetle_groove_due(&track_groove_clock,track_audio_clock);
  if(due)track_audio_fire(audio,due);
  uint32_t until=beetle_groove_wait(&track_groove_clock,track_audio_clock,interval-track_audio_clock);
  for(uint8_t ci=0u;ci<TRACK_SYNTH_CHANNELS;ci++)
   if(track_gate_remaining[ci]&&track_gate_remaining[ci]<until)until=track_gate_remaining[ci];
  uint32_t chunk=frames-offset<until?frames-offset:until;
  beetle_stem_offset=offset;
  track_chip_set_punch_octave(&track_chip,track_state.punch_mask);track_chip_render(&track_chip,render_mix,dst+offset*2u,chunk);
  track_drum_render(&track_drum,render_mix,dst+offset*2u,chunk);
  track_chip.punch_bpm=track_state.transport.bpm;track_chip_process_punch(&track_chip,track_state.punch_mask,dst+offset*2u,chunk);
  track_master_process(&track_master_dsp,&track_master_cfg,dst+offset*2u,chunk);
  offset+=chunk;track_audio_clock+=chunk;
  for(uint8_t ci=0u;ci<TRACK_SYNTH_CHANNELS;ci++)if(track_gate_remaining[ci]){
   if(track_gate_remaining[ci]<=chunk){track_gate_remaining[ci]=0u;track_chip_release_channel(&track_chip,ci);}
   else track_gate_remaining[ci]-=chunk;
  }
  if(track_audio_clock>=interval){
   track_audio_clock-=interval;
   track_swing_pending_mask=0u;
   track_tick(&track_state);
   uint8_t song_position_before_wrap=track_song.position;
   if(track_state.transport.play_step==0u && track_song.enabled){
    if(track_song_on_wrap(&track_song,&track_state)<0){
     track_punch_record_finish();track_replay_punch_mask=0u;
     track_sync_punch_params();
     track_state.punch_mask=track_live_punch_mask;
     track_audio_all_off(audio);track_audio_started=0u;beetle_mute_cancel(&track_mute_queue);track_live_mutes.pending=track_live_solos.pending=0u;track_live_mutes.pending_values=track_live_solos.pending_values=0u;
     if(offset<frames){beetle_stem_offset=offset;track_chip_set_punch_octave(&track_chip,track_state.punch_mask);track_chip_render(&track_chip,render_mix,dst+offset*2u,frames-offset);track_drum_render(&track_drum,render_mix,dst+offset*2u,frames-offset);track_chip.punch_bpm=track_state.transport.bpm;track_chip_process_punch(&track_chip,track_state.punch_mask,dst+offset*2u,frames-offset);track_master_process(&track_master_dsp,&track_master_cfg,dst+offset*2u,frames-offset);}
     return;
    }
   }
   if(track_song.enabled && track_song.position!=song_position_before_wrap){
    track_song_apply_stops();
    track_punch_record_song_transition();
   }
   beetle_isr_song_position=(uint8_t)(track_song.enabled&&track_song.composite_active?
      track_song.position+1u:0u);
   track_replay_punch_mask=track_song_effect_mask_boundary(&track_song,
        track_song_punch_pattern(&track_song,&track_state),track_state.transport.play_step,
        (uint8_t)(track_state.transport.play_step!=0u ||
                  !track_song.enabled || !track_song.composite_active ||
                  track_song.position==song_position_before_wrap));
   track_sync_punch_params();
   track_state.punch_mask=(uint16_t)(track_replay_punch_mask|track_live_punch_mask);
   track_chip_begin_step(&track_chip);
   track_swing_pending_mask=0u;
   /* Transport-facing live mutes commit exactly at the new pattern start.
    * Playback can be stopped mid-step without activating queued changes. */
   if(track_state.transport.play_step==0u){
    beetle_mute_commit(&track_mute_queue,&track_mix);
    if(track_song.enabled){
     beetle_live_commit(&track_live_mutes);beetle_live_commit(&track_live_solos);
     live_audio_mix=track_mix;
     for(uint8_t c=0u;c<TRACK_CHANNELS;c++){
      live_audio_mix.ch[c].mute=beetle_live_value(&track_live_mutes,c,track_mix.ch[c].mute);
      live_audio_mix.ch[c].solo=beetle_live_value(&track_live_solos,c,track_mix.ch[c].solo);
     }
     render_mix=&live_audio_mix;
    }
   }
   track_groove_start_step(audio,interval);
  }
 }
}
/* Sample-current note/chord LED preview for the Song Chain cursor lane.
 * This scans at most ONE 5-note chord in the UI task, never the audio ISR.
 * Song row focus stays put when stopped; while playing the cursor selects
 * the column and the live row gives the sounding material. */
static uint32_t beetle_song_key_led_mask(void){
 if(track_page!=9u || track_song.edit_column>=TRACK_CHANNELS)return 0u;
 const uint8_t row=(uint8_t)(track_state.transport.playing&&track_song.enabled?track_song.position:track_song.edit_row);
 const uint8_t lane=track_song.edit_column;
 if(row>=track_song.count)return 0u;
 const uint8_t ref=track_song.chain[row].lane[lane];
 if(!track_song_lane_visible(&track_song,ref,lane))return 0u;
 const uint8_t src=track_song_source(ref);
 if(src>=TRACK_PATTERNS)return 0u;
 const struct track_pattern *pat=&track_song.patterns[src];
 const uint8_t length=pat->length?pat->length:1u;
 const uint8_t step=(uint8_t)(track_state.transport.play_step%length);
 const struct track_step *st=&pat->ch[lane][step];
 if(!track_state.transport.playing && st->note>=TRACK_NOTE_STOP &&
    (lane!=TRACK_DRUM_CHANNEL || st->chord==0u))return 0u;
 uint32_t mask=0u;
 if(lane==TRACK_DRUM_CHANNEL){
  if(track_state.transport.playing){
   for(uint8_t v=0u;v<TRACK_DRUM_POLY;v++){
    const struct track_drum_voice *voice=&track_drum.voices[v];
    if(voice->active&&voice->id<27u)mask|=(uint32_t)1u<<voice->id;
   }
  }else{
   if(st->note<27u)mask|=(uint32_t)1u<<st->note;
   if(st->chord>=1u && st->chord<=TRACK_DRUM_COUNT)
    mask|=(uint32_t)1u<<(st->chord-1u);
  }
 }else if(track_state.transport.playing){
  /* The actual sounding voices carry across empty steps; this is cheaper
   * and more accurate than scanning Song patterns for note history. */
  const int32_t key_origin=53+12*(int32_t)native_controls.octave;
  for(uint8_t v=0u;v<TRACK_CHIP_VOICES;v++){
   const struct track_chip_voice *voice=&track_chip.voices[v];
   if(!voice->active||voice->channel!=lane||voice->released)continue;
   int32_t key=(int32_t)voice->note-key_origin;
   if(key>=0 && key<27)mask|=(uint32_t)1u<<(uint32_t)key;
  }
 }else{
  uint8_t notes[5];uint8_t count=track_chord_keylocked(st->note,st->chord,&track_state.harmony,notes);
  const int32_t key_origin=53+12*(int32_t)native_controls.octave;
  for(uint8_t n=0u;n<count;n++){
   int32_t key=(int32_t)notes[n]-key_origin;
   if(key>=0 && key<27)mask|=(uint32_t)1u<<(uint32_t)key;
  }
 }
 return mask;
}
#include "beetle_rec_led.h"
static void track_fm1_leds(void){
 /* Animation planes are NOT normal TRACK LED state. The original splash
  * handed full-brightness LEDs back but forgot the dim/mid planes, so a
  * trailing ripple could remain lit indefinitely on several physical keys.
  * Clear those planes on EVERY normal refresh, including boot-to-tracker.
  * Unlike a cosmetic delay, this also handles an interrupted boot wave. */
 /* Splash/ripple LEDs own every key and every function button until the
  * startup screen ends; normal mute/guide lights must not overwrite them. */
 if((int32_t)(track_boot_splash_until-etch_ms)>0){
  fm1_irq_off();
  for(uint8_t id=0u;id<41u;id++){
   uint8_t light=beetle_ripple_level(etch_ms-beetle_ripple_start_ms,id);
   fm1_led_key(id,light==2u);
   native_seq_led_dim(id,light==1u);
   native_seq_led_mid(id,0u);
  }
  fm1_irq_enable_all();return;
 }
 fm1_irq_off();
 uint32_t song_keys=beetle_song_key_led_mask();
 for(uint8_t col=0u;col<FM1_NCOL;col++){
  fm1_led_dim[col]=0u;
  fm1_led_mid[col]=0u;
  fm1_led_breath[col]=0u;
 }
 /* REC LED now follows the active REC mode, never the previous screen. */
 for(uint8_t b=2u;b<14u;b++)fm1_led_key(b,b==13u?beetle_rec_led_on(track_page,track_state.record_armed,track_punch_rec_armed):1u);
 struct etch_harmony_settings h={track_state.harmony.root,track_state.harmony.mode,track_state.harmony.keylock,0u};
 uint32_t guide=etch_led_scale_guide_targets(native_controls.octave,&h);
 uint32_t locked=etch_led_keylock_targets(fm1_in.notes,native_controls.octave,&h);
 for(uint8_t k=0u;k<27u;k++){
  uint8_t active=(uint8_t)(((fm1_in.notes|guide|locked|song_keys)>>k)&1u);
  if(track_page==6u || track_page==30u){
   int8_t lane=beetle_black_mute_lane(k);
   int8_t solo_lane=beetle_white_solo_lane(k);
   if(track_page==30u){
    active=(uint8_t)(lane>=0?beetle_live_value(&track_live_mutes,(uint8_t)lane,track_mix.ch[(uint8_t)lane].mute):
      solo_lane>=0?beetle_live_value(&track_live_solos,(uint8_t)solo_lane,track_mix.ch[(uint8_t)solo_lane].solo):0u);
   }else active=(uint8_t)(lane>=0?beetle_mute_next(&track_mute_queue,&track_mix,(uint8_t)lane):
       solo_lane>=0?track_mix.ch[(uint8_t)solo_lane].solo:0u);
  }
  fm1_led_key(14u+k,active);
 }
 /* Persistent octave offset feedback: minus LED for negative values, plus
  * LED for positive ones. Both dark at 0, unless physically held. */
 fm1_led_key(0u,(uint8_t)(track_page==30u?track_live_q_mute:(native_controls.octave<0 || (fm1_in.buttons&1u))));
 fm1_led_key(1u,(uint8_t)(track_page==30u?track_live_q_solo:(native_controls.octave>0 || (fm1_in.buttons&2u))));
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
 return playing?(uint8_t)((track_ui_step_override?track_ui_step_snapshot:track_state.transport.play_step)&1u):1u;
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
 if(track_battery_low20){lcd_fill(101u,3u,39u,9u,TRACK_DARK);track_pix_center("BEETLE",120u,4u,0xF800u);}
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
     track_drum_name((uint8_t)(step->chord-1u)):(step->chord==15u?"STP":"---");
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
 static const char *const drums[]={"STEP","NOTE","NOTE2","KIT","FX"};
 static const uint16_t xs[]={36u,77u,121u,163u,202u};
 static const char *const chords[TRACK_CHORD_COUNT]={"---","MAJ","MIN","DIM","AUG","SU2","SU4","MJ7","MN7","DO7","AD9","MN9"};
 static const char *const chord_full[TRACK_CHORD_COUNT]={"EMPTY","MAJOR","MINOR","DIMINISHED","AUGMENTED","SUSPENDED 2","SUSPENDED 4","MAJOR 7","MINOR 7","DOMINANT 7","ADD 9","MINOR 9"};
 uint8_t drum=(uint8_t)(track_state.transport.edit_channel==TRACK_DRUM_CHANNEL);
 for(uint8_t i=0u;i<5u;i++)track_pix_center(drum?drums[i]:headings[i],xs[i],28u,TRACK_WHITE);
 const uint8_t drawn_step=track_ui_step_override?track_ui_step_snapshot:track_state.transport.play_step;
 uint8_t first=(uint8_t)((track_state.transport.playing?drawn_step:track_state.transport.edit_step)&0xF0u);
 for(uint8_t i=0u;i<16u;i++){
  uint8_t row=(uint8_t)(first+i); if(row>=track_state.active.length || row>=TRACK_STEPS)continue;
  uint16_t y=(uint16_t)(43u+(uint16_t)i*10u);
  if(etch_scope_active && etch_panel_composite &&
     (y+11u<=etch_scope_origin || y-2u>=etch_scope_origin+etch_scope_height))continue;
  uint8_t focused=(uint8_t)(!track_state.transport.playing&&track_state.transport.edit_step==row);
  /* The audio ISR can advance while SPI DMA is busy. Both repainted rows
   * must use the same step snapshot, with no long IRQ-off critical region. */
  uint8_t live=(uint8_t)(track_state.transport.playing&&drawn_step==row);
  if(beetle_range_on && !track_state.transport.playing){
   uint8_t a=beetle_range_anchor,b=beetle_range_end;
   if(a>b){uint8_t t=a;a=b;b=t;}
   if(row>=a && row<=b)lcd_fill(22u,y-1u,198u,9u,0x6810u);
  }
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
   track_pix_cell(st->chord>=1u&&st->chord<=TRACK_DRUM_COUNT?track_drum_name((uint8_t)(st->chord-1u)):(st->chord==15u?"STP":"---"),xs[2],y);
   track_u8(number,st->instrument+1u);track_pix_cell("KIT",xs[3],y);
  }else{
   char pitch[5];track_note_label(st->note,pitch);track_pix_cell(pitch,xs[1],y);
   if(st->note==255u)track_pix_cell("--",xs[2],y);
   else{track_u8(number,st->instrument+1u);track_pix_cell(number,xs[2],y);}
   track_pix_cell(chords[st->chord<TRACK_CHORD_COUNT?st->chord:0u],xs[3],y);
  }
  const char *fx=track_web_stepfx[st->effect<TRACK_FX_COUNT?st->effect:0u];
  char fx3[4]={fx[0],fx[1],fx[2],0};
  uint8_t source=track_state.pattern_slot,fxrow=row;
  const uint8_t drawn_song_pos=track_ui_step_override?track_ui_song_pos_snapshot:track_song.position;
 if(track_song.enabled && track_song.composite_active && drawn_song_pos<track_song.count){
   uint8_t src=track_song.chain[drawn_song_pos].lane[track_state.transport.edit_channel];
   if(src<TRACK_PATTERNS){source=src;uint8_t len=track_song.patterns[src].length;
    fxrow=(uint8_t)(row%(len?len:TRACK_STEPS));}
  }
  const uint8_t fxkey=track_extra_key(track_state.transport.edit_channel,fxrow,0u);
  const uint8_t synth_extra=track_extra_count(&track_song,source,fxkey);
  uint8_t mul=(uint8_t)((st->effect!=0u)+synth_extra>1u);
  if(drum){
   uint8_t note2fx=track_state.active.note2_fx[row].effect;
   uint8_t drum_n2=(uint8_t)((note2fx!=0u)+track_extra_count(&track_song,source,track_extra_key(TRACK_DRUM_CHANNEL,fxrow,1u)));
   mul|=(uint8_t)(drum_n2>1u);
   if(!st->effect && note2fx){const char *f2=track_web_stepfx[note2fx<TRACK_FX_COUNT?note2fx:0u];
    fx3[0]=f2[0];fx3[1]=f2[1];fx3[2]=f2[2];}
  }
  if(!st->effect && !mul && synth_extra){
   const struct track_extra_fx *single=track_extra_find(&track_song,source,fxkey,0u);
   if(!single)single=track_extra_find(&track_song,source,fxkey,1u);
   if(single){const char *label=track_web_stepfx[track_extra_effect(single)];
    fx3[0]=label[0];fx3[1]=label[1];fx3[2]=label[2];}
  }
  if(mul){fx3[0]='M';fx3[1]='U';fx3[2]='L';}
  track_pix_cell(fx3,xs[4],y);
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
   else{uint8_t queued=(uint8_t)(j==3u && (track_mute_queue.pending&(1u<<c)));
    uint8_t on=j==3u?beetle_mute_next(&track_mute_queue,&track_mix,c):track_mix.ch[c].solo;
    if(queued){val[0]='Q';val[1]=on?'1':'0';val[2]=0;}else{val[0]=on?'O':'-';val[1]=on?'N':'-';val[2]=0;}}
   /* Purple CH index only while actual post-mixer PCM is being produced. */
   uint16_t ink=(j==0u&&track_state.transport.playing&&track_lane_pcm_peak[c]>=96u)?TRACK_PINK:TRACK_WHITE;
   track_pix_text(val,xs[j],y+4u,ink);
  }
 }
 track_pix_text("K1 LANE K2 FIELD K3 SET",10u,207u,TRACK_PINK);
 track_pix_text(track_mix.quantize_mutes?"SEL: Q MUTES ON":"SEL: Q MUTES OFF",10u,222u,track_mix.quantize_mutes?TRACK_PINK:TRACK_WHITE);
}
/* Mixer-adjacent performance screen; full 240x240 bounded layout. */
static void beetle_snapshot_screen(void){
 track_page_header("PERFORMANCE",43u);
 track_pix_text("MUTE SNAPSHOTS  1-4",18u,51u,TRACK_PINK);
 for(uint8_t slot=0u;slot<BEETLE_SNAPSHOT_SLOTS;slot++){
  uint16_t y=(uint16_t)(79u+slot*31u);
  if(track_snapshot_slot==slot)lcd_fill(9u,y-5u,220u,27u,0x4807u);
  char label[12]="SLOT ";label[5]=(char)('1'+slot);label[6]=0;
  track_pix_text(label,15u,y,track_snapshot_slot==slot?TRACK_BRIGHT:TRACK_WHITE);
  if(!(track_snapshots.valid&(1u<<slot))){
   track_pix_text("EMPTY",120u,y,TRACK_WHITE);continue;
  }
  for(uint8_t lane=0u;lane<TRACK_CHANNELS;lane++){
   uint16_t x=(uint16_t)(107u+lane*20u);
   uint16_t fill=(track_snapshots.masks[slot]&(1u<<lane))?TRACK_PINK:TRACK_WHITE;
   lcd_fill(x,y-1u,13u,13u,fill);
   /* Unmuted = white, muted = pink. Both are distinguishable. */
  }
 }
 lcd_fill(10u,205u,219u,1u,0x4807u);
 track_pix_text("K1 SLOT K2 SAVE K3 RECALL",9u,211u,TRACK_WHITE);
 track_pix_text("FX GROUPS   HOME MIXER",9u,225u,TRACK_PINK);
}
static void beetle_group_screen(void){
 track_page_header("MUTE GROUPS",43u);
 track_pix_text("4 GROUPS / 6 LANES",18u,51u,TRACK_PINK);
 for(uint8_t slot=0u;slot<BEETLE_GROUP_SLOTS;slot++){
  uint16_t y=(uint16_t)(79u+slot*31u);
  if(track_group_slot==slot)lcd_fill(9u,y-5u,220u,27u,0x4807u);
  char label[10]="GROUP ";label[6]=(char)('1'+slot);label[7]=0;
  track_pix_text(label,13u,y,track_group_slot==slot?TRACK_BRIGHT:TRACK_WHITE);
  for(uint8_t lane=0u;lane<TRACK_CHANNELS;lane++){
   uint16_t x=(uint16_t)(107u+lane*20u);
   uint8_t member=(track_groups.masks[slot]&(1u<<lane))!=0u;
   if(track_group_slot==slot&&track_group_lane==lane)
    lcd_fill((uint16_t)(x-2u),(uint16_t)(y-3u),17u,17u,TRACK_PINK);
   lcd_fill(x,(uint16_t)(y-1u),13u,13u,member?TRACK_PINK:TRACK_WHITE);
   if(track_group_slot==slot&&track_group_lane==lane)
    lcd_fill((uint16_t)(x+3u),(uint16_t)(y+2u),7u,7u,member?TRACK_DARK:0x4807u);
  }
 }
 lcd_fill(10u,205u,219u,1u,0x4807u);
 track_pix_text("K1 GRP K2 LANE SEL SET",9u,211u,TRACK_WHITE);
 track_pix_text("K3 TOGGLE HOME: SNAPS",9u,225u,TRACK_PINK);
}
/* Live song mixer: white keys 1-6 correspond to CH1..CH6. Pending changes
 * take effect on the NEXT pattern boundary while transport is running.
 * No heap, no flash writes, and no change to the saved Q-mute preference. */
static void track_fm1_live_mixer_screen(void){
 lcd_fill(0u,0u,240u,240u,TRACK_DARK);
 track_page_header("LIVE MIXER",43u);
 track_pix_text("CH      MUTE        SOLO",10u,50u,TRACK_PINK);
 for(uint8_t c=0u;c<TRACK_CHANNELS;c++){
  const uint16_t y=(uint16_t)(72u+c*22u);
  char lane[5]="CH";lane[2]=(char)('1'+c);lane[3]=0;
  track_pix_text(lane,10u,y,TRACK_WHITE);
  uint8_t mute=beetle_live_next(&track_live_mutes,c,track_mix.ch[c].mute);
  uint8_t solo=beetle_live_next(&track_live_solos,c,track_mix.ch[c].solo);
  uint8_t qm=(uint8_t)((track_live_mutes.pending&(1u<<c))!=0u);
  uint8_t qs=(uint8_t)((track_live_solos.pending&(1u<<c))!=0u);
  track_pix_text(qm?(mute?"[Q M]":"[Q -]"):(mute?"[X] M":"[ ] M"),67u,y,qm||mute?TRACK_PINK:TRACK_WHITE);
  track_pix_text(qs?(solo?"[Q S]":"[Q -]"):(solo?"[X] S":"[ ] S"),153u,y,qs||solo?TRACK_PINK:TRACK_WHITE);
 }
 track_pix_text(track_live_q_mute?"OCT-: Q MUTE ON":"OCT-: Q MUTE OFF",8u,210u,track_live_q_mute?TRACK_PINK:TRACK_WHITE);
 track_pix_text(track_live_q_solo?"OCT+: Q SOLO ON":"OCT+: Q SOLO OFF",8u,224u,track_live_q_solo?TRACK_PINK:TRACK_WHITE);
}
static void track_fm1_song_screen(void){
 lcd_fill(0u,0u,240u,240u,TRACK_DARK);
 track_page_header("SONG CHAINS",43u);
 static const char *heads[8]={"POS","CH1","CH2","CH3","CH4","CH5","DR","REP"};
 static const uint8_t xs[8]={5u,37u,65u,93u,121u,149u,177u,207u};
 for(uint8_t c=0;c<8u;c++)track_pix_text(heads[c],xs[c],54u,TRACK_PINK);
 const uint8_t drawn_song_pos=track_ui_step_override?track_ui_song_pos_snapshot:track_song.position;
 const uint8_t focus=(track_song.enabled&&track_state.transport.playing)?drawn_song_pos:track_song.edit_row;
 const uint8_t start=(uint8_t)((focus/8u)*8u);
 for(uint8_t j=0u;j<8u;j++){
  uint8_t r=(uint8_t)(start+j);if(r>=track_song.count)break;
  uint16_t y=(uint16_t)(71u+15u*j);
  if(etch_scope_active && etch_panel_composite &&
     (y+13u<=etch_scope_origin || y-2u>=etch_scope_origin+etch_scope_height))continue;
  uint8_t live=(uint8_t)(track_song.enabled&&track_state.transport.playing&&r==drawn_song_pos);
  if(live){lcd_fill(2u,y-2u,236u,15u,0x780Bu);}
  else if(r==track_song.edit_row)lcd_fill(2u,y-2u,236u,14u,0x4007u);
  if(!track_state.transport.playing&&track_song_range_on &&
    beetle_song_rows_in_range(r,track_song_range_anchor,track_song_range_end))
    lcd_fill(2u,y-2u,236u,2u,TRACK_PINK);
  if(!track_state.transport.playing&&r==track_song.edit_row){
   uint8_t x=xs[track_song.edit_column+1u];
   lcd_fill(x-3u,y-2u,28u,14u,0x880Du);
  }
  char num[4];track_u8(num,r+1u);track_pix_text(num,xs[0],y,TRACK_WHITE);
  for(uint8_t c=0u;c<TRACK_CHANNELS;c++){
   uint8_t pat=track_song.chain[r].lane[c];
   if(!track_song_lane_visible(&track_song,pat,c))track_pix_text("--",xs[c+1u],y,TRACK_WHITE);
   else{track_u8(num,(uint8_t)(track_song_source(pat)+1u));track_pix_text(num,xs[c+1u],y,TRACK_WHITE);}
  }
  track_u8(num,track_song.chain[r].repeats);track_pix_text(num,xs[7],y,TRACK_WHITE);
 }
 track_pix_text(track_song.enabled?(track_song.loop?"[X] SONG ON LOOP ON":"[X] SONG ON LOOP OFF"):(track_song.loop?"[ ] SONG OFF LOOP ON":"[ ] SONG OFF LOOP OFF"),6u,193u,TRACK_PINK);
 track_pix_text("SEL+K1 RANGE  SEQ+OP1/2",6u,207u,TRACK_BRIGHT);
 track_pix_text("OCT+ ADD  OCT- DEL",6u,219u,TRACK_BRIGHT);
 track_pix_text("SEL SONG ON/OFF",6u,231u,TRACK_BRIGHT);
}
static void track_fm1_overlay(const char *title){
 track_fm1_grid(); /* consistent background with the original tracker UI */
 lcd_fill(10u,70u,220u,111u,0x1007u);
 lcd_fill(10u,70u,220u,2u,TRACK_PINK);
 label(title,17u,79u,TRACK_BRIGHT);
}
static void track_record_banner(void){
 /* Never obscure BEETLE's centre-top name or the 20%-battery red state.
  * The small REC status lives in the blank line below the page title. */
 /* Tracker REC state is communicated ONLY by the physical REC LED.
  * Preserve the FX/Punch-In status overlay, without any tracker text. */
 if(track_page!=3u && track_page!=4u)return;
 lcd_fill(77u,33u,86u,9u,TRACK_DARK);
 track_pix_center(track_punch_rec_armed?"FX REC ON":"FX REC OFF",120u,34u,
    track_punch_rec_armed?TRACK_PINK:TRACK_WHITE);
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
 track_pix_text("ANY KEY: PREVIEW SELECTED DRUM",4u,211u,TRACK_PINK);
 track_pix_text("SELECT DRUM  K1 FIELD K3 VALUE",4u,222u,TRACK_BRIGHT);
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
  char name[23];name[0]=(char)('1'+i);name[1]=' ';name[2]='[';
  name[3]=track_master_cfg.enabled[fx]?'X':' ';name[4]=']';name[5]=' ';
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
  /* The shared footer below carries master navigation instructions. */
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
  track_pix_text(fx==1u?"SEL THR K2 RAT K3 GAIN K4 ATK":"SEL WET K2 TAIL K3 TONE K4 WID",6u,210u,TRACK_WHITE);
 }
 track_pix_text("SELECT FX SEL ON/OFF SEQ+K4",3u,224u,TRACK_PINK);
}
static void beetle_draw_keyboard_octave(uint16_t y){
 char msg[24]="KEY OCT ";int8_t v=native_controls.octave;
 msg[8]=(char)(v<0?'-':'+');msg[9]=(char)('0'+(v<0?-v:v));
 msg[10]=' ';msg[11]='O';msg[12]='C';msg[13]='T';msg[14]='-';msg[15]='/';msg[16]='+';msg[17]=0;
 track_pix_text(msg,9u,y,TRACK_WHITE);
}
/* On-screen numerical counters, deliberately no format strings or heap. */
static void track_rc_metric(const char *name,uint32_t value,uint16_t y){
 char digits[18];track_fmt_int(digits,value);
 track_pix_text(name,10u,y,TRACK_WHITE);
 uint16_t width=track_pix_width(digits);
 track_pix_text(digits,width<230u?(uint16_t)(230u-width):8u,y,TRACK_PINK);
}
static void track_fm1_paint(void){
 if((int32_t)(track_boot_splash_until-etch_ms)>0){
  beetle_boot_art((uint8_t)((etch_ms/100u)&1u));
  return;
 }
 if(track_page==22u){
  track_page_header("STEP GROOVE",43u);
  const struct track_step *s=&track_state.active.ch[track_state.transport.edit_channel][track_state.transport.edit_step];
  char n[12];
  track_pix_text("K1 STEP  K2 PARAM",10u,50u,TRACK_WHITE);
  track_pix_text("STEP",12u,75u,TRACK_WHITE);
  track_fmt_int(n,track_state.transport.edit_step+1u);track_pix_text(n,98u,75u,TRACK_PINK);
  track_pix_text("MICRO",12u,102u,track_groove_focus==0u?TRACK_PINK:TRACK_WHITE);
  track_fmt_int(n,TRACK_GROOVE_MICRO(s));track_pix_text(n,108u,102u,track_groove_focus==0u?TRACK_PINK:TRACK_WHITE);
  track_pix_text("RATCHET",12u,131u,track_groove_focus==1u?TRACK_PINK:TRACK_WHITE);
  track_fmt_int(n,TRACK_GROOVE_RATCHET(s));track_pix_text(n,108u,131u,track_groove_focus==1u?TRACK_PINK:TRACK_WHITE);
  track_pix_text("PROB %",12u,160u,track_groove_focus==2u?TRACK_PINK:TRACK_WHITE);
  track_fmt_int(n,TRACK_GROOVE_PROB(s));track_pix_text(n,108u,160u,track_groove_focus==2u?TRACK_PINK:TRACK_WHITE);
  track_pix_text("SEQ+OP1/2 COPY/PASTE",7u,183u,TRACK_WHITE);
  track_pix_text("SEQ+OP3/4 FILL/CLEAR",7u,201u,TRACK_PINK);
  track_pix_text("K3 VALUE SEL RESET",12u,222u,TRACK_WHITE);
  return;
 }
 if(track_page==21u){
  track_page_header("STEP DYNAMICS",43u);
  const struct track_step *s=&track_state.active.ch[track_state.transport.edit_channel][track_state.transport.edit_step];
  char n[12];track_pix_text("K1 STEP  K2 PARAM",10u,49u,TRACK_WHITE);
  track_fmt_int(n,track_state.transport.edit_step+1u);track_pix_text("STEP",12u,76u,TRACK_WHITE);
  track_pix_text(n,72u,76u,TRACK_PINK);
  track_pix_text("VEL",12u,108u,track_dyn_focus?TRACK_WHITE:TRACK_PINK);
  track_fmt_int(n,track_step_velocity(s));track_pix_text(n,95u,108u,track_dyn_focus?TRACK_WHITE:TRACK_PINK);
  track_pix_text("GATE",12u,137u,track_dyn_focus?TRACK_PINK:TRACK_WHITE);
  uint8_t g=track_step_gate(s);
  if(!g)track_pix_text("LEGATO",95u,137u,track_dyn_focus?TRACK_PINK:TRACK_WHITE);
  else {track_fmt_int(n,(uint32_t)g*100u/16u);track_pix_text(n,95u,137u,track_dyn_focus?TRACK_PINK:TRACK_WHITE);track_pix_text("%",128u,137u,TRACK_WHITE);}
  track_pix_text("K3 ADJUST / SEL DEFAULT",8u,184u,TRACK_PINK);
  track_pix_text("SEQ/HOME: TRACKER",12u,217u,TRACK_WHITE);
  return;
 }
 if(track_page==20u){
  /* V0.57: native BEETLE language: pixel type, [ ] row markers,
   * flat 1-bit steps and the exact magenta selection frame used by the
   * tracker cursor.  240x240 bounded; no PICO-8 style UI artwork. */
  track_page_header("BLOCK EDIT / LANE",43u);
  const char *const ops[]={"COPY","PASTE","CLEAR","TRANSPOSE","SHIFT","UNDO","REDO"};
  char num[12];
  char ctxline[24]="CH ";uint8_t at=3u;
  track_fmt_int(num,(uint32_t)track_state.transport.edit_channel+1u);
  for(uint8_t i=0u;num[i]&&at<10u;i++)ctxline[at++]=num[i];
  ctxline[at++]=' ';ctxline[at++]='P';ctxline[at++]='A';ctxline[at++]='T';ctxline[at++]=' ';
  track_fmt_int(num,(uint32_t)track_state.pattern_slot+1u);
  for(uint8_t i=0u;num[i]&&at<22u;i++)ctxline[at++]=num[i];ctxline[at]=0;
  track_pix_text(ctxline,12u,48u,TRACK_PINK);
  track_pix_text("K4 START    K1 END",12u,69u,TRACK_WHITE);
  track_pix_text("RANGE [",12u,87u,TRACK_WHITE);
  track_fmt_int(num,(uint32_t)beetle_ed.first+1u);track_pix_text(num,55u,87u,TRACK_PINK);
  track_pix_text("..",86u,87u,TRACK_WHITE);
  track_fmt_int(num,(uint32_t)beetle_ed.last+1u);track_pix_text(num,104u,87u,TRACK_PINK);
  track_pix_text("]",133u,87u,TRACK_WHITE);
  track_pix_text("K2 ACTION",12u,107u,TRACK_WHITE);
  track_pix_text(ops[beetle_ed.action],112u,107u,TRACK_PINK);
  if(beetle_ed.action==3u||beetle_ed.action==4u){
   track_pix_text("K3 AMOUNT",12u,126u,TRACK_WHITE);
   track_fmt_int(num,(uint32_t)(beetle_ed.amount<0?-beetle_ed.amount:beetle_ed.amount));
   track_pix_text(beetle_ed.amount<0?"-":"+",112u,126u,TRACK_PINK);
   track_pix_text(num,124u,126u,TRACK_PINK);
  }else track_pix_text("K3 AMOUNT: N/A",12u,126u,0x780Bu);
  track_pix_text("STEPS 01-16",12u,148u,TRACK_BRIGHT);
  uint8_t a=beetle_ed.first<beetle_ed.last?beetle_ed.first:beetle_ed.last;
  uint8_t b=beetle_ed.first>beetle_ed.last?beetle_ed.first:beetle_ed.last;
  for(uint8_t st=0u;st<16u;st++){
   uint16_t x=(uint16_t)(13u+st*14u),y=164u;
   uint8_t chosen=(uint8_t)(st>=a&&st<=b);
   uint16_t back=chosen?0x5008u:TRACK_DARK;
   lcd_fill(x,y,12u,15u,chosen?TRACK_PINK:0x7BEFu);
   lcd_fill((uint16_t)(x+1u),(uint16_t)(y+1u),10u,13u,back);
   const struct track_step *cell=&track_state.active.ch[track_state.transport.edit_channel][st];
   uint8_t active=(uint8_t)(cell->note!=TRACK_NOTE_EMPTY||cell->chord||cell->effect);
   if(active)lcd_fill((uint16_t)(x+4u),(uint16_t)(y+4u),4u,7u,chosen?TRACK_BRIGHT:TRACK_PINK);
   else lcd_fill((uint16_t)(x+5u),(uint16_t)(y+6u),2u,3u,chosen?TRACK_PINK:TRACK_WHITE);
  }
  track_pix_text(beetle_ed.status==1u?"BLOCK COPIED":beetle_ed.status==2u?"EDIT APPLIED":
   beetle_ed.status==3u?"REDO OK":beetle_ed.status==4u?"NO CHANGE":
   beetle_ed.status==BEETLE_EDIT_FX_POOL_FULL?"FX STORAGE FULL":"SEL: APPLY",12u,191u,TRACK_PINK);
  lcd_fill(12u,207u,216u,1u,0x4807u);
  track_pix_text("HOME / SEQ: TRACKER",12u,213u,TRACK_WHITE);
  track_pix_text("EDIT WHILE STOPPED",12u,228u,TRACK_PINK);
  return;
 }

 if(track_page==25u){
  track_page_header("INSTRUMENT LIBRARY",43u);
  track_pix_text("8 PROJECT PATCH SLOTS",13u,48u,TRACK_PINK);
  for(uint8_t slot=0u;slot<BEETLE_LIBRARY_SLOTS;slot++){
   uint16_t y=(uint16_t)(66u+slot*16u);
   if(slot==track_library_slot)lcd_fill(9u,y-2u,220u,15u,TRACK_ROW_BG);
   char num[5];track_fmt_int(num,slot+1u);track_pix_text(num,14u,y,slot==track_library_slot?TRACK_PINK:TRACK_WHITE);
   char name[BEETLE_LIBRARY_NAME+1u];
   for(uint8_t k=0u;k<BEETLE_LIBRARY_NAME;k++)name[k]=track_library.name[slot][k];
   name[BEETLE_LIBRARY_NAME]=0;
   track_pix_text(beetle_library_valid(&track_library,slot)?name:"[EMPTY]",48u,y,slot==track_library_slot?TRACK_PINK:TRACK_WHITE);
   if(beetle_library_valid(&track_library,slot))track_pix_text("*",200u,y,TRACK_PINK);
  }
  static const char *const actions[]={"STORE","LOAD","COPY","RENAME"};
  track_pix_text(actions[track_library_action],11u,198u,TRACK_PINK);
  char target[12]="INST ";track_fmt_int(target+5u,track_library_target+1u);
  char dest[12]="DEST ";track_fmt_int(dest+5u,track_library_copy_source+1u);
  track_pix_text(track_library_action==2u?dest:target,125u,198u,TRACK_WHITE);
  track_pix_text(track_library_notice==1u?"PATCH STORED":track_library_notice==2u?"PATCH LOADED":
   track_library_notice==3u?"PATCH COPIED":track_library_notice==4u?"EMPTY SLOT":track_library_notice==5u?"NAME SAVED":"K1 SLOT K2 ACTION",9u,214u,TRACK_PINK);
  track_pix_text("K3 INST  SEL APPLY",9u,227u,TRACK_WHITE);return;
 }
 if(track_page==26u){
  track_page_header("INSTRUMENT NAME",43u);
  track_pix_text("K1 POSITION K2 LETTER",12u,53u,TRACK_PINK);
  char name[BEETLE_LIBRARY_NAME+1u];
  for(uint8_t k=0u;k<BEETLE_LIBRARY_NAME;k++)name[k]=track_library_name_editor.touched[k]?track_library_name_editor.name[k]:'_';
  name[BEETLE_LIBRARY_NAME]=0;
  uint16_t left=(uint16_t)(120u-track_pix_width(name)/2u);
  track_pix_text(name,left,100u,TRACK_WHITE);
  lcd_fill((uint16_t)(left+6u*(track_library_name_editor.cursor%BEETLE_LIBRARY_NAME)),110u,5u,2u,TRACK_PINK);
  track_pix_text("SEL CONFIRM",20u,175u,TRACK_PINK);
  track_pix_text("HOME CANCEL",20u,207u,TRACK_WHITE);return;
 }
 if(track_page==38u){
  track_page_header("SETTINGS",43u);
  track_pix_text("USB AUDIO MODE",15u,58u,TRACK_PINK);
#if ETCH_NATIVE_UAC
  track_pix_text(uac_multitrack?"> MULTITRACK: ON":"> MULTITRACK: OFF",14u,87u,TRACK_WHITE);
  track_pix_text(uac_multitrack?"6 INPUT CHANNELS (MONO)":"2 INPUT CHANNELS (STEREO)",8u,115u,TRACK_BRIGHT);
  track_pix_text("CH1 CH2 CH3 CH4 CH5 CH6",7u,138u,TRACK_WHITE);
  track_pix_text("SEL: SWITCH / USB RECONNECT",6u,168u,TRACK_PINK);
  track_pix_text("DAW MUST REOPEN INPUT DEVICE",6u,190u,TRACK_BRIGHT);
#else
  track_pix_text("USB AUDIO NOT IN THIS BUILD",8u,105u,TRACK_BRIGHT);
#endif
  track_pix_text("HOME: BACK TO PROJECTS",14u,217u,TRACK_WHITE);
  return;
 }
 if(track_page==27u){
  /* Copy under interrupt exclusion: ALNK0 writes counters, never the UI. */
  struct beetle_rc_monitor snap;
  uint32_t clips,voice_shed;
  fm1_irq_off();snap=native_rc;clips=native_audio_clip_frames;voice_shed=native_voice_shed_count;fm1_irq_enable_all();
  track_page_header(track_rc_tab?"RC MIDI INPUT":"RC AUDIO LOAD",43u);
  track_pix_text("READ ONLY / HARDWARE PROBE",10u,40u,TRACK_PINK);
  if(!track_rc_tab){
   track_rc_metric("LAST LATE SONG POS",snap.last_late_song_pos,52u);
   track_rc_metric("DSP MAX x0.1%",beetle_rc_tenths(snap.max_render),64u);
   track_rc_metric("WALL MAX x0.1%",beetle_rc_tenths(snap.max_wall),80u);
   track_rc_metric("NEST MAX x0.1%",beetle_rc_tenths(snap.max_nested),96u);
   track_rc_metric("DMA OK BLOCKS",snap.blocks,112u);
   track_rc_metric("WALL >= 85%",snap.wall_85,128u);
   track_rc_metric("WALL >= 95%",snap.wall_95,144u);
   track_rc_metric("LATE AFTER COPY",snap.late,160u);
   track_rc_metric("BLOCKED BEFORE",snap.guard,176u);
   track_rc_metric("CLIP FRAMES",clips,192u);
   track_rc_metric("VOICE SHED",voice_shed,208u);
  }else{
   track_rc_metric("MIDI RX PACKETS",snap.midi_rx,65u);
   track_rc_metric("MIDI F8 CLOCKS",snap.midi_clock,83u);
   track_rc_metric("RX OVERFLOWS",snap.midi_overflows,101u);
   track_rc_metric("CLOCK MIN MS",snap.clock_min_ms,119u);
   track_rc_metric("CLOCK MAX MS",snap.clock_max_ms,137u);
   track_rc_metric("CLOCK LAST MS",snap.clock_last_ms,155u);
   track_pix_text("MILLISECONDS RESOLUTION",11u,178u,TRACK_PINK);
   track_pix_text("NOT CLOCK SYNC / LATENCY",11u,195u,TRACK_WHITE);
  }
  track_pix_text("K1 TAB  HOME MENU  SEQ EXIT",8u,227u,TRACK_PINK);
  return;
 }
 if(track_page==0u){track_fm1_grid();return;}
 if(track_page==6u){track_fm1_mixer_screen();return;}
 if(track_page==23u){beetle_snapshot_screen();return;}
 if(track_page==24u){beetle_group_screen();return;}
 if(track_page==9u){track_fm1_song_screen();return;}
 if(track_page==30u){track_fm1_live_mixer_screen();return;}
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
   static const char *const fx[]={"REPEAT","RING MOD","FILTER","TAPE","STUTTER","OCT UP","OCT DN","MUTE","SCRATCH","BITCRUSH","PUMP","GLITCH","WOW","PHASER","RADIO","ISOLATOR"};
   track_pix_text(ev?(ev->down?fx[ev->fx&15u]:"[STP]"):"---",68u,y,TRACK_BRIGHT);
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
  const uint8_t use2=(uint8_t)(track_state.transport.edit_channel==TRACK_DRUM_CHANNEL&&track_fx_drum_lane);
  const uint8_t key=track_extra_key(track_state.transport.edit_channel,track_state.transport.edit_step,use2);
  char nv[12],item[3][38];const char *items[3];
  const uint8_t primary=use2?track_state.active.note2_fx[track_state.transport.edit_step].effect:st->effect;
  const uint8_t pv=use2?track_state.active.note2_fx[track_state.transport.edit_step].value:st->value;
  for(uint8_t j=0u;j<3u;j++){
   const struct track_extra_fx *ext=j?track_extra_find(&track_song,track_state.pattern_slot,key,(uint8_t)(j-1u)):0;
   const uint8_t effect=j?track_extra_effect(ext):primary;
   const uint8_t value=j?(ext?ext->value:50u):pv;
   const char *nm=track_web_stepfx[effect<TRACK_FX_COUNT?effect:0u];
   uint8_t at=0u;item[j][at++]='F';item[j][at++]='X';item[j][at++]=(char)('1'+j);item[j][at++]=' ';
   for(uint8_t k=0u;nm[k]&&at<24u;k++)item[j][at++]=nm[k];
   item[j][at++]=' ';track_fmt_int(nv,value);
   for(uint8_t k=0u;nv[k]&&at<35u;k++)item[j][at++]=nv[k];item[j][at]=0;
   items[j]=item[j];
  }
  track_web_list(items,3u,track_fx_slot,80u,26u);
  track_pix_text("K1 ROW   K2 FX TYPE",15u,177u,TRACK_PINK);
  track_pix_text("K3 VALUE OCT- REMOVE",15u,189u,TRACK_PINK);
  track_pix_text("OCT+ NEXT FX SLOT",15u,201u,TRACK_WHITE);
  char cap[30]="FX STORAGE ";track_fmt_int(nv,track_song.extras.count);
  uint8_t at=11u;for(uint8_t k=0u;nv[k]&&at<19u;k++)cap[at++]=nv[k];cap[at++]='/';cap[at++]='4';cap[at++]='8';cap[at]=0;
  track_pix_text(cap,15u,214u,TRACK_PINK);
  return;
 }
 if(track_page==4u){
  track_page_header(track_punch_page?"PUNCH-IN FX 2/2":"PUNCH-IN FX 1/2",43u);
  static const char *const punches[]={"REPEAT","RING MOD","FILTER","TAPE STOP","STUTTER","OCT UP","OCT DOWN","MUTE",
   "SCRATCH","BITCRUSH","PUMP","GLITCH","WOW","PHASER","RADIO","DJ ISOLATOR"};
  for(uint8_t j=0u;j<8u;j++){
   uint8_t fx=(uint8_t)(j+track_punch_page*8u);
   uint16_t x=(uint16_t)(17u+(j%2u)*106u),y=(uint16_t)(58u+(j/2u)*32u);
   uint8_t held=(uint8_t)(((track_live_punch_mask|track_replay_punch_mask)&(1u<<fx))!=0u);
   lcd_fill(x,y,99u,26u,held?0xE818u:0x3806u);
   char name[22];uint8_t a=0u,number=(uint8_t)(fx+1u);
   if(number>=10u)name[a++]='1';name[a++]=(char)('0'+(number%10u));name[a++]=' ';
   uint8_t k=0u;while(punches[fx][k]&&k<15u){name[a++]=punches[fx][k++];}name[a]=0;
   track_pix_text(name,x+4u,y+6u,held?TRACK_DARK:TRACK_WHITE);
   if(held&&(fx<5u||fx>=8u)){
    if(fx==4u){
     static const char *const rates[]={"/8","/4","/2","X1","X2","X4","X8"};
     char alg[16]="BPM ";const char *rate=rates[beetle_stutter_index(track_chip.punch_param[4])];
     uint8_t i=4u;for(uint8_t p=0u;rate[p]&&i<14u;)alg[i++]=rate[p++];alg[i]=0;
     track_pix_text(alg,x+4u,y+16u,TRACK_WHITE);
    }else {char alg[16]="K1  ";char val[12];track_fmt_int(val,track_chip.punch_param[fx]);
     uint8_t i=4u;for(uint8_t p=0u;val[p]&&i<14u;)alg[i++]=val[p++];alg[i]=0;
     track_pix_text(alg,x+4u,y+16u,TRACK_WHITE);}
   }
  }
  track_pix_text("HOLD KEY + K1: VALUE",12u,192u,TRACK_PINK);
  track_pix_text(track_punch_rec_armed?"FX REC: ON":"FX REC: OFF",12u,202u,track_punch_rec_armed?TRACK_PINK:TRACK_WHITE);
  track_pix_text(track_replay_punch_mask?"REC FX PLAYING / LIT":"FX BUTTON: NEXT PAGE",12u,221u,TRACK_BRIGHT);return;
 }
 if(track_page==5u){
  track_page_header(track_osc_bank==0u?"INSTRUMENT / OSC":
    track_osc_bank==1u?"VA OSC A / SHAPE":
    track_osc_bank==2u?"VA OSC B / SHAPE":"VA OSC / MIX",43u);
  /* Explicit patch number mirrors the 1-based INST value in TRACKER. */
  char patch_label[24]="INST 1 / 8";
  patch_label[5]=(char)('1'+(track_input_instrument[track_state.transport.edit_channel]&7u));
  lcd_fill(139u,29u,95u,13u,TRACK_DARK);
  track_pix_text(patch_label,144u,30u,TRACK_PINK);
  if(track_osc_bank){
   static const char *const waves[]={"SAW","TRIANGLE","SQUARE","SINE","PULSE"};
   static const char *const sidwaves[]={"SAW","TRIANGLE","PULSE","NOISE","PULSE"};
   static const char *const tuning[]={"SHAPE","COARSE","FINE","PWIDTH"};
   static const char *const combining[]={"DETUNE B","MIX A/B","MORPH","DRIVE","SUB OSC","NOISE","FM DEPTH"};
   static const char *const sidcombine[]={"DETUNE B","MIX A/B","FILT MODE","RESONANCE","SUB OSC","NOISE","FM DEPTH"};
   const struct track_step *st=&track_state.active.ch[track_state.transport.edit_channel][track_state.transport.edit_step];
   const struct track_va_patch *p=&track_chip.patches[track_input_instrument[track_state.transport.edit_channel]&7u];
   const uint8_t sid_mode=(uint8_t)(track_chip.osc_type[track_input_instrument[track_state.transport.edit_channel]&7u]>=TRACK_CHIP_RESID_LITE && track_chip.osc_type[track_input_instrument[track_state.transport.edit_channel]&7u]<=TRACK_CHIP_SID_NOISE);
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
   track_pix_text("EDITING SAVED PATCH  K3 CHANGE",12u,195u,TRACK_PINK);
   beetle_draw_keyboard_octave(214u);return;
  }
  static const char *const types[]={"RETRO PULSE","RETRO WAVE","VA OSC","VA OSC2","SID 6581","SID 8580","SID DIRTY","SID BASS","SID NOISE","NES PULSE","NES TRIANGLE","NES NOISE"};
  static const char *const keys[]={"TYPE","ATTACK","DECAY","SUSTAIN","RELEASE","DUTY","CUTOFF","VOLUME"};
  const struct track_step *st=&track_state.active.ch[track_state.transport.edit_channel][track_state.transport.edit_step];
  const struct track_instrument_ui_values *v=&track_ui_inst[track_input_instrument[track_state.transport.edit_channel]&7u];
  char values[8][24];char items[8][40];const char *entry[8];
  uint8_t type=track_chip.osc_type[track_input_instrument[track_state.transport.edit_channel]&7u]%TRACK_CHIP_TYPES;
  const char *name=types[type];uint8_t k=0u;
  while(name[k]&&k<23u){values[0][k]=name[k];k++;}values[0][k]=0;
  const uint8_t cents[]={v->attack,v->decay,v->sustain,v->release};
  for(uint8_t i=0u;i<4u;i++){
   /* ADSR fields are percentages 0..100: 100 must be shown as 1.00,
    * not the invalid two-digit overflow 0.:0. Display only; do not
    * modify the stored envelope or the realtime voice parameters. */
   uint8_t n=(uint8_t)(cents[i]>100u?100u:cents[i]);
   values[i+1u][0]=(char)(n==100u?'1':'0');values[i+1u][1]='.';
   values[i+1u][2]=(char)('0'+(n%100u)/10u);
   values[i+1u][3]=(char)('0'+n%10u);values[i+1u][4]=0;
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
  track_pix_text((track_edit_poly?"POLY AUDITION":"MONO AUDITION"),17u,202u,TRACK_PINK);
  beetle_draw_keyboard_octave(216u);return;
 }
 if(track_page==19u){
  track_page_header("HELP",43u);
  uint8_t first=(uint8_t)(beetle_help_chapter_heading(track_help_chapter)+1u);
  uint8_t length=beetle_help_chapter_length(track_help_chapter);
  uint8_t max=beetle_help_chapter_maxscroll(track_help_chapter);
  const char *chapter=beetle_help_lines[first-1u]+2;
  char counter[16],num[8],total[8];
  track_fmt_int(num,(uint32_t)track_help_chapter+1u);
  track_fmt_int(total,beetle_help_chapter_count());
  uint8_t n=0u,j=0u;while(num[j]&&n<5u)counter[n++]=num[j++];counter[n++]='/';
  j=0u;while(total[j]&&n<11u)counter[n++]=total[j++];counter[n]=0;
  track_pix_text("K2: CHAPTER   K1: SCROLL",9u,49u,TRACK_PINK);
  lcd_fill(9u,63u,221u,1u,0x4807u);
  lcd_fill(9u,66u,219u,14u,0x3005u);
  track_pix_text(chapter,12u,69u,TRACK_BRIGHT);
  track_pix_text(counter,196u,69u,TRACK_PINK);
  for(uint8_t row=0u;row<BEETLE_HELP_VISIBLE;row++){
   uint8_t local=(uint8_t)(track_help_offset+row);
   if(local>=length)break;
   uint8_t idx=(uint8_t)(first+local);
   track_pix_text(beetle_help_lines[idx],12u,(uint16_t)(86u+(uint16_t)row*12u),TRACK_WHITE);
  }
  lcd_fill(231u,84u,2u,110u,0x3807u);
  uint16_t thumb_y=(uint16_t)(84u+(max?(uint32_t)track_help_offset*92u/max:0u));
  lcd_fill(230u,thumb_y,4u,18u,TRACK_PINK);
  lcd_fill(9u,201u,221u,1u,0x4807u);
  track_pix_text("HOME: MENU  SEQ: TRACKER",10u,208u,TRACK_BRIGHT);
  char count[18]="";char a[8],b[8];
  uint8_t seen=(uint8_t)(track_help_offset+BEETLE_HELP_VISIBLE);
  if(seen>length)seen=length;
  track_fmt_int(a,seen);track_fmt_int(b,length);
  n=0u;j=0u;while(a[j]&&n<8u)count[n++]=a[j++];count[n++]='/';
  j=0u;while(b[j]&&n<15u)count[n++]=b[j++];count[n]=0;
  track_pix_center(count,120u,225u,TRACK_PINK);
  return;
 }
 if(track_page==29u){
  track_page_header("PROJECTS",43u);
  const char *const options[]={"NEW PROJECT","LOAD PROJECT"};
  track_web_list(options,2u,track_boot_menu_choice,82u,32u);
  track_pix_center("K1 CHOOSE   SEL OPEN",120u,209u,TRACK_PINK);
  return;
 }
 if(track_page==28u){
  track_page_header("CREDITS",43u);
  static const char *const lines[]={
   "BEETLE / SIKKENWORKS",
   "JOPPE SIKKEN",
   "DESIGN / FIRMWARE / UI",
   "",
   "FELUCCA / LEO KUROSHITA",
   "HUGELTON INSTRUMENTS",
   "FM1 HAL / BOOT / OTA",
   "",
   "RESID / DAG LEM (SID REF)",
   "NES_SND_EMU / SHAY GREEN",
   "JAMES ATHEY (MAINTAINER)",
   "NES TECHNICAL REFERENCE"
  };
  /* Keep all credit lines visible above the footer on the 240px display. */
  for(uint8_t i=0u;i<12u;i++){
   uint16_t y=(uint16_t)(53u+i*12u);
   uint16_t color=(i==0u||i==4u||i==8u||i==9u)?TRACK_PINK:TRACK_WHITE;
   track_pix_text(lines[i],8u,y,color);
  }
  track_pix_text("HOME MENU  SEQ TRACKER",9u,217u,TRACK_BRIGHT);
  return;
 }
 if(track_page==33u || track_page==34u){
  track_page_header("WIPE ALL STORAGE",43u);
  track_pix_center(track_page==33u?"ARE YOU SURE?":"REALLY?",120u,80u,TRACK_PINK);
  track_pix_center("DELETE ALL 12 PROJECTS",120u,113u,TRACK_WHITE);
  track_pix_center(track_page==33u?"NO UNDO - BACKUP FIRST":"ETCH/FELUCCA DATA TOO",120u,136u,TRACK_BRIGHT);
  track_pix_center(track_wipe_yes?"NO     > YES <":"> NO <     YES",120u,170u,TRACK_PINK);
  track_pix_center("K1 CHOOSE  SEL CONFIRM",120u,203u,TRACK_WHITE);
  track_pix_center("HOME CANCEL",120u,219u,TRACK_BRIGHT);return;
 }
 if(track_page==35u){
  track_page_header("INITIALIZING STORAGE",43u);
  track_pix_center("ERASE + VERIFY 12 SLOTS",120u,88u,TRACK_PINK);
  char progress[32]="SECTORS ";char val[12];track_fmt_int(val,track_storage_progress_done);
  uint8_t x=8u,j=0u;while(val[j]&&x<22u)progress[x++]=val[j++];progress[x++]='/';
  track_fmt_int(val,track_storage_progress_total);j=0u;while(val[j]&&x<29u)progress[x++]=val[j++];progress[x]=0;
  track_pix_center(progress,120u,125u,TRACK_WHITE);
  track_pix_center("DO NOT POWER OFF",120u,174u,TRACK_PINK);return;
 }
 if(track_page==36u){
  track_page_header(track_wipe_result==0?"STORAGE READY":"WIPE NOT COMPLETED",43u);
  track_pix_center(track_wipe_result==0?"ALL 12 SLOTS [EMPTY]":
    track_wipe_result==-1?"UNKNOWN / FOREIGN STORAGE":"ERASE OR VERIFY ERROR",120u,108u,TRACK_PINK);
  track_pix_center(track_wipe_result==0?"BEETLE MARKER RECREATED":
    "CHECK STORAGE / RECOVERY",120u,142u,TRACK_WHITE);
  track_pix_center("HOME: PROJECT MENU",120u,202u,TRACK_BRIGHT);return;
 }
 if(track_page==37u){
  track_page_header("CLEAR CURRENT LANE",43u);
  track_pix_center("ARE YOU SURE YOU WANT",120u,78u,TRACK_WHITE);
  track_pix_center("TO CLEAR THIS LANE?",120u,96u,TRACK_WHITE);
  char msg[16]="PAT 00 / CH 0";
  msg[4]=(char)('0'+(track_state.pattern_slot+1u)/10u);
  msg[5]=(char)('0'+(track_state.pattern_slot+1u)%10u);
  msg[12]=(char)('1'+track_state.transport.edit_channel);
  track_pix_center(msg,120u,130u,TRACK_BRIGHT);
  track_pix_center(beetle_clear_lane_yes?"NO      > YES <":"> NO <      YES",120u,167u,TRACK_PINK);
  track_pix_center("K1 CHOOSE  SEL CONFIRM",120u,204u,TRACK_WHITE);
  track_pix_center("HOME CANCEL",120u,220u,TRACK_BRIGHT);
  return;
 }
 if(track_page==1u){
  track_page_header("PROJECTS",43u);
  const char *const entries[]={"SAVE","SAVE AS","LOAD PROJECT","NEW PROJECT","HELP","CREDITS","SETTINGS","DIAGNOSTICS","WIPE ALL STORAGE"};
  track_web_list(entries,9u,track_menu_index,47u,16u);
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
  const char *const entries[]={root,scale,lock,"REVERSE KEY SEARCH >"};
  track_web_list(entries,4u,3u,54u,33u);
  track_pix_text("ANALYSE 48 PATS + CHORD TONES",8u,190u,TRACK_PINK);
  track_pix_text("K1 ROOT K2 SCALE K3 LOCK",9u,205u,TRACK_WHITE);
  track_pix_text("SEL SEARCH / HOME BACK",14u,220u,TRACK_BRIGHT);return;
 }
 if(track_page==32u){
  track_page_header("REVERSE KEY / SCALE",43u);
  if(!track_reverse_report.events){
   track_pix_center("NO MELODIC NOTES FOUND",120u,96u,TRACK_PINK);
   track_pix_center("ENTER NOTES OR CHORDS FIRST",120u,122u,TRACK_WHITE);
   track_pix_text("K4 RESCAN   HOME BACK",12u,218u,TRACK_BRIGHT);return;
  }
  uint8_t first=(uint8_t)(track_reverse_index/5u*5u);
  char num[12];char row[35];
  for(uint8_t i=0u;i<5u&&first+i<BEETLE_REVERSE_CANDIDATES;i++){
   uint8_t index=(uint8_t)(first+i);
   const struct beetle_reverse_candidate *c=&track_reverse_report.ranked[index];
   uint8_t at=0u;const char *v=etch_note_name(c->root);
   row[at++]=index==track_reverse_index?'>':' ';
   while(*v&&at<6u)row[at++]=*v++;
   while(at<6u)row[at++]=' ';
   v=etch_mode_name(c->mode);
   while(*v&&at<23u)row[at++]=*v++;
   while(at<24u)row[at++]=' ';
   track_fmt_int(num,c->fit);v=num;
   while(*v&&at<29u)row[at++]=*v++;
   row[at++]='%';row[at]=0;
   if(index==track_reverse_index)lcd_fill(7u,(uint16_t)(56u+i*27u),227u,21u,0x4807u);
   track_pix_text(row,11u,(uint16_t)(59u+i*27u),index==track_reverse_index?TRACK_WHITE:TRACK_BRIGHT);
  }
  const struct beetle_reverse_candidate *selected=&track_reverse_report.ranked[track_reverse_index];
  if(selected->outside==0u){
   track_pix_text("EXACT MATCH (NOT UNIQUE)",11u,196u,TRACK_PINK);
  }else{
   track_pix_text("CLOSEST FIT - OUTSIDE TONES",9u,196u,TRACK_PINK);
  }
  track_pix_text("K1 RESULTS SEL APPLY KEY/SCALE",4u,211u,TRACK_WHITE);
  track_pix_text("HOME BACK  K4 RESCAN",11u,225u,TRACK_BRIGHT);return;
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
   track_storage_result==TRACK_JOURNAL_INVALID?"PROJECT DATA ERROR":
   track_storage_result==TRACK_JOURNAL_IO?"FLASH I/O ERROR":
   track_storage_result==TRACK_JOURNAL_CAPACITY?(track_new_project_full?"NO FREE PROJECT SLOT":"PROJECT TOO FULL"):"STORAGE ERROR",120u,109u,TRACK_WHITE);
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
static void track_storage_full_popup_draw(void){
 /* Topmost 1-bit-style overlay, clipped safely by both LCD half-slices.
  * Never displayed during WRITING/READING: the blocking I/O guard wins. */
 if(track_page!=8u || track_storage_result!=TRACK_JOURNAL_CAPACITY ||
    (int32_t)(track_storage_full_until-etch_ms)<=0)return;
 lcd_fill(12u,85u,216u,83u,TRACK_DARK);
 lcd_fill(12u,85u,216u,2u,TRACK_PINK);
 track_pix_center("STORAGE FULL",120u,98u,TRACK_PINK);
 track_pix_center(track_storage_full_reason==1u?"ALL 12 PROJECT SLOTS USED":"NO SPACE FOR PROJECT",120u,119u,TRACK_WHITE);
 track_pix_center("PROJECT NOT SAVED",120u,140u,TRACK_BRIGHT);
 track_pix_center("FREE SPACE / USE SAVE AS",120u,155u,TRACK_WHITE);
}
static void track_fx_storage_popup_draw(void){
 if((int32_t)(track_fx_full_until-etch_ms)<=0)return;
 /* Draw in both guarded LCD slices, above whichever page rejected an edit. */
 lcd_fill(13u,95u,214u,53u,TRACK_DARK);
 lcd_fill(13u,95u,214u,2u,TRACK_PINK);
 track_pix_center("FX STORAGE FULL",120u,109u,TRACK_BRIGHT);
 track_pix_center("REMOVE FX TO FREE SPACE",120u,127u,TRACK_PINK);
}
static void track_fm1_draw(void){
 track_runtime_validate();
 /* Sample-clock position is latched once for BOTH LCD halves. The IRQ is
  * never disabled during a screen render or SPI DMA transfer. */
 track_ui_step_snapshot=track_state.transport.play_step;
 track_ui_song_pos_snapshot=track_song.position;
 track_ui_step_override=1u;
 /* LCD uses only dedicated guarded RAM, never the pattern song bank. */
 track_panel_begin_slice(0u,120u);track_fm1_paint();track_record_banner();track_input_diag_draw();track_fx_storage_popup_draw();track_storage_full_popup_draw();etch_panel_commit();
 track_panel_begin_slice(120u,120u);track_fm1_paint();track_record_banner();track_input_diag_draw();track_fx_storage_popup_draw();track_storage_full_popup_draw();etch_panel_commit();
 track_ui_step_override=0u;
 /* Reset after BOTH LCD slices, so low-level audio peaks never become stale. */
 if(track_page==6u){fm1_irq_off();for(uint8_t c=0;c<TRACK_CHANNELS;c++)track_lane_pcm_peak[c]=0u;fm1_irq_enable_all();}
}
/* Rebuild only two affected 13-pixel-high tracker rows. Redraw via the
 * actual tracker painter, not an approximate rectangle: all bracket text,
 * FX labels and highlighted backgrounds remain pixel-exact. The clipped
 * strip renderer visits only intersecting cells. Each SPI transfer is ~6 KiB,
 * not two 57.6 KiB full-screen halves. This remains MAIN-LOOP ONLY. */
static void beetle_playhead_grid_strip(uint8_t step){
 const struct beetle_lcd_dirty_band band=beetle_grid_play_band(step);
 track_panel_begin_slice(band.y,band.height);
 track_fm1_paint();
 etch_panel_commit();
}
/* The small two-frame beetle icon is part of the tracker step animation.
 * The panel staging RAM is reused as a tightly packed 22x23 sprite AFTER
 * the two strips have completed. The raw LCD primitive takes RGB565 bytes
 * in swapped order; never hand it a buffer still being modified. */
static void beetle_playhead_icon_blit(uint8_t step){
 if(!beetle_lcd_guard_ok())return;
 const uint32_t *pixels=beetle_play_icon[step&1u];
 const uint16_t pink=(uint16_t)((TRACK_PINK>>8)|(TRACK_PINK<<8));
 for(uint16_t row=0u;row<23u;row++){
  uint32_t bits=pixels[row];
  for(uint16_t col=0u;col<22u;col++)
   etch_scope_canvas[row*22u+col]=(bits&(1u<<(21u-col)))?pink:0u;
 }
 lcd_blit(10u,205u,22u,23u,etch_scope_canvas);
 lcd_sync(); /* staging canvas belongs to the LCD DMA until completion */
}
static void beetle_playhead_grid_delta(uint8_t before,uint8_t after){
 if(before==after)return;
 track_ui_step_snapshot=after;
 track_ui_song_pos_snapshot=track_song.position;
 track_ui_step_override=1u;
 beetle_playhead_grid_strip(before);
 beetle_playhead_grid_strip(after);
 track_ui_step_override=0u;
 beetle_playhead_icon_blit(after);
}
/* In Song Chains, redraw only the two 15-pixel highlight rows while the
 * focused position stays inside one 8-row page. Page crossings still need
 * a full screen render to expose the next eight song positions. */
static void beetle_playhead_song_strip(uint8_t position){
 const struct beetle_lcd_dirty_band band=beetle_song_play_band(position);
 track_panel_begin_slice(band.y,band.height);
 track_fm1_paint();
 etch_panel_commit();
}
static void beetle_playhead_song_delta(uint8_t before,uint8_t after){
 if(before==after || (before>>3u)!=(after>>3u))return;
 /* Never let Song Mode advance halfway through the two dirty strips:
  * the UI must remember the position that actually reached the LCD. */
 track_ui_song_pos_snapshot=after;
 track_ui_step_override=1u;
 beetle_playhead_song_strip(before);
 beetle_playhead_song_strip(after);
 track_ui_step_override=0u;
}
static void track_begin_save(void){
 track_save_return_page=track_page==29u?0u:track_page;
 track_name_begin(&track_editor,track_named?track_title:0);
 track_page=2u;
}
/* Begin input quarantine BEFORE drawing the flash status screen. The ISR may
 * still scan inputs during a slow NOR operation, but no resulting input is
 * ever dispatched as an application command. */
static void beetle_storage_input_lock(void){
 beetle_io_input_begin(&track_io_input_guard,etch_ms);
 beetle_load_input_arm(&track_load_input_guard,etch_ms);
 track_sel_tracker_ready=0u;
 native_edit_keys_blocked=1u;
}
static void track_complete_save(void){
 if(track_new_project_full){
  /* All 12 slots occupied or flash ownership unknown. Never auto-overwrite. */
  track_storage_result=TRACK_JOURNAL_CAPACITY;track_show_storage_full(1u);track_page=8u;return;
 }
 /* A synchronous SAVE blocks input polling (including VERIFY). Catch any SEL
  * edge produced while it runs, even when returning directly to TRACKER.
  * Also protect error/overwrite/provisioning paths. */
 track_storage_just_initialized=0u;
 if(track_state.transport.playing||native_safe||native_tripped||native_updating){
  track_storage_result=TRACK_JOURNAL_HARDWARE_LOCKED;track_page=8u;return;
 }
 track_name_finish(&track_editor,track_state.project_slot,track_title);
#if BEETLE_NATIVE_STORAGE
 /* Display feedback BEFORE any potentially slow flash inspection or I/O. */
 beetle_storage_input_lock();
 track_storage_busy_kind=1u;track_page=18u;screen();
 native_pause();
 beetle_storage_last_audit=beetle_hal_probe_ownership_ui();
 if(beetle_storage_last_audit==BEETLE_CLAIM_IO || beetle_storage_last_audit==BEETLE_CLAIM_INVALID){
  track_storage_result=TRACK_JOURNAL_IO;track_page=8u;native_start();return;
 }
 /* Never silently overwrite an existing project after firmware reboot. */
 if(!track_overwrite_wait && track_state.project_slot!=track_session_saved_slot && beetle_storage_last_audit==BEETLE_CLAIM_EXISTING){
  /* Preview is for prompting only. The full journal SAVE validates BOTH
   * banks and refuses foreign data before any erase, regardless of preview. */
  char peek_title[13];
  int found=beetle_project_preview_slot(track_state.project_slot,peek_title);
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
 if(track_storage_result==TRACK_JOURNAL_CAPACITY)track_show_storage_full(2u);
 track_page=track_storage_result==TRACK_JOURNAL_OK?track_save_return_page:8u;
}
/* Never guess that a used, foreign or unreadable bank is free. A complete
 * two-bank preview is read-only. A manually selected SAVE AS is separate. */
static uint8_t beetle_first_unused_slot(void){
#if BEETLE_NATIVE_STORAGE
 if(beetle_hal_probe_ownership_ui()==BEETLE_CLAIM_BLANK)return 0u;
 if(beetle_hal_probe_ownership_ui()!=BEETLE_CLAIM_EXISTING)return TRACK_PROJECTS;
 for(uint8_t slot=0u;slot<TRACK_PROJECTS;slot++){
  char name[TRACK_NAME_MAX+1u];
  int result=beetle_project_preview_slot(slot,name);
  if(result==TRACK_JOURNAL_NO_PROJECT)return slot;
  /* OK = occupied; CRC/IO/foreign errors are never free. */
 }
 return TRACK_PROJECTS;
#else
 return 0u; /* read-only firmware never writes projects */
#endif
}
static void beetle_project_new_session(void){
 uint8_t next=beetle_first_unused_slot();
 uint8_t selected=next<TRACK_PROJECTS?next:track_state.project_slot;
 track_new_project_full=(uint8_t)(next>=TRACK_PROJECTS);
 fm1_irq_off();
 track_chip_silence(&track_chip);track_drum_stop(&track_drum);
 track_init(&track_state);track_state.record_armed=1u;track_punch_rec_armed=0u;track_song_range_on=0u;track_song_rows_clip.valid=0u;beetle_mute_cancel(&track_mute_queue);beetle_live_reset();beetle_snapshots_init(&track_snapshots);track_snapshot_slot=0u;beetle_groups_init(&track_groups);track_group_slot=track_group_lane=0u;track_song_init(&track_song,&track_state);
 beetle_edit_reset(&beetle_ed);
 track_mixer_init(&track_mix);track_drum_init(&track_drum);
#if BEETLE_NATIVE_STORAGE
 memset(&beetle_staged.oneshots,0,sizeof(beetle_staged.oneshots));
 beetle_staged.oneshots.first=beetle_oneshots_first;
 beetle_staged.oneshots.second=beetle_oneshots_second;
 memset(beetle_oneshots_first,0,sizeof(beetle_oneshots_first));
 memset(beetle_oneshots_second,0,sizeof(beetle_oneshots_second));
 beetle_live_oneshots=beetle_staged.oneshots;
 track_drum_bind_oneshots(&track_drum,&beetle_live_oneshots);
#endif
 track_chip_init(&track_chip);
 for(uint8_t k=0u;k<13u;k++)track_punch_base_param[k]=track_chip.punch_param[k<5u?k:k+3u];
 track_punch_clear(&track_rec_events);
 track_ui_inst_reset();beetle_library_init(&track_library);track_library_slot=track_library_action=track_library_target=track_library_copy_source=track_library_notice=0u;
 for(uint8_t c=0;c<TRACK_SYNTH_CHANNELS;c++){track_lane_instrument[c]=c;track_input_instrument[c]=c;}
 track_input_instrument[TRACK_DRUM_CHANNEL]=0u;for(uint8_t ch=0u;ch<TRACK_CHANNELS;ch++)track_input_explicit[ch]=0u;track_edit_patch_full=0u;
 track_state.project_slot=selected;track_state.dirty=0u;
 track_live_punch_mask=0u;track_replay_punch_mask=0u;track_punch_record_reset();
 track_swing_pending_mask=0u;track_preview_mask=0u;
 for(uint8_t i=0;i<27u;i++){track_preview_channel[i]=255u;track_preview_note[i]=255u;}
 fm1_irq_enable_all();
#if defined(BEETLE_NATIVE_V2) && BEETLE_NATIVE_V2
 beetle_bd4_live_bind(beetle_v2_workspace(),sizeof(beetle_staged));
#endif

 for(uint8_t i=0;i<=TRACK_NAME_MAX;i++)track_title[i]=0u;
 track_named=0u;track_unsaved_action=0u;track_overwrite_wait=0u;track_session_saved_slot=255u;track_saved_toast_until=0u;track_save_return_page=0u;track_preset_focus_valid=0u;track_load_from_boot=0u;track_page=0u;
}
static void beetle_project_open_load_browser(void){
 track_load_from_boot=(uint8_t)(track_page==29u);
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
 int rc=beetle_project_preview_slot(slot,preview);
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
static void beetle_wipe_erase_progress(uint8_t done,uint8_t total){
 track_storage_progress_done=done;track_storage_progress_total=total;
 screen();
}
static void beetle_storage_erase_progress(uint8_t done,uint8_t total){
 track_storage_progress_done=done;track_storage_progress_total=total;
 screen();
}
static void beetle_project_load_selected(void){
 /* Fence SEL on both successful and failed LOAD; a queued confirmation press
  * must never be reinterpreted as step deletion on the return frame. */
#if BEETLE_NATIVE_STORAGE
 if(!track_state.transport.playing&&!native_safe&&!native_tripped&&!native_updating){
  beetle_storage_input_lock();
  track_storage_busy_kind=2u;track_page=18u;screen();
  native_pause();track_storage_result=beetle_project_load_slot(track_state.project_slot);native_start();
  if(track_storage_result==TRACK_JOURNAL_OK){
   track_new_project_full=0u;
   beetle_edit_reset(&beetle_ed);
  }
  track_page=track_storage_result==TRACK_JOURNAL_OK?0u:14u;
  if(track_page==14u)track_load_feedback=track_storage_result;
 }else {track_storage_result=TRACK_JOURNAL_HARDWARE_LOCKED;track_page=8u;}
#else
 track_storage_result=TRACK_JOURNAL_HARDWARE_LOCKED;track_page=8u;
#endif
 track_unsaved_action=0u;
}
#if defined(BEETLE_NATIVE_V2) && BEETLE_NATIVE_V2
/* Same translation unit: completed by beetle_sample_transfer.inc below. */
static uint8_t beetle_sample_xfer_active;
#endif
static void track_fm1_poll(void){
#if defined(BEETLE_NATIVE_V2) && BEETLE_NATIVE_V2
 /* Must run before the 18ms UI rate-limit; bounded background prefetch. */
 if(!beetle_sample_xfer_active && !track_storage_busy_kind)
  beetle_bd4_live_poll(beetle_v2_hal_read,0,fm1_irq_off,fm1_irq_enable_all);
#endif

#if ETCH_NATIVE_OTA && defined(BEETLE_NATIVE_V2) && BEETLE_NATIVE_V2
 beetle_sample_transfer_maintenance();
#endif
 /* Capture the page BEFORE any button or project-menu action can navigate.
  * SEL can be a project confirmation at entry and a Tracker press by exit. */
 const uint8_t input_entry_page=track_page;
 uint32_t now=etch_ms;
 if(now-track_last_poll<18u)return;
 track_last_poll=now;
 uint32_t buttons=fm1_in.buttons, released=0;
 uint32_t pressed=fm1_input_edges(&released);
 int32_t k1=fm1_enc_take(2),k2=fm1_enc_take(3);
 int32_t k3=fm1_enc_take(4),k4=fm1_enc_take(5),sel=fm1_enc_take(0);
 int32_t alg=fm1_enc_take(1); /* physical ALGORITHM rotary */
 int32_t preset=fm1_enc_take(6); /* Physical PRESET: note / EMPTY / STP */
 /* This must run before ALL UI actions, including the global HOME/PLAY/SAVE
  * shortcuts. Consume physical key edges and every encoder detent. Require
  * 320 ms with ALL controls released after flash finishes. */
 if(track_page==18u || track_io_input_guard.active){
  uint32_t notes=fm1_in.notes,note_edges=fm1_input_note_edges();
  uint8_t turned=(uint8_t)(!!(k1||k2||k3||k4||sel||alg||preset));
  (void)beetle_io_input_block(&track_io_input_guard,now,buttons,notes,
     pressed|released,note_edges,turned);
  if(!track_io_input_guard.active && track_page!=18u){
   native_edit_keys_blocked=0u;
   track_storage_busy_kind=0u;
  }
  return;
 }
 /* Drain input from a completed blocking project LOAD before it can
  * reach the tracker or trigger SEL/step deletion. */
 beetle_load_input_filter(&track_load_input_guard,now,buttons,&pressed,&released);
 /* Every transition through a non-tracker page or any active I/O guard
  * revokes the delete permission. Re-arm only when the TRACKER was
  * actually polled with SEL physically released (including error cases). */
 beetle_sel_rearm(&track_sel_tracker_ready,input_entry_page,
                  buttons,pressed,&track_load_input_guard);
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
 /* Reverse key finder is a modal, read-only analyzer until SEL applies
  * a candidate. No accidental edits, note triggers or project writes. */
 if(track_page==32u){
  if(k1){int32_t next=(int32_t)track_reverse_index+k1;
   if(next<0)next=0;if(next>=BEETLE_REVERSE_CANDIDATES)next=BEETLE_REVERSE_CANDIDATES-1;
   track_reverse_index=(uint8_t)next;changed=1u;}
  if(k4){beetle_reverse_analyse(&track_song,&track_state,&track_reverse_report);
   track_reverse_index=0u;changed=1u;}
  if((pressed&(1u<<3))&&track_reverse_report.events){
   const struct beetle_reverse_candidate *selected=&track_reverse_report.ranked[track_reverse_index];
   fm1_irq_off();
   track_state.harmony.root=selected->root;
   track_state.harmony.mode=selected->mode;
   track_state.dirty=1u;
   fm1_irq_enable_all();
   track_page=7u;changed=1u;
  }else if(pressed&(1u<<8)){
   track_page=7u;track_home_fired=1u;track_home_since=now;changed=1u;
  }else if(pressed&(1u<<11)){track_page=0u;track_seq_fired=1u;changed=1u;}
  if(released&(1u<<8)){track_home_fired=0u;track_home_since=0u;}
  (void)fm1_input_note_edges();
  if(changed||now-track_last_paint>=650u){screen();track_last_paint=now;}
  return;
 }
 /* Shared keyboard octave: octaves shift LIVE keyboard notes in both
  * EDIT and TRACKER, never existing pattern cells. A simultaneous OCT chord
  * continues toggling mono/poly only in EDIT. The physical recovery combo
  * at power-up remains handled before this input loop. */
 if(track_page==0u || track_page==5u){
  /* SEQ + OCT-/+ edits ONLY the selected saved synth NOTE (one octave).
   * Latch at the actual chord press, to avoid accidentally shifting the live
   * keyboard when SEQ and OCT are released in different orders. The chord
   * is valid from any tracker edit column, but not on CH6 or empty/STP. */
  if(track_page==0u && (buttons&(1u<<11)) && (buttons&3u))
   track_step_oct_held|=(uint8_t)(buttons&3u);
  if(track_step_oct_held){
   const uint8_t completed=(uint8_t)(track_step_oct_held & released & 3u);
   if(completed){
    track_seq_fired=1u; /* modifier release never navigates away */
    if(!track_state.transport.playing &&
       track_state.transport.edit_channel<TRACK_SYNTH_CHANNELS){
     const int8_t semitones=(int8_t)((completed&1u)?-12:12);
     const uint8_t ch=track_state.transport.edit_channel;
     const uint8_t a=beetle_range_on?beetle_range_anchor:track_state.transport.edit_step;
     const uint8_t b=beetle_range_on?beetle_range_end:track_state.transport.edit_step;
     fm1_irq_off();
     beetle_edit_begin(&beetle_ed,&track_song,&track_state);
     if(track_transpose_note_range(&track_state,ch,a,b,semitones)>0){
      (void)beetle_edit_commit(&beetle_ed,&track_song,&track_state);
      if(track_scroll_preview_active){
       track_chip_note_off(&track_chip,track_scroll_preview_channel,track_scroll_preview_note);
       track_scroll_preview_active=0u;
      }
      /* Preview only when a single saved note changed. */
      if(!beetle_range_on){
       const struct track_step *updated=&track_state.active.ch[ch][track_state.transport.edit_step];
       if(updated->note<TRACK_NOTE_STOP && (track_patch_live_channel(ch,updated->instrument),track_chip_note(&track_chip,ch,updated->instrument,updated->note,track_patch_volume_velocity(updated->instrument,100u)))){
        track_scroll_preview_channel=ch;track_scroll_preview_note=updated->note;
        track_scroll_preview_active=1u;track_scroll_preview_until=now+180u;
       }
      }
     }else beetle_ed.pending_valid=0u;
     fm1_irq_enable_all();changed=1u;
    }
   }
   track_step_oct_held&=(uint8_t)(buttons&3u);
   /* Reset chord state; no octave change may bleed through afterwards. */
   track_key_oct_chord.last=0u;track_key_oct_chord.combo_latched=0u;
  }
  enum etch_octave_action key_oct=track_step_oct_held?ETCH_OCT_NONE:
    etch_octave_scan(&track_key_oct_chord,buttons);
  if(key_oct==ETCH_OCT_MINUS||key_oct==ETCH_OCT_PLUS){
   const int8_t octave_delta=(int8_t)(key_oct==ETCH_OCT_MINUS?-12:12);
   uint8_t transposed=0u,octave_targets_saved=0u;
   if(track_page==0u && !track_state.transport.playing &&
      track_state.transport.edit_channel<TRACK_SYNTH_CHANNELS){
    uint8_t ch=track_state.transport.edit_channel;
    uint8_t a=beetle_range_on?beetle_range_anchor:track_state.transport.edit_step;
    uint8_t b=beetle_range_on?beetle_range_end:track_state.transport.edit_step;
    /* Never change LIVE keyboard octave while a selection is being edited,
     * even if all notes are already at pitch 0/127 and cannot shift further. */
    octave_targets_saved=beetle_range_on;
    const uint8_t lo=a<b?a:b,hi=a>b?a:b;
    for(uint8_t r=lo;r<=hi;r++)if(track_state.active.ch[ch][r].note<TRACK_NOTE_STOP){octave_targets_saved=1u;break;}
    fm1_irq_off();beetle_edit_begin(&beetle_ed,&track_song,&track_state);
    if(track_transpose_note_range(&track_state,ch,a,b,octave_delta)>0){
     transposed=1u;(void)beetle_edit_commit(&beetle_ed,&track_song,&track_state);
     if(track_scroll_preview_active){track_chip_note_off(&track_chip,track_scroll_preview_channel,track_scroll_preview_note);track_scroll_preview_active=0u;}
    }else beetle_ed.pending_valid=0u;
    fm1_irq_enable_all();
   }
   if(!transposed && !octave_targets_saved){
    native_controls.octave=beetle_keyboard_shift(native_controls.octave,
       key_oct==ETCH_OCT_MINUS?-1:1);
   }
   changed=1u;
   /* Held EDIT notes are rearticulated at the new octave on next poll. */
   if(track_page==5u && track_edit_prev_keys){
    fm1_irq_off();track_chip_silence(&track_chip);fm1_irq_enable_all();
    track_edit_prev_keys=0u;track_edit_mono_birth=0u;beetle_mono_reset(&track_mono_keys);
    for(uint8_t j=0u;j<27u;j++){track_edit_voice_key[j]=255u;track_edit_voice_note[j]=255u;}
   }
  }else if(key_oct==ETCH_OCT_TOGGLE && track_page==5u){
   track_edit_poly^=1u;
   fm1_irq_off();track_chip_silence(&track_chip);fm1_irq_enable_all();
   track_edit_prev_keys=0u;track_edit_mono_birth=0u;beetle_mono_reset(&track_mono_keys);
   for(uint8_t j=0u;j<27u;j++){track_edit_voice_key[j]=255u;track_edit_voice_note[j]=255u;}
   changed=1u;
  }
 }else { /* Never leave a half-pressed octave chord stuck across pages. */
  track_step_oct_held=0u;track_key_oct_chord.last=0u;track_key_oct_chord.combo_latched=0u;
 }
 if(track_page==29u){
  /* Only displayed after a real FM1 boot. Do not allow global shortcuts
   * or TRACKER note/step edits to consume the same confirmation edge. */
  if(k1)track_boot_menu_choice=(uint8_t)((((int32_t)track_boot_menu_choice+k1)%2+2)%2);
  if(pressed&(1u<<3)){
   beetle_load_input_arm(&track_load_input_guard,now);
   if(track_boot_menu_choice==0u)beetle_project_new_session();
   else beetle_project_open_load_browser();
  }
  if(changed||now-track_last_paint>700u){screen();track_last_paint=now;}
  return;
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
 if(track_page==37u){
  /* Modal: no notes, pattern changes or other shortcuts may leak into editor. */
  track_preview_release(0u);(void)fm1_input_note_edges();
  if(k1||k2||sel){int32_t choice=k1?k1:(k2?k2:sel);beetle_clear_lane_yes=(uint8_t)(choice>0);changed=1u;}
  if(pressed&(1u<<8)){
   beetle_clear_lane_yes=0u;track_page=0u;track_home_fired=1u;changed=1u;
  }else if(pressed&(1u<<3)){
   if(beetle_clear_lane_yes && !track_state.transport.playing){
    fm1_irq_off();
    beetle_edit_begin(&beetle_ed,&track_song,&track_state);
    if(track_clear_lane(&track_state,track_state.transport.edit_channel)==0){
     /* Clearing a lane also releases its FX2/FX3 records, including NOTE2. */
     uint8_t lane=track_state.transport.edit_channel;
     for(uint8_t row=0u;row<TRACK_STEPS;row++){
      track_extra_clear(&track_song,track_state.pattern_slot,track_extra_key(lane,row,0u));
      if(lane==TRACK_DRUM_CHANNEL)track_extra_clear(&track_song,track_state.pattern_slot,track_extra_key(lane,row,1u));
     }
     (void)beetle_edit_commit(&beetle_ed,&track_song,&track_state);
     track_state.dirty=1u;
    }else beetle_ed.pending_valid=0u;
    fm1_irq_enable_all();
    beetle_range_on=0u;
   }
   beetle_clear_lane_yes=0u;track_page=0u;changed=1u;
  }
  if(changed||now-track_last_paint>=700u){screen();track_last_paint=now;}
  return;
 }
 if(track_page==33u || track_page==34u){
  /* These menus consume every key and encoder input, including SAVE. */
  track_preview_release(0u);(void)fm1_input_note_edges();
  if(k1||k2||sel)track_wipe_yes=(uint8_t)((track_wipe_yes+((k1+k2+sel)>0?1u:1u))&1u);
  if(pressed&(1u<<8)){track_wipe_yes=0u;track_page=1u;track_home_fired=1u;}
  else if(pressed&(1u<<3)){
   if(!track_wipe_yes){track_page=1u;}
   else if(track_page==33u){track_page=34u;track_wipe_yes=0u;}
   else{
    track_wipe_yes=0u;
    beetle_storage_input_lock();
    track_page=35u;track_storage_progress_done=0u;
    track_storage_progress_total=(uint8_t)((BEETLE_FLASH_MARKER_END-BEETLE_FLASH_BASE)/BEETLE_FLASH_SECTOR);
    screen();
#if BEETLE_NATIVE_STORAGE
    native_pause();beetle_write_permit=1u;beetle_full_wipe_confirmed=1u;
    track_wipe_result=beetle_hal_wipe_projects(beetle_wipe_erase_progress);
    beetle_full_wipe_confirmed=0u;beetle_write_permit=0u;native_start();
    if(track_wipe_result==0){
     for(uint8_t s=0u;s<TRACK_PROJECTS;s++){
      track_slot_seen[s]=0u;track_load_status[s]=2u;track_load_titles[s][0]=0;
     }
     beetle_project_new_session(); /* detach previously loaded project */
     track_menu_index=0u;
    }
#else
    track_wipe_result=-1;
#endif
    beetle_load_input_arm(&track_load_input_guard,etch_ms);
    track_sel_tracker_ready=0u;
    track_page=36u;
   }
  }
  if(changed||now-track_last_paint>=800u){screen();track_last_paint=now;}
  return;
 }
 if(track_page==35u || track_page==36u){
  track_preview_release(0u);(void)fm1_input_note_edges();
  if(track_page==36u && ((pressed&(1u<<8))||(pressed&(1u<<3)))){
   track_page=1u;track_home_fired=1u;track_menu_index=8u;
  }
  if(changed||now-track_last_paint>=850u){screen();track_last_paint=now;}
  return;
 }
 if(track_page==12u){
  if(pressed&(1u<<8)){track_storage_confirm_since=0u;track_name_begin(&track_editor,track_title);track_page=2u;screen();return;}
#if BEETLE_NATIVE_STORAGE
  if(!native_safe&&!native_tripped&&!native_updating&&!track_state.transport.playing&&
     (buttons&((1u<<3)|(1u<<9)))==((1u<<3)|(1u<<9))){
   if(!track_storage_confirm_since)track_storage_confirm_since=now?now:1u;
   if(beetle_hold_finished(now,track_storage_confirm_since)){
    beetle_storage_input_lock();
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
    if(track_storage_result==TRACK_JOURNAL_CAPACITY)track_show_storage_full(2u);
 track_page=track_storage_result==TRACK_JOURNAL_OK?track_save_return_page:8u;
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
  if(pressed&(1u<<8)){track_page=track_load_from_boot?29u:1u;track_home_fired=1u;track_home_since=now;}
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
 /* Undo/Redo on EDIT, MIXER, SONG, MASTER and LIBRARY screens. OP5/6
  * are otherwise instrument audition keys, so only consume when SEQ is held.
  * The active transport must be STOPPED to avoid racing the audio ISR. */
 if((track_page==5u||track_page==6u||track_page==9u||track_page==17u||
     track_page==25u||track_page==26u) && (buttons&(1u<<11)) &&
     !track_state.transport.playing){
  uint32_t key_edges=fm1_input_note_edges();
  if(track_page==9u && (key_edges&((1u<<1)|(1u<<3)))){
   track_seq_fired=1u;
   if(key_edges&(1u<<1)){
    (void)beetle_song_rows_copy(&track_song_rows_clip,&track_song,
      track_song_range_on?track_song_range_anchor:track_song.edit_row,
      track_song_range_on?track_song_range_end:track_song.edit_row);
    track_song_range_on=0u;
   }
   if((key_edges&(1u<<3)) && track_song_rows_clip.valid){
    fm1_irq_off();
    beetle_edit_begin_bytes(&beetle_ed,BEETLE_EDIT_SONG,0u,&track_song.chain[0],BEETLE_SONG_EDIT_BYTES);
    if(beetle_song_rows_paste(&track_song_rows_clip,&track_song,track_song.edit_row)==0){
     track_state.dirty=1u;
     (void)beetle_edit_commit_bytes(&beetle_ed,BEETLE_EDIT_SONG,0u,&track_song.chain[0],BEETLE_SONG_EDIT_BYTES);
    }else beetle_ed.pending_valid=0u;
    fm1_irq_enable_all();
   }
   screen();track_last_paint=now;return;
  }
  if(key_edges&((1u<<10)|(1u<<13))){
   fm1_irq_off();(void)track_history_step((uint8_t)((key_edges&(1u<<13))!=0u));fm1_irq_enable_all();
   track_seq_fired=1u;screen();track_last_paint=now;return;
  }
 }
 /* Isolated block editor: no sound/input note changes while editing. */
 if(track_page==20u){
  if(k4){beetle_ed.first=beetle_block_bound_length((int32_t)beetle_ed.first+k4,track_state.active.length);beetle_ed.status=0u;}
  if(k1){beetle_ed.last=beetle_block_bound_length((int32_t)beetle_ed.last+k1,track_state.active.length);beetle_ed.status=0u;}
  if(k2){int32_t op=(int32_t)beetle_ed.action+k2;beetle_ed.action=(uint8_t)(op<0?0:op>=BEETLE_BLOCK_ACTIONS?BEETLE_BLOCK_ACTIONS-1:op);beetle_ed.status=0u;}
  if(k3){int32_t amount=(int32_t)beetle_ed.amount+k3;
   if(amount< -24)amount=-24;if(amount>24)amount=24;
   beetle_ed.amount=(int8_t)amount;beetle_ed.status=0u;
  }
  if(pressed&(1u<<3)){
   if(!track_state.transport.playing){
    fm1_irq_off();
    if(beetle_ed.action==5u||beetle_ed.action==6u){
     uint8_t applied=track_history_step((uint8_t)(beetle_ed.action==6u));
     beetle_ed.status=applied?(uint8_t)(beetle_ed.action==6u?3u:2u):4u;
    }else {
     (void)beetle_edit_block_execute(&beetle_ed,&track_song,&track_state);
     if(beetle_ed.status==BEETLE_EDIT_FX_POOL_FULL)track_fx_full_until=now+1800u;
    }
    fm1_irq_enable_all();
   }
   else beetle_ed.status=4u;
  }
  if(pressed&(1u<<8)){track_page=0u;track_home_fired=1u;track_home_since=now;}
  if(released&(1u<<8)){track_home_fired=0u;track_home_since=0u;}
  if(pressed&(1u<<11))track_page=0u; /* release of entry chord must not close this page */
  (void)fm1_input_note_edges();
  if(changed||now-track_last_paint>=750u){screen();track_last_paint=now;}
  return;
 }
 /* V0.59 independent velocity/gate step editor. No playback edits allowed.
  * SEQ+FX enters; SEQ/HOME exits. No legacy tracker column changes. */
 if(track_page==22u){
  if(k1)track_state.transport.edit_step=track_row_move_length(track_state.transport.edit_step,k1,track_state.active.length);
  if(k2){int32_t f=(int32_t)track_groove_focus+k2;track_groove_focus=(uint8_t)(f<0?0:f>2?2:f);}
  /* Page-local SEQ chord; do not let OP1/2 modify the tracker lane
   * clipboard, or OP3/4 overwrite all six lanes. The input is consumed
   * even during PLAY to avoid deferred edits on the next screen. */
  if(pressed&(1u<<11))track_seq_fired=0u;
  uint32_t groove_keys=fm1_input_note_edges();
  if((buttons&(1u<<11)) && (groove_keys&((1u<<1)|(1u<<3)|(1u<<5)|(1u<<8)))){
   uint8_t ch=track_state.transport.edit_channel;
   struct track_step *step=&track_state.active.ch[ch][track_state.transport.edit_step];
   track_seq_fired=1u;
   if(groove_keys&(1u<<1))beetle_groove_copy(&track_groove_clip,step);
   if(!track_state.transport.playing &&
      ((groove_keys&((1u<<3)|(1u<<5)|(1u<<8)))!=0u)){
    fm1_irq_off();beetle_edit_begin(&beetle_ed,&track_song,&track_state);
    if(groove_keys&(1u<<3)) (void)beetle_groove_paste(&track_groove_clip,step);
    if(groove_keys&(1u<<5)) (void)beetle_groove_fill(&track_state.active,ch,step->groove);
    if(groove_keys&(1u<<8)) (void)beetle_groove_clear(&track_state.active,ch);
    if(beetle_edit_commit(&beetle_ed,&track_song,&track_state))track_state.dirty=1u;
    fm1_irq_enable_all();
   }
  }
  if((k3 || (pressed&(1u<<3))) && !(buttons&(1u<<11)) && !track_state.transport.playing){
   fm1_irq_off();beetle_edit_begin(&beetle_ed,&track_song,&track_state);
   struct track_step *step=&track_state.active.ch[track_state.transport.edit_channel][track_state.transport.edit_step];
   int32_t v=track_groove_focus==0u?TRACK_GROOVE_MICRO(step):track_groove_focus==1u?TRACK_GROOVE_RATCHET(step):(int32_t)((step->groove>>5u)&3u);
   v=(pressed&(1u<<3))?(track_groove_focus==1u?1:0):v+k3;
   if(track_groove_focus==0u)track_groove_set_micro(step,(uint8_t)(v<0?0:v>7?7:v));
   else if(track_groove_focus==1u)track_groove_set_ratchet(step,(uint8_t)(v<1?1:v>4?4:v));
   else track_groove_set_probability(step,(uint8_t)(v<0?0:v>3?3:v));
   if(beetle_edit_commit(&beetle_ed,&track_song,&track_state))track_state.dirty=1u;
   fm1_irq_enable_all();
  }
  if(pressed&(1u<<8)){track_page=0u;track_home_fired=1u;track_home_since=now;}
  if(released&(1u<<11)){
   if(!track_seq_fired)track_page=0u;
   track_seq_fired=0u;
  }
  screen();track_last_paint=now;return;
 }
 if(track_page==21u){
  if(k1)track_state.transport.edit_step=track_row_move_length(track_state.transport.edit_step,k1,track_state.active.length);
  if(k2)track_dyn_focus=(uint8_t)((int32_t)track_dyn_focus+k2>0?1u:0u);
  if((k3||(pressed&(1u<<3)))&&!track_state.transport.playing){
   fm1_irq_off();beetle_edit_begin(&beetle_ed,&track_song,&track_state);
   struct track_step *s=&track_state.active.ch[track_state.transport.edit_channel][track_state.transport.edit_step];
   uint8_t before=track_dyn_focus?track_step_gate(s):(uint8_t)TRACK_DYN_VEL(s->dynamics);
   int32_t next=(pressed&(1u<<3))?0:(int32_t)before+k3;
   if(next<0)next=0;if(next>15)next=15;
   if(track_dyn_focus)track_step_set_gate(s,(uint8_t)next);
   else track_step_set_vel(s,(uint8_t)next);
   if(beetle_edit_commit(&beetle_ed,&track_song,&track_state))track_state.dirty=1u;
   fm1_irq_enable_all();changed=1u;
  }
  if(pressed&(1u<<8)){track_page=0u;track_home_fired=1u;track_home_since=now;}
  if(released&(1u<<8)){track_home_fired=0u;track_home_since=0u;}
  if(pressed&(1u<<11))track_page=0u;
  (void)fm1_input_note_edges();
  if(changed||now-track_last_paint>=650u){screen();track_last_paint=now;}
  return;
 }
 /* SEQ + SEL from MIXER opens four performance mute snapshots. Consume
  * the entire chord here before ordinary SEL toggles QUANT MUTES. */
 if(track_page==6u && (buttons&(1u<<11)) && (pressed&(1u<<3))){
  track_snapshot_slot=0u;track_seq_fired=1u;track_page=23u;
  (void)fm1_input_note_edges();screen();track_last_paint=now;return;
 }
 /* Hold SEQ and tap EDIT from synth EDIT to open instrument library. */
 if(track_page==5u && (buttons&(1u<<11)) && (pressed&(1u<<6)) &&
    track_state.transport.edit_channel<TRACK_SYNTH_CHANNELS){
  track_library_slot=track_library_action=track_library_notice=0u;
  track_library_target=track_state.active.ch[track_state.transport.edit_channel][track_state.transport.edit_step].instrument&7u;
  track_page=25u;track_seq_fired=1u;
  (void)fm1_input_note_edges();screen();track_last_paint=now;return;
 }
 /* FX from PERFORMANCE opens MUTE GROUPS without triggering step FX. */
 if(track_page==23u && (pressed&(1u<<2))){
  track_group_slot=track_group_lane=0u;track_page=24u;
  (void)fm1_input_note_edges();screen();track_last_paint=now;return;
 }
 if(track_page==24u){
  if(k1){int32_t next=(int32_t)track_group_slot+k1;
   if(next<0)next=0;if(next>=BEETLE_GROUP_SLOTS)next=BEETLE_GROUP_SLOTS-1;
   track_group_slot=(uint8_t)next;changed=1u;}
  if(k2){int32_t next=(int32_t)track_group_lane+k2;
   if(next<0)next=0;if(next>=TRACK_CHANNELS)next=TRACK_CHANNELS-1;
   track_group_lane=(uint8_t)next;changed=1u;}
  if(pressed&(1u<<3)){fm1_irq_off();
   uint8_t dirty=beetle_group_member_toggle(&track_groups,track_group_slot,track_group_lane);
   fm1_irq_enable_all();if(dirty)track_state.dirty=1u;changed=1u;}
  if(k3){fm1_irq_off();
   uint8_t dirty=beetle_group_toggle(&track_groups,track_group_slot,&track_mute_queue,&track_mix,track_state.transport.playing);
   fm1_irq_enable_all();if(dirty)track_state.dirty=1u;changed=1u;}
  if(pressed&(1u<<8) || pressed&(1u<<2)){track_page=23u;track_home_fired=1u;track_home_since=now;changed=1u;}
  else if(pressed&(1u<<7)){track_page=6u;changed=1u;}
  else if(pressed&(1u<<11)){track_page=0u;changed=1u;}
  if(released&(1u<<8)){track_home_fired=0u;track_home_since=0u;}
  (void)fm1_input_note_edges();
  if(changed||now-track_last_paint>=650u){screen();track_last_paint=now;}
  return;
 }
 if(track_page==23u){
  if(k1){int32_t next=(int32_t)track_snapshot_slot+k1;
   if(next<0)next=0;if(next>=BEETLE_SNAPSHOT_SLOTS)next=BEETLE_SNAPSHOT_SLOTS-1;
   track_snapshot_slot=(uint8_t)next;changed=1u;}
  if(k2){fm1_irq_off();uint8_t dirty=beetle_snapshot_store(&track_snapshots,track_snapshot_slot,&track_mix);
   fm1_irq_enable_all();if(dirty)track_state.dirty=1u;changed=1u;}
  if(k3 || (pressed&(1u<<3))){fm1_irq_off();
   uint8_t dirty=beetle_snapshot_recall(&track_snapshots,track_snapshot_slot,&track_mute_queue,&track_mix,track_state.transport.playing);
   fm1_irq_enable_all();if(dirty)track_state.dirty=1u;changed=1u;}
  if(pressed&(1u<<8) || pressed&(1u<<7)){
   track_page=6u;track_home_fired=1u;track_home_since=now;changed=1u;
  }else if(pressed&(1u<<11)){track_page=0u;changed=1u;}
  if(released&(1u<<8)){track_home_fired=0u;track_home_since=0u;}
  (void)fm1_input_note_edges();
  if(changed||now-track_last_paint>=650u){screen();track_last_paint=now;}
  return;
 }

 /* Instrument library operations never reach tracker note-clear or live FX.
  * All patch transfers happen under the audio IRQ lock. */
 if(track_page==25u){
  if(k1){int32_t v=(int32_t)track_library_slot+k1;if(v<0)v=0;if(v>=BEETLE_LIBRARY_SLOTS)v=BEETLE_LIBRARY_SLOTS-1;track_library_slot=(uint8_t)v;changed=1u;}
  if(k2){int32_t v=(int32_t)track_library_action+k2;track_library_action=(uint8_t)((v%4+4)%4);changed=1u;}
  if(k3){int32_t v=(int32_t)track_library_target+k3;if(v<0)v=0;if(v>3)v=3;track_library_target=(uint8_t)v;changed=1u;}
  if(k4 && track_library_action==2u){int32_t v=(int32_t)track_library_copy_source+k4;
   if(v<0)v=0;if(v>=BEETLE_LIBRARY_SLOTS)v=BEETLE_LIBRARY_SLOTS-1;track_library_copy_source=(uint8_t)v;changed=1u;}
  if(pressed&(1u<<3)){
   if(track_state.transport.playing){track_library_notice=4u;}
   else if(track_library_action==0u){
    struct track_store_instrument patch;
    track_history_begin_library(track_library_slot);
    fm1_irq_off();track_library_capture_active(track_library_target,&patch);
    track_library_notice=beetle_library_store(&track_library,track_library_slot,&patch)?1u:4u;
    fm1_irq_enable_all();track_state.dirty=1u;
    /* New patches receive a stable default name until the user renames. */
    if(track_library.name[track_library_slot][0]==0){char temp[9]="PATCH 01";
     temp[6]=(char)('0'+(track_library_slot+1u)/10u);temp[7]=(char)('0'+(track_library_slot+1u)%10u);
     beetle_library_rename(&track_library,track_library_slot,temp);}
    track_history_commit_library(track_library_slot);
   }else if(track_library_action==1u){
    struct track_store_instrument patch;
    track_library_notice=beetle_library_load(&track_library,track_library_slot,&patch)?2u:4u;
    if(track_library_notice==2u){
     track_history_begin_instrument(track_library_target,track_state.transport.edit_channel);
     fm1_irq_off();track_library_apply_active(track_library_target,&patch);fm1_irq_enable_all();
     track_type_capture(track_library_target);track_history_commit_instrument(track_library_target,track_state.transport.edit_channel);track_state.dirty=1u;}
   }else if(track_library_action==2u){
    track_history_begin_library(track_library_copy_source);
    track_library_notice=beetle_library_copy(&track_library,track_library_slot,track_library_copy_source)?3u:4u;
    track_history_commit_library(track_library_copy_source);
    if(track_library_notice==3u)track_state.dirty=1u;
   }else if(beetle_library_valid(&track_library,track_library_slot)){
    char name[BEETLE_LIBRARY_NAME+1u];
    for(uint8_t j=0u;j<BEETLE_LIBRARY_NAME;j++)name[j]=track_library.name[track_library_slot][j];
    name[BEETLE_LIBRARY_NAME]=0;track_name_begin(&track_library_name_editor,name);track_page=26u;
   }else track_library_notice=4u;
   changed=1u;
  }
  if(pressed&(1u<<8)){track_page=5u;track_home_fired=1u;track_home_since=now;changed=1u;}
  if(pressed&(1u<<11)){track_page=0u;track_seq_fired=1u;changed=1u;}
  (void)fm1_input_note_edges();if(changed||now-track_last_paint>=650u){screen();track_last_paint=now;}return;
 }
 if(track_page==26u){
  if(k1){int32_t v=(int32_t)track_library_name_editor.cursor+k1;if(v<0)v=0;if(v>=BEETLE_LIBRARY_NAME)v=BEETLE_LIBRARY_NAME-1;
   track_library_name_editor.cursor=(uint8_t)v;changed=1u;}
  if(k2){track_name_change(&track_library_name_editor,k2);changed=1u;}
  if(pressed&(1u<<3)){
   char name[TRACK_NAME_MAX+1u];track_name_finish(&track_library_name_editor,0u,name);
   track_history_begin_library(track_library_slot);
   beetle_library_rename(&track_library,track_library_slot,name);
   track_history_commit_library(track_library_slot);
   track_state.dirty=1u;track_page=25u;track_library_notice=5u;changed=1u;
  }
  if(pressed&(1u<<8)){track_page=25u;track_home_fired=1u;track_home_since=now;changed=1u;}
  (void)fm1_input_note_edges();if(changed||now-track_last_paint>=650u){screen();track_last_paint=now;}return;
 }
 /* Read the real K1 menu movement before consuming SEL. A fast turn and
  * click can appear in the same 18ms foreground poll; checking the previous
  * menu index first would select an adjacent item or accidentally LOAD. */
 if(track_page==1u && k1){
  int32_t next=(int32_t)track_menu_index+k1;
  if(next<0)next=0;if(next>8)next=8;
  track_menu_index=(uint8_t)next;k1=0;changed=1u;
 }
 /* Handle SETTINGS via the real FM1 key matrix (SEL = button 3), ahead
  * of all generic project handlers and tracker shortcuts. Always use the
  * ENTERING page so an input cannot be interpreted twice. */
 if(beetle_settings_enter(&track_settings_nav,track_page,track_menu_index,
                         buttons,pressed,released)){
  track_page=BEETLE_SETTINGS_PAGE;track_sel_tracker_ready=0u;
  changed=1u;
  (void)fm1_input_note_edges();
  screen();track_last_paint=now;
  return;
 }
 /* Settings consumes ALL input. In particular, the SEL press which opened
  * this page may still be physically held; require its full release and a
  * second press before the actual USB mode changes. HOME returns to PROJECTS. */
 if(track_page==BEETLE_SETTINGS_PAGE){
#if ETCH_NATIVE_UAC
  if(beetle_settings_toggle(&track_settings_nav,buttons,pressed,released)){
   fm1_irq_off();usb_multitrack_switch((uint8_t)!uac_multitrack);fm1_irq_enable_all();changed=1u;
  }
#else
  (void)beetle_settings_toggle(&track_settings_nav,buttons,pressed,released);
#endif
  if(pressed&(1u<<8)){track_page=1u;track_home_fired=1u;track_home_since=now;changed=1u;}
  if(released&(1u<<8)){track_home_fired=0u;track_home_since=0u;}
  if(released&(1u<<11)){track_page=0u;changed=1u;}
  (void)fm1_input_note_edges();
  if(changed||now-track_last_paint>=600u){screen();track_last_paint=now;}
  return;
 }
 /* RC hardware telemetry is read-only. Never pass its input events into
  * tracker editing, flash menus, Punch-In or the musical transport. */
 if(track_page==27u){
  if(k1){track_rc_tab=(uint8_t)(track_rc_tab^(uint8_t)(k1&1));changed=1u;}
  if(pressed&(1u<<8)){track_page=1u;track_home_fired=1u;track_home_since=now;changed=1u;}
  if(released&(1u<<8)){track_home_fired=0u;track_home_since=0u;}
  if(released&(1u<<11)){track_page=0u;changed=1u;}
  (void)fm1_input_note_edges();
  if(changed||now-track_last_paint>=500u){screen();track_last_paint=now;}
  return;
 }
 /* Credits is read-only and never forwards edits to the tracker. */
 if(track_page==28u){
  if(pressed&(1u<<8)){track_page=1u;track_home_fired=1u;track_home_since=now;changed=1u;}
  if(released&(1u<<8)){track_home_fired=0u;track_home_since=0u;}
  if(released&(1u<<11)){track_page=0u;changed=1u;}
  (void)fm1_input_note_edges();
  if(changed||now-track_last_paint>=750u){screen();track_last_paint=now;}
  return;
 }
 /* HELP never changes patterns, instrument state, slots or tempo.
  * Trap navigation here so HOME does not accidentally arm long-press guide. */
 if(track_page==19u){
  if(k1){
   track_help_offset=beetle_help_chapter_scroll(track_help_chapter,track_help_offset,k1);
   changed=1u;
  }
  if(k2){
   uint8_t chapter=beetle_help_chapter_shift(track_help_chapter,k2);
   if(chapter!=track_help_chapter){track_help_chapter=chapter;track_help_offset=0u;changed=1u;}
  }
  if(pressed&(1u<<8)){track_page=1u;track_home_fired=1u;track_home_since=now;changed=1u;}
  if(released&(1u<<8)){track_home_fired=0u;track_home_since=0u;}
  if(released&(1u<<11)){track_page=0u;changed=1u;}
  (void)fm1_input_note_edges();
  if(changed||now-track_last_paint>=750u){screen();track_last_paint=now;}
  return;
 }
 /* Punch REC must never follow the user to the Tracker or Song screen. */
 if(track_page!=3u && track_page!=4u && track_punch_rec_armed){
  fm1_irq_off();track_punch_record_finish();track_punch_rec_armed=0u;fm1_irq_enable_all();
 }
 /* Allow preview voices to release even when switching channels/pages,
  * pressing PLAY or entering REC. No stuck notes after navigation. */
 track_preview_release((track_page==0u && !track_state.transport.playing)?fm1_in.notes:0u);

 /* The sixteen white keys select sixteen independent Punch-In FX. Pressing
  * a key focuses its owning page, regardless of what is currently shown.
  * Releasing the most recent key restores the previous still-held effect. */
 if(track_page==3u || track_page==4u){
  static const uint8_t white_key_pos[16]={0u,2u,4u,6u,7u,9u,11u,12u,14u,16u,18u,19u,21u,23u,24u,26u};
  uint32_t mask=fm1_in.notes;
  if(track_page==3u){ /* FX shortcuts also work while per-step page is visible */ }
  for(uint8_t i=0u;i<16u;i++){
   uint32_t bit=1u<<white_key_pos[i];uint8_t before=(uint8_t)((track_old_note_mask&bit)!=0u);
   uint8_t after=(uint8_t)((mask&bit)!=0u);
   if(before==after)continue;
   fm1_irq_off();
   if(after){
    track_live_punch_mask|=(uint16_t)(1u<<i);
    for(uint8_t q=0u;q<track_punch_held_count;q++)if(track_punch_held_order[q]==i){
      for(uint8_t z=q;z+1u<track_punch_held_count;z++)track_punch_held_order[z]=track_punch_held_order[z+1u];track_punch_held_count--;break;}
    if(track_punch_held_count<16u)track_punch_held_order[track_punch_held_count++]=i;
   }else{
    track_live_punch_mask&=(uint16_t)~(1u<<i);
    for(uint8_t q=0u;q<track_punch_held_count;q++)if(track_punch_held_order[q]==i){
      for(uint8_t z=q;z+1u<track_punch_held_count;z++)track_punch_held_order[z]=track_punch_held_order[z+1u];track_punch_held_count--;break;}
   }
   track_punch_focus=track_punch_held_count?track_punch_held_order[track_punch_held_count-1u]:255u;
   if(track_punch_focus<16u)track_punch_page=(uint8_t)(track_punch_focus>>3u);
   if(after||track_punch_focus<16u)track_page=4u;
   track_state.punch_mask=(uint16_t)(track_live_punch_mask|track_replay_punch_mask);
   track_sync_punch_params();fm1_irq_enable_all();
   if(track_punch_rec_armed&&track_state.transport.playing&&track_audio_started){
    fm1_irq_off();
    uint8_t source=after?track_song_punch_pattern(&track_song,&track_state):track_recorded_punch_source[i];
    if(source<TRACK_PATTERNS&&(after||(track_recorded_punch_mask&(1u<<i)))){
     if(after&&!(track_recorded_punch_edited[source]&(1u<<i))){
      track_song_clear_punch_fx(&track_song,source,i);
      track_recorded_punch_edited[source]|=(uint16_t)(1u<<i);
     }
     if(track_song_record_value(&track_song,source,track_state.transport.play_step,i,after,
           after?track_punch_saved_value(i):255u)==0){
      (void)track_punch_add(&track_rec_events,track_state.transport.play_step,i,after);
      if(after){track_recorded_punch_mask|=(uint16_t)(1u<<i);track_recorded_punch_source[i]=source;}
      else{track_recorded_punch_mask&=(uint16_t)~(1u<<i);track_recorded_punch_source[i]=TRACK_PATTERNS;}
      track_state.dirty=1u;
     }
    }
    fm1_irq_enable_all();
   }
   changed=1u;
  }
  track_old_note_mask=mask;
 }else if(track_live_punch_mask){
  fm1_irq_off();track_live_punch_mask=0u;track_punch_focus=255u;track_punch_held_count=0u;
  track_state.punch_mask=track_replay_punch_mask;track_sync_punch_params();fm1_irq_enable_all();
  track_old_note_mask=0u;changed=1u;
 }

  /* ALGORITHM edits a held Punch-In and remembers its value until reconfigured.
  * Octaves/MUTE deliberately have no parameter. Otherwise ALG chooses
  * the current pattern in the TRACK screen; SELECT remains channel turn. */
 if(k1 && track_live_punch_mask){
  uint8_t fx=track_punch_focus;
  if(fx>=16u || (fx>=5u&&fx<8u)){
   fx=255u;
   for(uint8_t j=0u;j<track_punch_held_count;j++){
    uint8_t choice=track_punch_held_order[track_punch_held_count-1u-j];
    if(choice<5u||choice>=8u){fx=choice;break;}
   }
  }
  if(fx<16u&&(track_live_punch_mask&(1u<<fx))){
   uint8_t pi=fx<5u?fx:(uint8_t)(fx-3u);
   int32_t value;
   if(fx==4u){
    int32_t index=(int32_t)beetle_stutter_index(track_punch_base_param[4])+k1;
    if(index<0)index=0;if(index>6)index=6;
    value=beetle_stutter_value((uint8_t)index);
   }else value=(int32_t)track_punch_base_param[pi]+k1;
   if(value<0)value=0;if(value>100)value=100;
   fm1_irq_off();
   if(track_punch_base_param[pi]!=(uint8_t)value){
    track_punch_base_param[pi]=(uint8_t)value;
    if(track_punch_rec_armed&&track_state.transport.playing&&(track_page==3u||track_page==4u)){
     uint8_t source=(track_recorded_punch_mask&(1u<<fx))?track_recorded_punch_source[fx]:TRACK_PATTERNS;
     if(source<TRACK_PATTERNS&&track_song_record_value(&track_song,source,
            track_state.transport.play_step,fx,2u,(uint8_t)value)==0)track_state.dirty=1u;
    }
    track_state.dirty=1u;
   }
   track_sync_punch_params();fm1_irq_enable_all();changed=1u;
  }
 }

 if(alg && !track_live_punch_mask && track_page==0u &&
    (buttons&(1u<<11)) && !track_state.transport.playing){
  fm1_irq_off();beetle_edit_begin(&beetle_ed,&track_song,&track_state);
  if(track_lane_rotate(&track_state.active,track_state.transport.edit_channel,
    track_state.transport.edit_column,alg))track_state.dirty=1u;
  fm1_irq_enable_all();track_seq_fired=1u;changed=1u;alg=0;
 }
 if(alg && !track_live_punch_mask && track_page==0u && !track_state.transport.playing){
   int32_t pat=(int32_t)track_state.pattern_slot+alg;
   if(pat<0)pat=0;if(pat>=TRACK_PATTERNS)pat=TRACK_PATTERNS-1;
   fm1_irq_off();(void)track_song_switch(&track_song,&track_state,(uint8_t)pat);fm1_irq_enable_all();
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
 /* SEQ+ENV opens the independent microtiming / ratchet / probability editor.
  * Intercept before ENV's ordinary STOP-cell insertion. */
 if(track_page==0u && !track_state.transport.playing &&
    (buttons&(1u<<11)) && (pressed&(1u<<4))){
  track_groove_focus=0u;track_seq_fired=1u;track_page=22u;
  (void)fm1_input_note_edges();screen();track_last_paint=now;return;
 }
 /* SEQ+FX opens the independent per-step dynamics editor. */
 if(track_page==0u && !track_state.transport.playing &&
    (buttons&(1u<<11)) && (pressed&(1u<<2))){
  track_dyn_focus=0u;track_seq_fired=1u;track_page=21u;
  (void)fm1_input_note_edges();screen();track_last_paint=now;return;
 }
 /* SEQ+SEL opens non-destructive block editing. Capture button before
  * the tracker SEL delete handler can see it. */
 if(track_page==0u && !track_state.transport.playing &&
    (buttons&(1u<<11)) && (pressed&(1u<<3))){
  beetle_ed.first=beetle_ed.last=track_state.transport.edit_step;
  beetle_ed.action=0u;beetle_ed.status=0u;track_seq_fired=1u;track_page=20u;
  (void)fm1_input_note_edges();screen();track_last_paint=now;return;
 }
 /* SEQ is tracker/home; SEQ+OP1/OP2 copy/paste the selected pattern lane. */
 if(pressed&(1u<<11)){track_seq_since=now?now:1u;track_seq_fired=0u;}
 /* Process the SEQ modifier BEFORE the general BPM handler.  The physical
  * K4 encoder shares its delta with BPM: consume it here, including while
  * PLAY/REC forbids edits, so it can never leak through as a tempo change.
  * Keep the SEQ chord latched to prevent SEQ-up from closing a page. */
 if(beetle_pattern_length_chord(track_page,buttons,released,k4)){
  track_seq_fired=1u;
  if(!track_state.transport.playing){
   uint8_t length=beetle_pattern_length_target(track_state.active.length,k4);
   if(length!=track_state.active.length){
    fm1_irq_off();
    beetle_edit_begin(&beetle_ed,&track_song,&track_state);
    if(track_set_pattern_length(&track_state,length)==0){
     (void)beetle_edit_commit(&beetle_ed,&track_song,&track_state);
    }else beetle_ed.pending_valid=0u;
    fm1_irq_enable_all();
   }
  }
  k4=0; /* already consumed; the end-of-poll BPM path must not see it */
  changed=1u;
 }
 if((buttons&(1u<<11))&&track_seq_since&&!track_seq_fired&&now-track_seq_since>=1200u){
  track_seq_fired=1u; /* block unwanted short-press when using chord */
 }
 if(released&(1u<<11)){
  if(!track_seq_fired){track_page=0u;changed=1u;}
  track_seq_since=0u;track_seq_fired=0u;/* Copy persists across SEQ releases and pattern switches. */
 }
 if(pressed&(1u<<9)){
  if(track_page==2u)track_complete_save();else if(track_named){track_save_return_page=track_page;track_name_begin(&track_editor,track_title);track_complete_save();}else track_begin_save();
  changed=1u;
  if(track_io_input_guard.active)return;
 }
 if(track_page==11u){
  if(pressed&(1u<<3)){track_overwrite_wait=1u;track_complete_save();changed=1u;}
  if(track_io_input_guard.active)return;
 }else if(track_page==2u){
  if(k1)track_name_move(&track_editor,k1);
  if(k2)track_name_change(&track_editor,k2);
  if(k4){
   int32_t slot=(int32_t)track_state.project_slot+k4;
   if(slot<0)slot=0;if(slot>=TRACK_PROJECTS)slot=TRACK_PROJECTS-1;
   track_state.project_slot=(uint8_t)slot;
   track_new_project_full=0u; /* explicitly chosen overwrite via SAVE AS */
  }
 }else{
  if(pressed&(1u<<2)){
    if(track_page!=3u){
     track_fx_slot=0u;
     if(track_state.transport.edit_channel==TRACK_DRUM_CHANNEL)
      track_fx_drum_lane=(uint8_t)(track_state.transport.edit_column==1u);
    }
    if(track_page==3u){track_page=4u;track_punch_page=0u;}
    else if(track_page==4u){if(track_punch_page)track_page=3u;else track_punch_page=1u;}
    else{track_page=3u;track_punch_page=0u;}
   }
  if(pressed&(1u<<6)){if(track_page!=5u)track_edit_focus_step(track_state.transport.edit_channel);track_page=(uint8_t)(track_state.transport.edit_channel==TRACK_DRUM_CHANNEL?16u:(track_page==5u?16u:5u));track_osc_cursor=0u;track_osc_bank=0u;track_type_queued=0;}
  if(pressed&(1u<<5)){track_page=17u;track_osc_cursor=0u;} /* LFO => MASTER */
  if(pressed&(1u<<10)){
   /* Live Mixer can be entered only when SONG is enabled, never just
    * because the Song editor is visible. ARP while SONG OFF stays here. */
   if(track_page==9u){if(track_song.enabled)track_page=30u;}
   else if(track_page==30u){track_page=9u;}
   else {
    /* The currently edited pattern is the live source until it is captured.
     * Refresh it before showing auto-empty Song Chain lane labels. */
    fm1_irq_off();track_song_capture(&track_song,&track_state);fm1_irq_enable_all();
    track_page=9u;
   }
   changed=1u;
  }
  /* GLO remains the dedicated conventional MIXER shortcut. */
  if(pressed&(1u<<7))track_page=(track_page==6u)?0u:6u;
  if(pressed&(1u<<13)){track_rec_since=now?now:1u;track_rec_long=0u;}
  if(released&(1u<<13)){
   if(track_page==3u || track_page==4u){
    fm1_irq_off();track_punch_rec_armed^=1u;
    if(!track_punch_rec_armed){track_punch_record_finish();track_live_punch_mask=0u;track_punch_held_count=0u;track_punch_focus=255u;
      track_state.punch_mask=track_replay_punch_mask;}
    else track_punch_record_reset();
    fm1_irq_enable_all();track_old_note_mask=fm1_in.notes;
   }else{
    /* REC ON = physical notes are entered into tracker, OFF = audition only. */
    track_state.record_armed^=1u;
   }
   track_rec_since=0u;track_rec_long=0u;changed=1u;
  }
  if(pressed&(1u<<12)){
   fm1_irq_off();
   if(track_state.transport.playing){
    beetle_mute_cancel(&track_mute_queue);track_live_mutes.pending=track_live_mutes.pending_values=0u;track_live_solos.pending=track_live_solos.pending_values=0u;
    track_punch_record_finish();track_stop(&track_state);
    track_live_punch_mask=0u;track_replay_punch_mask=0u;track_state.punch_mask=0u;track_sync_punch_params();
    if(track_song_follow_active){track_song.edit_row=track_song_preplay_row;track_song_follow_active=0u;}
   }else{
    if(track_song.enabled){
     track_song_preplay_row=track_song.edit_row;
     track_song_follow_active=1u;
     track_song_start(&track_song,&track_state);
     track_song.edit_row=track_song.position;
    }
    /* Start with no previous physical down events; only newly struck keys record. */
    track_punch_record_reset();track_old_note_mask=fm1_in.notes;
    track_live_punch_mask=0u;track_replay_punch_mask=0u;track_state.punch_mask=0u;
    track_play(&track_state);
   }
   fm1_irq_enable_all();
  }
  if(track_scroll_preview_active && ((int32_t)(now-track_scroll_preview_until)>=0 || track_page!=0u || track_state.transport.playing)){
  fm1_irq_off();track_chip_note_off(&track_chip,track_scroll_preview_channel,track_scroll_preview_note);
  track_scroll_preview_active=0u;fm1_irq_enable_all();
 }
 if(track_page==0u){
   if((pressed&(1u<<3)) && track_page==0u && !track_state.transport.playing){
    beetle_sel_defer=track_sel_tracker_ready;
    beetle_sel_was_range=0u;
    beetle_sel_delete_range=beetle_range_on;
    /* SEL tap with an existing range deletes it; SEL+K1 starts a NEW range.
     * Do not discard the old selection until we know which gesture it is. */
    if(!beetle_sel_delete_range)
     beetle_step_selection_arm(&beetle_range_on,&beetle_range_anchor,
                               &beetle_range_end,track_state.transport.edit_step);
   }
   if((buttons&(1u<<3)) && k1 && !track_state.transport.playing){
    /* Both anchor and endpoint are part of the copied selection. */
    uint8_t next=track_row_move_length(track_state.transport.edit_step,k1,track_state.active.length);
    if(beetle_sel_delete_range){
     beetle_step_selection_arm(&beetle_range_on,&beetle_range_anchor,
                               &beetle_range_end,track_state.transport.edit_step);
     beetle_sel_delete_range=0u;
    }
    beetle_step_selection_extend(&beetle_range_on,&beetle_range_end,next);
    beetle_sel_was_range=1u;
    track_state.transport.edit_step=next;k1=0;changed=1u;
   }
   const uint8_t can_delete_step=(uint8_t)(
     !track_state.transport.playing && (released&(1u<<3)) && beetle_sel_defer && !beetle_sel_was_range &&
     !(track_load_input_guard.active));
   const uint8_t can_delete_range=(uint8_t)(can_delete_step && beetle_sel_delete_range && beetle_range_on);
   if(released&(1u<<3)){beetle_sel_defer=0u;beetle_sel_was_range=0u;beetle_sel_delete_range=0u;}
   fm1_irq_off();
   if(!track_state.transport.playing &&
     (k3||preset||can_delete_step||(pressed&(1u<<4))||
      (fm1_in.notes && !(buttons&(1u<<11)))))
     beetle_edit_begin(&beetle_ed,&track_song,&track_state);
   if(can_delete_step){
    if(can_delete_range){
     (void)track_clear_step_range(&track_state,track_state.transport.edit_channel,
                                beetle_range_anchor,beetle_range_end);
    }else (void)track_clear_step(&track_state,track_state.transport.edit_channel,track_state.transport.edit_step);
    {uint8_t lo=can_delete_range?(beetle_range_anchor<beetle_range_end?beetle_range_anchor:beetle_range_end):track_state.transport.edit_step;
     uint8_t hi=can_delete_range?(beetle_range_anchor>beetle_range_end?beetle_range_anchor:beetle_range_end):track_state.transport.edit_step;
     for(uint8_t row=lo;row<=hi && row<TRACK_STEPS;row++){
      uint8_t key=track_extra_key(track_state.transport.edit_channel,row,0u);
      track_extra_clear(&track_song,track_state.pattern_slot,key);
      if(track_state.transport.edit_channel==TRACK_DRUM_CHANNEL)
       track_extra_clear(&track_song,track_state.pattern_slot,track_extra_key(TRACK_DRUM_CHANNEL,row,1u));
     }
    }
    beetle_range_on=0u;
    changed=1u;
   }
   if(k1 && !track_live_punch_mask)track_state.transport.edit_step=track_row_move_length(track_state.transport.edit_step,k1,track_state.active.length);
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
    if(track_state.transport.edit_column==1u && track_state.transport.edit_channel<TRACK_SYNTH_CHANNELS)
     track_input_instrument[track_state.transport.edit_channel]=
      (uint8_t)track_state.active.ch[track_state.transport.edit_channel][track_state.transport.edit_step].instrument;
    if(track_state.transport.edit_column==1u&&track_state.transport.edit_channel<TRACK_SYNTH_CHANNELS)track_input_explicit[track_state.transport.edit_channel]=1u;
    if(audition){
     uint8_t lane=track_state.transport.edit_channel;
     const struct track_step *st=&track_state.active.ch[lane][track_state.transport.edit_step];
     if(lane==TRACK_DRUM_CHANNEL){if(st->note<TRACK_DRUM_NOTE_COUNT)track_drum_trigger(&track_drum,st->note);}
     else if(st->note<128u){
      if((track_patch_live_channel(lane,st->instrument),
             track_chip_note(&track_chip,lane,st->instrument&7u,st->note,track_patch_volume_velocity(st->instrument,100u)))){
       track_scroll_preview_channel=lane;track_scroll_preview_note=st->note;
       track_scroll_preview_active=1u;track_scroll_preview_until=now+155u;
      }
     }
    }
   }
   if(preset && (track_state.transport.edit_column==0u ||
           (track_state.transport.edit_channel==TRACK_DRUM_CHANNEL&&track_state.transport.edit_column==1u))){
    uint8_t ch=track_state.transport.edit_channel,row=track_state.transport.edit_step;
    uint8_t col=track_state.transport.edit_column,pat=track_state.pattern_slot;
    struct track_step *st=&track_state.active.ch[ch][row];
    uint8_t is_chord=(uint8_t)(ch==TRACK_DRUM_CHANNEL && col==1u);
    uint8_t value=is_chord?st->chord:st->note;
    const uint8_t empty=(ch==TRACK_DRUM_CHANNEL&&col==1u)?0u:TRACK_NOTE_EMPTY;
    const uint8_t stop=(ch==TRACK_DRUM_CHANNEL&&col==1u)?15u:TRACK_NOTE_STOP;
    if(!track_preset_focus_valid || track_preset_focus_pattern!=pat ||
       track_preset_focus_channel!=ch || track_preset_focus_step!=row ||
       track_preset_focus_column!=col){
      track_preset_focus_valid=1u;track_preset_focus_pattern=pat;
      track_preset_focus_channel=ch;track_preset_focus_step=row;
      track_preset_focus_column=col;track_preset_focus_original=255u;
    }
    if(value!=empty && value!=stop)
     track_preset_focus_original=value;
    uint8_t original=track_preset_focus_original;
    /* An originally empty step has just EMPTY and STP; no phantom
     * note inherited from another cell or another pattern. */
    uint8_t has_note=(uint8_t)(original!=255u);
    int32_t idx=(value==empty)?(has_note?1:0):
       (value==stop)?(has_note?2:1):0;
    int32_t count=has_note?3:2;
    idx=((idx+(preset%count))%count+count)%count;
    value=has_note?(idx==0?original:idx==1?empty:stop):(idx==0?empty:stop);
    if(is_chord)st->chord=value;else st->note=value;
    track_state.dirty=1u;changed=1u;
   }
   if(pressed&(1u<<4)){ /* ENV: STOP only the focused NOTE cell. */
    struct track_step *st=&track_state.active.ch[track_state.transport.edit_channel][track_state.transport.edit_step];
    beetle_stop_focused_note(st,track_state.transport.edit_channel,track_state.transport.edit_column);
    track_state.dirty=1u;changed=1u;
   }
   if(sel){int32_t target=(int32_t)track_state.transport.edit_channel+sel;
    if(target<0){track_page=10u;track_punch_edit_row=track_state.transport.edit_step;track_punch_edit_index=0u;}
    else {if(target>=TRACK_CHANNELS)target=TRACK_CHANNELS-1u;track_state.transport.edit_channel=(uint8_t)target;}
   }
   /* OCT-/+ alone shifts the live keyboard. SEQ+OCT-/+ transposes the
    * selected saved synth note with short preview and undo support. */
   fm1_irq_enable_all();
   /* Physical keyboard: NOTE entry advances exactly one step (15 -> 00).
    * NOTE2 on the drum channel changes only the second hit, no advance.
    * K3 edits still stay on the selected row for precise pitch adjustment. */
   /* SEQ+OP1 copies, SEQ+OP2 pastes a full pattern lane, regardless of
    * selected editor column; no keyboard note gets accidentally entered. */
   if((buttons&(1u<<11))){
    uint32_t combo_edges=fm1_input_note_edges();
    if(beetle_clear_lane_chord((uint8_t)(track_page==0u),1u,
        track_state.transport.playing,0u,combo_edges)){
     beetle_clear_lane_yes=0u;track_page=37u;track_seq_fired=1u;
     track_preview_release(0u);
     screen();track_last_paint=now;return;
    }
    uint8_t lane=track_state.transport.edit_channel;
    if(combo_edges&(1u<<1u)){
     fm1_irq_off();if(beetle_range_on){
      uint8_t a=beetle_range_anchor,b=beetle_range_end;
      if(a>b){uint8_t tmp=a;a=b;b=tmp;}
      (void)track_lane_clip_copy_range(&beetle_lane_clip,&track_state.active,lane,a,b);
      beetle_lane_fx_capture(lane,a,b);
     }else{
      (void)track_lane_clip_copy(&beetle_lane_clip,&track_state.active,lane);
      beetle_lane_fx_capture(lane,0u,TRACK_STEPS-1u);
     }
     fm1_irq_enable_all();beetle_lane_clip.valid=1u;beetle_range_on=0u;track_seq_fired=1u;changed=1u;
    }
    if((combo_edges&(1u<<3u)) && beetle_lane_clip.valid && !track_state.transport.playing){
     fm1_irq_off();
     uint8_t dest=(uint8_t)(beetle_lane_clip.count==TRACK_STEPS?0u:track_state.transport.edit_step);
     uint8_t count=beetle_lane_clip.count;
     if(count>TRACK_STEPS-dest)count=(uint8_t)(TRACK_STEPS-dest);
     if(beetle_lane_fx_can_paste(lane,dest,count)){
      beetle_edit_begin(&beetle_ed,&track_song,&track_state);
      (void)(beetle_lane_clip.count==TRACK_STEPS?
        track_lane_clip_paste(&beetle_lane_clip,&track_state.active,lane):
        track_lane_clip_paste_at(&beetle_lane_clip,&track_state.active,lane,dest));
      beetle_lane_fx_paste(lane,dest,count);
      (void)beetle_edit_commit(&beetle_ed,&track_song,&track_state);
      track_state.dirty=1u;changed=1u;
     }else track_fx_full_until=etch_ms+1800u;
     fm1_irq_enable_all();track_seq_fired=1u;
    }
    if(combo_edges&(1u<<5u)){
     fm1_irq_off();beetle_edit_copy_pattern(&beetle_ed,&track_song,&track_state);fm1_irq_enable_all();
     track_seq_fired=1u;changed=1u;
    }
    if((combo_edges&(1u<<8u)) && !track_state.transport.playing){
     fm1_irq_off();if(beetle_edit_paste_pattern(&beetle_ed,&track_song,&track_state))changed=1u;
     else if(beetle_ed.status==BEETLE_EDIT_FX_POOL_FULL)track_fx_full_until=now+1800u;
     fm1_irq_enable_all();track_seq_fired=1u;
    }
    if((combo_edges&(1u<<10u)) && !track_state.transport.playing){
     fm1_irq_off();(void)beetle_edit_commit(&beetle_ed,&track_song,&track_state);
     if(track_history_step(0u))changed=1u;
     fm1_irq_enable_all();track_seq_fired=1u;
    }
    if((combo_edges&(1u<<13u)) && !track_state.transport.playing){
     fm1_irq_off();(void)beetle_edit_commit(&beetle_ed,&track_song,&track_state);
     if(track_history_step(1u))changed=1u;
     fm1_irq_enable_all();track_seq_fired=1u;
    }
   }
   if(!(buttons&(1u<<11))){
    uint32_t keypressed=fm1_input_note_edges();
    if(buttons&(1u<<11))keypressed=0u; /* Chords handled at page level below. */
    for(uint8_t k=0u;k<27u;k++)if(keypressed&(1u<<k)){
     const uint8_t ch=track_state.transport.edit_channel;
     const uint8_t note=(uint8_t)(ch==TRACK_DRUM_CHANNEL?k:
                           beetle_keyboard_pitch(k,native_controls.octave));
     fm1_irq_off();
     if(track_state.record_armed){
      const uint8_t at=track_state.transport.edit_step;
      /* CH6 NOTE2 is a separate drum hit. When NOTE2 has focus, write
       * only its 1..14 chord encoding; keep NOTE1 and step cursor intact.
       * Any other focused column retains the V0.85 NOTE1-entry behavior. */
      if(ch==TRACK_DRUM_CHANNEL && track_state.transport.edit_column==1u){
       if(note<TRACK_DRUM_COUNT)(void)track_enter_drum_note2(&track_state,note);
      }else if(track_enter_note(&track_state,note)==0 && ch<TRACK_SYNTH_CHANNELS){
       track_state.active.ch[ch][at].instrument=(uint8_t)(track_input_instrument[ch]&7u);
       track_input_explicit[ch]=1u;
      }
     }
     track_preset_last_note=note;
     /* Key audition works while STOPPED; playback itself owns voices during
      * transport. Audio ISR renders these native chip/drum voices. */
     if(!track_state.transport.playing){
      if(ch==TRACK_DRUM_CHANNEL)(void)track_drum_trigger(&track_drum,note);
      else if((track_patch_live_channel(ch,track_input_instrument[ch]),track_chip_note(&track_chip,ch,
           track_input_instrument[ch],
           note,track_patch_volume_velocity(track_input_instrument[ch],100u)))){
       track_preview_channel[k]=ch;track_preview_note[k]=note;track_preview_mask|=(1u<<k);
      }
     }
     fm1_irq_enable_all();changed=1u;
    }
   }else (void)fm1_input_note_edges();
   fm1_irq_off();(void)beetle_edit_commit(&beetle_ed,&track_song,&track_state);fm1_irq_enable_all();
  }else{
   if(track_page==5u){
    /* V1.4.10: mono EDIT note arbitration must not discard fast contact edges.
     * In old code a 45ms rejected onset still advanced prev_keys, so that
     * key could remain silent until released again. Hard silence on every
     * note also interrupted SID tails. The planner accepts every new note
     * and defers only brief key-up bounce. DSP mutations are IRQ guarded. */
    uint32_t held=fm1_in.notes;
    uint8_t ch=track_state.transport.edit_channel==TRACK_DRUM_CHANNEL?0u:track_state.transport.edit_channel;
    uint8_t inst=(uint8_t)(track_input_instrument[ch]&7u);
    if(!track_edit_poly){
     /* Reuse EDIT's per-key poly bookkeeping for mono priority and bounce age.
      * These arrays are exclusively owned by this page and mode. */
     if(!track_mono_keys.initialized)for(uint8_t k=0u;k<27u;k++){
      track_edit_voice_key[k]=0u;track_edit_voice_note[k]=0u;
     }
     struct beetle_mono_action act=beetle_mono_scan(&track_mono_keys,held,now,
                              track_edit_voice_key,track_edit_voice_note);
     if(act.kind==1u){
      uint8_t pitch=beetle_keyboard_pitch(act.key,native_controls.octave);
      fm1_irq_off();
      /* A short tail instead of killing all voices leaves no sudden reset.
       * NOTE-ON is never cancelled by a late release from another key. */
      track_chip_stop_channel(&track_chip,ch);
      track_patch_live_channel(ch,inst);
      if(track_chip_note(&track_chip,ch,inst,pitch,track_patch_volume_velocity(inst,100u))){
       track_edit_mono_birth=track_chip.voices[track_chip.last_voice_index].birth;
       track_edit_mono_pitch=pitch;track_edit_mono_channel=ch;
      }else track_edit_mono_birth=0u;
      fm1_irq_enable_all();changed=1u;
     }else if(act.kind==2u){
      fm1_irq_off();
      /* Release only the exact audition generation; never a later note in
       * a reused voice slot. This protects rapid SID retriggering. */
      for(uint8_t vi=0u;vi<track_chip.voice_count;vi++){
       struct track_chip_voice *v=&track_chip.voices[vi];
       if(v->active&&v->birth==track_edit_mono_birth &&
          v->channel==track_edit_mono_channel && v->note==track_edit_mono_pitch)
        v->released=1u;
      }
      track_edit_mono_birth=0u;
      fm1_irq_enable_all();changed=1u;
     }
    }else{
     uint32_t onset=held&~track_edit_prev_keys,offs=track_edit_prev_keys&~held;
     /* POLY remains independent: unlike old code, no 45ms blind spot. */
     for(uint8_t k=0u;k<27u;k++)if(offs&(1u<<k)){
      fm1_irq_off();
      if(track_edit_voice_key[k]<TRACK_CHIP_VOICES){
       const struct track_chip_voice *v=&track_chip.voices[track_edit_voice_key[k]];
       if(v->active&&v->note==track_edit_voice_note[k])
        track_chip_note_off(&track_chip,v->channel,track_edit_voice_note[k]);
      }
      fm1_irq_enable_all();
      track_edit_voice_key[k]=255u;track_edit_voice_note[k]=255u;
     }
     for(uint8_t k=0u;k<27u;k++)if(onset&(1u<<k)){
      uint8_t pitch=beetle_keyboard_pitch(k,native_controls.octave);
      fm1_irq_off();
      track_patch_live_channel(ch,inst);
      if(track_chip_note(&track_chip,ch,inst,pitch,track_patch_volume_velocity(inst,100u))){
       track_edit_voice_key[k]=track_chip.last_voice_index;
       track_edit_voice_note[k]=pitch;
      }
      fm1_irq_enable_all();
     }
     if(onset||offs)changed=1u;
    }
    track_edit_prev_keys=held;
    (void)fm1_input_note_edges();
   }else{
    /* V0.80 DRUM ENGINE EDIT: every physical key auditions the currently
     * selected component, including while REC was armed elsewhere. These
     * are onset edges only and NEVER insert/clear a tracker step. Audio
     * IRQ ownership is respected; the normal drum voice pool is used. */
    if(track_page==16u){
     uint32_t pressed_notes=fm1_input_note_edges();
     if(pressed_notes){
      fm1_irq_off();
      for(uint8_t k=0u;k<27u;k++)if(pressed_notes&(1u<<k))
       (void)track_drum_trigger(&track_drum,track_drum_component);
      fm1_irq_enable_all();changed=1u;
     }
    }
    if(track_edit_prev_keys || track_mono_keys.stable || track_edit_mono_birth){
     fm1_irq_off();track_chip_silence(&track_chip);fm1_irq_enable_all();
     track_edit_prev_keys=0u;track_edit_mono_birth=0u;beetle_mono_reset(&track_mono_keys);
     for(uint8_t j=0u;j<27u;j++){track_edit_voice_key[j]=255u;track_edit_voice_note[j]=255u;}}
    if(track_page==6u || (track_page==30u && track_song.enabled))mixer_key_pressed=fm1_input_note_edges();
    else if(track_page!=16u)(void)fm1_input_note_edges();
   }
   if(k4&&track_page==9u){fm1_irq_off();track_set_bpm(&track_state,(uint16_t)((int32_t)track_state.transport.bpm+k4));fm1_irq_enable_all();}
   if(track_page==9u){
    if(!track_state.transport.playing&&(k3||(pressed&(1u|2u)) || (released&(1u<<3))))
     (void)beetle_edit_begin_bytes(&beetle_ed,BEETLE_EDIT_SONG,0u,
       &track_song.chain[0],BEETLE_SONG_EDIT_BYTES);
    if((pressed&(1u<<3)) && !track_state.transport.playing){
     track_song_range_anchor=track_song.edit_row;
     track_song_range_end=track_song.edit_row;
     track_song_sel_was_range=0u;
    }
    if(k1){int32_t row=(int32_t)track_song.edit_row+k1;
     if(row<0)row=0;if(row>=track_song.count)row=track_song.count-1u;
     track_song.edit_row=(uint8_t)row;
     if((buttons&(1u<<3))&&!track_state.transport.playing){
      track_song_range_on=1u;track_song_sel_was_range=1u;
      track_song_range_end=track_song.edit_row;
     }
    }
    if(k2){int32_t col=(int32_t)track_song.edit_column+k2;
     if(col<0)col=0;if(col>6)col=6;track_song.edit_column=(uint8_t)col;}
    if(k3){fm1_irq_off();(void)track_song_change(&track_song,track_song.edit_row,track_song.edit_column,k3);track_state.dirty=1u;fm1_irq_enable_all();}
    if(pressed&2u){fm1_irq_off();(void)track_song_insert(&track_song,track_song.edit_row);track_state.dirty=1u;fm1_irq_enable_all();}
    if(pressed&1u){fm1_irq_off();(void)track_song_delete(&track_song,track_song.edit_row);track_state.dirty=1u;fm1_irq_enable_all();}
    if((released&(1u<<3)) && !track_song_sel_was_range){
     fm1_irq_off();track_song.enabled^=1u;
     if(!track_song.enabled){track_song_restore(&track_song,&track_state);beetle_live_reset();}
     fm1_irq_enable_all();changed=1u;
    }
    if(released&(1u<<3))track_song_sel_was_range=0u;
    if(!track_state.transport.playing)
     (void)beetle_edit_commit_bytes(&beetle_ed,BEETLE_EDIT_SONG,0u,
       &track_song.chain[0],BEETLE_SONG_EDIT_BYTES);
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
    if(sel&&track_state.transport.edit_channel==TRACK_DRUM_CHANNEL)
     track_fx_drum_lane=(uint8_t)(((int32_t)track_fx_drum_lane+sel)%2+2)%2;
    /* K1 selects FX1/FX2/FX3, K2 changes the chosen effect,
     * K3 changes its value. All three slots are selectable immediately. */
    if(k1){int32_t row=(int32_t)track_fx_slot+k1;
     track_fx_slot=(uint8_t)(row<0?0:row>2?2:row);}
    /* OCT buttons operate the selected FX slot ONLY on this page. */
    if(pressed&1u){
     if(!track_state.transport.playing){
      fm1_irq_off();beetle_edit_begin(&beetle_ed,&track_song,&track_state);
      const uint8_t lane=track_state.transport.edit_channel,row=track_state.transport.edit_step;
      uint8_t key=track_extra_key(lane,row,(uint8_t)(lane==TRACK_DRUM_CHANNEL&&track_fx_drum_lane));
      struct track_step *st=&track_state.active.ch[lane][row];
      struct track_note2_fx *f2=&track_state.active.note2_fx[row];
      uint8_t use2=(uint8_t)(lane==TRACK_DRUM_CHANNEL&&track_fx_drum_lane);
      if(!track_fx_slot){
       if(use2){f2->effect=0u;f2->value=50u;}
       else{st->effect=0u;st->value=50u;}
      }else track_extra_remove(&track_song,track_state.pattern_slot,key,(uint8_t)(track_fx_slot-1u));
      (void)beetle_edit_commit(&beetle_ed,&track_song,&track_state);
      fm1_irq_enable_all();track_state.dirty=1u;
     }
    }
    if(pressed&2u)track_fx_slot=(uint8_t)((track_fx_slot+1u)%3u);
    if((k2||k3) && !track_state.transport.playing){
     fm1_irq_off();beetle_edit_begin(&beetle_ed,&track_song,&track_state);
     uint8_t lane=track_state.transport.edit_channel,row=track_state.transport.edit_step;
     struct track_step *st=&track_state.active.ch[lane][row];
     struct track_note2_fx *f2=&track_state.active.note2_fx[row];
     uint8_t use2=(uint8_t)(lane==TRACK_DRUM_CHANNEL&&track_fx_drum_lane);
     uint8_t key=track_extra_key(lane,row,use2);
     uint8_t primary=use2?f2->effect:st->effect;
     struct track_extra_fx *ex=track_fx_slot?track_extra_get(&track_song,track_state.pattern_slot,key,(uint8_t)(track_fx_slot-1u)):0;
     uint8_t oldfx=track_fx_slot?track_extra_effect(ex):primary;
     uint8_t oldval=track_fx_slot?(ex?ex->value:50u):(use2?f2->value:st->value);
     if(k2){
      int32_t v=(int32_t)oldfx+k2;
      uint8_t next=(uint8_t)((v%TRACK_FX_COUNT+TRACK_FX_COUNT)%TRACK_FX_COUNT);
      if(next==TRACK_FX_VOLUME&&oldfx!=TRACK_FX_VOLUME)oldval=100u;
      if(next==TRACK_FX_DRUM_PITCH&&oldfx!=TRACK_FX_DRUM_PITCH)oldval=12u;
      if(!track_fx_slot){
       if(use2){f2->effect=next;f2->value=oldval;}else{st->effect=next;st->value=oldval;}
       /* FX2/FX3 stay independent when FX1 is cleared. */
      }else if(ex){
       if(next){ex->effect=(uint8_t)(next|((track_fx_slot-1u)<<7u));ex->value=oldval;}
       else track_extra_remove(&track_song,track_state.pattern_slot,key,(uint8_t)(track_fx_slot-1u));
      }else if(next){
       int rc=track_extra_add(&track_song,track_state.pattern_slot,key,(uint8_t)(track_fx_slot-1u),next,oldval);
       if(rc==-1)track_fx_full_until=etch_ms+1800u;
      } /* An empty FX1 is not a full storage pool. */
     }else if(k3 && (track_fx_slot==0u || ex)){
      int32_t v=(int32_t)oldval+k3;
      if(v<0)v=0;if(v>(oldfx==TRACK_FX_DRUM_PITCH?24:100))v=oldfx==TRACK_FX_DRUM_PITCH?24:100;
      if(ex)ex->value=(uint8_t)v;
      else if(use2)f2->value=(uint8_t)v;else st->value=(uint8_t)v;
     }
     if(beetle_edit_commit(&beetle_ed,&track_song,&track_state))track_state.dirty=1u;
     fm1_irq_enable_all();
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
    if(k3){
     uint8_t hist_ch=track_state.transport.edit_channel;
     if(!track_edit_prepare_patch(hist_ch)){k3=0;changed=1u;}
     if(k3){
     uint8_t hist_id=(uint8_t)(track_input_instrument[hist_ch]&7u);
     track_history_begin_instrument(hist_id,hist_ch);
     fm1_irq_off();
     uint8_t ch=hist_ch;
     struct track_instrument_ui_values *ui=&track_ui_inst[hist_id];
     int32_t v=0;
     if(track_osc_bank){
      struct track_va_patch *p=&track_chip.patches[hist_id];
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
     }else if(track_osc_cursor==0u){v=(int32_t)track_chip.osc_type[hist_id]+k3;
      track_type_switch(hist_id,(uint8_t)(((v%(int32_t)TRACK_CHIP_TYPES)+(int32_t)TRACK_CHIP_TYPES)%(int32_t)TRACK_CHIP_TYPES));
     }else if(track_osc_cursor>=1u&&track_osc_cursor<=4u){
      uint8_t *ptr=track_osc_cursor==1u?&ui->attack:track_osc_cursor==2u?&ui->decay:
       track_osc_cursor==3u?&ui->sustain:&ui->release;
      v=(int32_t)*ptr+k3;if(v<0)v=0;if(v>99)v=99;*ptr=(uint8_t)v;
      if(track_osc_cursor==1u)track_chip.env_attack[hist_id]=(uint8_t)v;
      else if(track_osc_cursor==2u)track_chip.env_decay[hist_id]=(uint8_t)v;
      else if(track_osc_cursor==3u)track_chip.env_sustain[hist_id]=(uint8_t)v;
      else track_chip.env_release[hist_id]=(uint8_t)v;
     }else if(track_osc_cursor==5u){
      v=(int32_t)ui->duty+k3;if(v<1)v=1;if(v>99)v=99;ui->duty=(uint8_t)v;
      track_chip.duty[hist_id]=(uint8_t)(v<19?0:v<38?1:v<63?2:3);
     }else if(track_osc_cursor==6u){
      v=(int32_t)ui->cutoff+k3*100;if(v<100)v=100;if(v>18000)v=18000;ui->cutoff=(uint16_t)v;
      track_chip.cutoff[hist_id]=(uint8_t)(v/75u>250u?250u:v/75u);
     }else if(track_osc_cursor==7u){
      v=(int32_t)ui->volume+k3;if(v<0)v=0;if(v>100)v=100;ui->volume=(uint8_t)v;
      /* Patch volume is independent of the mixer lane fader. */
     }
     /* Live edits update ALL active voices using this shared patch; waveform,
      * tuning, FM and ADSR changes never require a fresh keypress. */
     track_chip_refresh_instrument(&track_chip,hist_id);
     track_type_capture(hist_id);
     track_state.dirty=1u;fm1_irq_enable_all();
     track_history_commit_instrument(hist_id,hist_ch);}
    }
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
    if(!track_state.transport.playing&&(k1||k2||k3||k4||(pressed&(1u<<3))||(k4&&(buttons&(1u<<11)))))
     (void)beetle_edit_begin_bytes(&beetle_ed,BEETLE_EDIT_MASTER,0u,
       &track_master_cfg,sizeof(track_master_cfg));
    if(sel){int32_t v=(int32_t)track_master_focus+sel;if(v<0)v=0;if(v>2)v=2;track_master_focus=(uint8_t)v;}
    uint8_t fx=track_master_cfg.order[track_master_focus];
    if(pressed&(1u<<3)){track_master_cfg.enabled[fx]^=1u;track_state.dirty=1u;}
    if(k4 && (buttons&(1u<<11))){
      int32_t j=(int32_t)track_master_focus+(k4>0?1:-1);
      if(j>=0&&j<3){uint8_t x=track_master_cfg.order[track_master_focus];track_master_cfg.order[track_master_focus]=track_master_cfg.order[j];track_master_cfg.order[j]=x;track_master_focus=(uint8_t)j;track_state.dirty=1u;}
     }
    if(fx==0u){int32_t v;int32_t d[4]={k1,k2,k3,0};
     /* K1..K4 edit EQ bands. OCT- changes Q/width; OCT+ shifts center.
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
    if(!track_state.transport.playing)
     (void)beetle_edit_commit_bytes(&beetle_ed,BEETLE_EDIT_MASTER,0u,
       &track_master_cfg,sizeof(track_master_cfg));
   }
   if(track_page==30u && track_song.enabled){
    fm1_irq_off();
    if(pressed&1u){track_live_q_mute^=1u;
     if(!track_live_q_mute)beetle_live_commit(&track_live_mutes);changed=1u;}
    if(pressed&2u){track_live_q_solo^=1u;
     if(!track_live_q_solo)beetle_live_commit(&track_live_solos);changed=1u;}
    for(uint8_t k=0u;k<27u;k++)if(mixer_key_pressed&(1u<<k)){
     int8_t mute_lane=beetle_black_mute_lane(k);
     int8_t solo_lane=beetle_white_solo_lane(k);
     if(mute_lane>=0)beetle_live_toggle(&track_live_mutes,(uint8_t)mute_lane,
       track_mix.ch[(uint8_t)mute_lane].mute,(uint8_t)(track_state.transport.playing && track_live_q_mute));
     else if(solo_lane>=0)beetle_live_toggle(&track_live_solos,(uint8_t)solo_lane,
       track_mix.ch[(uint8_t)solo_lane].solo,(uint8_t)(track_state.transport.playing && track_live_q_solo));
     changed=1u;
    }
    fm1_irq_enable_all();
   }
   if(track_page==6u){
    if(!track_state.transport.playing && (k3||mixer_key_pressed||(pressed&(1u<<3))))
     (void)beetle_edit_begin_bytes(&beetle_ed,BEETLE_EDIT_MIXER,0u,
       &track_mix,sizeof(track_mix));
    /* Mixer SEL switches quantized mute mode; never reaches tracker CLEAR.
     * Switching OFF commits already armed changes immediately. */
    if(pressed&(1u<<3)){
     fm1_irq_off();
     track_mix.quantize_mutes^=1u;
     if(!track_mix.quantize_mutes)beetle_mute_commit(&track_mute_queue,&track_mix);
     fm1_irq_enable_all();track_state.dirty=1u;changed=1u;
    }
    if(k1||k2)track_mixer_move(&track_mix,k1,k2);
    if(k3){fm1_irq_off();
     if(track_state.transport.playing&&track_mix.quantize_mutes&&track_mix.col==2u){
      if(beetle_mute_arm(&track_mute_queue,&track_mix,track_mix.row,(uint8_t)(k3>0)))track_state.dirty=1u;
     }else if(track_mixer_change(&track_mix,k3))track_state.dirty=1u;
     fm1_irq_enable_all();}
    /* Six left-most physical black keys are direct lane-mute toggles.
     * Press edges only, not continuously held keys. Disabled for other pages. */
    if(mixer_key_pressed){
     fm1_irq_off();
     for(uint8_t k=0u;k<27u;k++)if(mixer_key_pressed&(1u<<k)){
      int8_t lane=beetle_black_mute_lane(k);
      if(lane>=0){
       if(track_state.transport.playing&&track_mix.quantize_mutes){
        if(beetle_mute_toggle(&track_mute_queue,&track_mix,(uint8_t)lane))track_state.dirty=1u;
       }else {track_mix.ch[(uint8_t)lane].mute^=1u;track_state.dirty=1u;}
      }else{
       int8_t solo_lane=beetle_white_solo_lane(k);
       if(solo_lane>=0){track_mix.ch[(uint8_t)solo_lane].solo^=1u;track_state.dirty=1u;}
      }
     }
     fm1_irq_enable_all();
    }
    if(!track_state.transport.playing)
     (void)beetle_edit_commit_bytes(&beetle_ed,BEETLE_EDIT_MIXER,0u,
       &track_mix,sizeof(track_mix));
   }
   if(track_page==1u && k4){
    int32_t v=(int32_t)track_state.project_slot+k4;
    if(v<0)v=0;if(v>=(int32_t)TRACK_PROJECTS)v=TRACK_PROJECTS-1;track_state.project_slot=(uint8_t)v;
   }
   if(track_page==1u && (pressed&(1u<<3))){
    if(track_menu_index==8u){
     track_wipe_yes=0u;track_page=33u;changed=1u;
    }else if(track_menu_index==0u){
     if(track_named){track_save_return_page=track_page;track_name_begin(&track_editor,track_title);track_complete_save();}
     else track_begin_save();
    }else if(track_menu_index==1u){
     /* SAVE AS always opens the title editor. K4 can select any of 12 slots. */
     track_overwrite_wait=0u;track_new_project_full=0u;track_begin_save();
    }else if(track_menu_index==7u){
     track_rc_tab=0u;track_page=27u;changed=1u;
    }else if(track_menu_index==6u){
     /* SETTINGS already handled above; never mistake this menu index for LOAD. */
    }else if(track_menu_index==5u){
     track_page=28u;changed=1u;
    }else if(track_menu_index==4u){
     track_help_chapter=0u;track_help_offset=0u;track_page=19u;changed=1u;
    }else if(track_menu_index==3u){
     if(track_state.dirty){track_unsaved_action=2u;track_page=13u;}
     else beetle_project_new_session();
    }else{
     if(track_state.dirty){track_unsaved_action=1u;track_page=13u;}
     else beetle_project_open_load_browser();
    }
   }
   if(track_page==7u){
    if(pressed&(1u<<3)){track_reverse_open();changed=1u;}
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
 if(k4 && track_page==0u && !(buttons&(1u<<11))){fm1_irq_off();track_set_bpm(&track_state,(uint16_t)((int32_t)track_state.transport.bpm+k4));fm1_irq_enable_all();}
 track_fm1_leds();
 /* Fast playhead path: 18 ms input tick, never coupled to the 125..380 ms
  * FULL-FRAME audio governor. Every distinct step is painted by two narrow
  * dirty strips plus a 22x23 icon, avoiding ~115 KiB of SPI transfer per
  * sixteenth note. Full repaint is required for actual content/view changes. */
 static uint8_t painted_page __attribute__((section(".pool")));
 static uint8_t painted_step __attribute__((section(".pool")));
 static uint8_t painted_playing __attribute__((section(".pool")));
 static uint8_t painted_song_pos __attribute__((section(".pool")));
 static uint8_t pending_full __attribute__((section(".pool")));
 static uint8_t paint_initialized __attribute__((section(".pool")));
 const uint8_t play_step=track_state.transport.play_step;
 const uint8_t playing=(uint8_t)(track_state.transport.playing!=0u);
 const uint8_t song_pos=track_song.position;
 const uint32_t since_paint=now-track_last_paint;
 const uint16_t dma_ui_floor=beetle_lcd_min_interval(native_ui_pressure,playing);
 const uint8_t view_transition=(uint8_t)(!paint_initialized||painted_page!=track_page||painted_playing!=playing);
 const uint8_t song_page_boundary=(uint8_t)(track_song.enabled&&song_pos!=painted_song_pos&&
      (song_pos>>3u)!=(painted_song_pos>>3u)&&track_page==9u);
 /* Keep a dirty flag across throttled ticks; never silently drop edits. */
 if(changed || track_battery_ui_dirty || view_transition || song_page_boundary ||
    (track_page==0u && playing && track_song.enabled && song_pos!=painted_song_pos))pending_full=1u;
 uint8_t do_full=(uint8_t)(view_transition ||
    (pending_full && (!playing || !dma_ui_floor || since_paint>=dma_ui_floor)) ||
    (!playing && since_paint>=750u) ||
    (playing && track_page==6u && since_paint>=100u) ||
    (playing && track_page!=0u && track_page!=9u && track_page!=6u && since_paint>=750u));
 if(do_full){
  screen();track_last_paint=now;track_battery_ui_dirty=0u;
  /* CRITICAL V1.4.5: do NOT assign transport.play_step here. A full
   * 240x240 LCD transfer can straddle a 16th-note boundary. The audio ISR
   * may already be at step N+1 while the LCD still contains the N frame.
   * Claiming N+1 is painted prevents the next delta from clearing N:
   * exactly the persistent magenta ghost seen in IMG_2437.mp4.
   * The per-frame snapshots are latched BEFORE the first SPI transfer. */
  painted_step=track_ui_step_snapshot;
  paint_initialized=1u;
  painted_song_pos=track_ui_song_pos_snapshot;
  painted_page=track_page;painted_playing=playing;pending_full=0u;
 }else if(paint_initialized && beetle_playhead_step_dirty(playing,track_page,painted_page,painted_step,play_step)){
  beetle_playhead_grid_delta(painted_step,play_step);
  painted_step=play_step;
 }else if(paint_initialized && beetle_song_fast_row_dirty(playing,track_page,painted_page,
         track_song.enabled,painted_song_pos,song_pos) && !song_page_boundary){
  beetle_playhead_song_delta(painted_song_pos,song_pos);
  painted_song_pos=song_pos;
 }
}
#endif
