from pathlib import Path
root=Path('/mnt/data/beetle_v134_final/firmware/fm1-native')
p=root/'beetle_pattern_edit.h';s=p.read_text()
s=s.replace(' struct track_song_clip punch;\n};',' struct track_song_clip punch;\n struct track_extra_bank extras; /* atomic Undo/Redo for FX1/FX2/FX3 */\n};',1)
s=s.replace(' img->pattern=eng->active;img->punch=bank->clips[eng->pattern_slot];',' img->pattern=eng->active;img->punch=bank->clips[eng->pattern_slot];img->extras=bank->extras;',1)
s=s.replace('''  for(uint32_t i=0u;i<sizeof(ed->pending->punch);i++)if(before[i]!=after[i]){changed=1u;break;}
 }
 if(!changed)return 0;''','''  for(uint32_t i=0u;i<sizeof(ed->pending->punch);i++)if(before[i]!=after[i]){changed=1u;break;}
 }
 if(!changed){
  before=(const uint8_t*)&ed->pending->extras;after=(const uint8_t*)&bank->extras;
  for(uint32_t i=0u;i<sizeof(bank->extras);i++)if(before[i]!=after[i]){changed=1u;break;}
 }
 if(!changed)return 0;''',1)
s=s.replace(' bank->clips[pat]=img->punch;eng->dirty=1u;return 1;',' bank->clips[pat]=img->punch;bank->extras=img->extras;eng->dirty=1u;return 1;',1)
needle=''' bank->clips[eng->pattern_slot]=ed->whole_clip->punch;
 if(beetle_edit_commit'''
replace=''' bank->clips[eng->pattern_slot]=ed->whole_clip->punch;
 /* Copy only FX belonging to source pattern into the target pattern.
  * Other project patterns must never be overwritten by a paste. */
 uint8_t need=0u,free=0u;
 for(uint8_t i=0u;i<ed->whole_clip->extras.count;i++)
  need+=(ed->whole_clip->extras.data[i].pattern==ed->pending_index /* destination? source below */);
 /* Source is the pattern stored with the whole-clip capture, not the destination. */
 (void)need;(void)free;
 if(beetle_edit_commit'''
# instead need proper source id from whole copy - add `whole_index` to editor, assign; capture before paste for undo.
s=s.replace(' uint8_t cursor,count,pending_valid,pending_index,pending_kind,whole_valid,block_valid,block_count;', ' uint8_t cursor,count,pending_valid,pending_index,pending_kind,whole_valid,whole_index,block_valid,block_count;',1)
s=s.replace(' beetle_edit_capture(ed->whole_clip,bank,eng);ed->whole_valid=1u;', ' beetle_edit_capture(ed->whole_clip,bank,eng);ed->whole_index=eng->pattern_slot;ed->whole_valid=1u;',1)
s=s.replace(''' bank->clips[eng->pattern_slot]=ed->whole_clip->punch;
 if(beetle_edit_commit(ed,bank,eng))''',''' bank->clips[eng->pattern_slot]=ed->whole_clip->punch;
 uint8_t previous=0u,needed=0u;
 for(uint8_t i=0u;i<bank->extras.count;i++)previous+=(bank->extras.data[i].pattern==eng->pattern_slot);
 for(uint8_t i=0u;i<ed->whole_clip->extras.count;i++)needed+=(ed->whole_clip->extras.data[i].pattern==ed->whole_index);
 if(bank->extras.count-previous+needed>TRACK_EXTRA_FX_CAP){
  /* Refuse the whole paste: no partial FX loss when storage is full. */
  eng->active=ed->pending->pattern;bank->clips[eng->pattern_slot]=ed->pending->punch;
  ed->pending_valid=0u;return 0;
 }
 for(uint8_t i=0u;i<bank->extras.count;){
  if(bank->extras.data[i].pattern==eng->pattern_slot){
   for(uint8_t j=i;j+1u<bank->extras.count;j++)bank->extras.data[j]=bank->extras.data[j+1u];
   bank->extras.count--;
  }else i++;
 }
 for(uint8_t i=0u;i<ed->whole_clip->extras.count;i++){
  const struct track_extra_fx *f=&ed->whole_clip->extras.data[i];
  if(f->pattern!=ed->whole_index)continue;
  struct track_extra_fx copy=*f;copy.pattern=eng->pattern_slot;
  bank->extras.data[bank->extras.count++]=copy;
 }
 if(beetle_edit_commit(ed,bank,eng))''',1)
p.write_text(s)
p=root/'track_runtime.h';s=p.read_text()
s=s.replace('beetle_ed __attribute__((section(".beetle_edit_scratch"),aligned(16)))','beetle_ed __attribute__((section(".beetle_usb_stems_ram"),aligned(16)))')
s=s.replace('beetle_edit_pending __attribute__((section(".beetle_edit_scratch")))','beetle_edit_pending __attribute__((section(".beetle_usb_stems_ram"),aligned(16)))')
s=s.replace('beetle_edit_block[TRACK_STEPS] __attribute__((section(".beetle_edit_scratch")))','beetle_edit_block[TRACK_STEPS] __attribute__((section(".beetle_usb_stems_ram"),aligned(16)))')
# clear step and range FX references in tracker
s=s.replace('''    beetle_range_on=0u;
    changed=1u;
   }
   if(k1''','''    {uint8_t lo=can_delete_range?(beetle_range_anchor<beetle_range_end?beetle_range_anchor:beetle_range_end):track_state.transport.edit_step;
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
   if(k1''',1)
p.write_text(s)
print('undo memory patch done')
