from pathlib import Path
p=Path('/mnt/data/beetle_v134_final/firmware/fm1-native/track_runtime.h');s=p.read_text()
def replace(old,new,count=1):
 global s
 assert s.count(old)==count,(s.count(old),old[:100]);s=s.replace(old,new)
replace('static uint8_t track_named,track_page,track_menu_index,track_fx_cursor,track_osc_cursor,track_osc_bank;', '''static uint8_t track_named,track_page,track_menu_index,track_fx_cursor,track_osc_cursor,track_osc_bank;
static uint8_t track_fx_slot; /* FX1/FX2/FX3, chosen in PER STEP FX */
static uint32_t track_fx_full_until; /* visible full-capacity overlay, no destructive writes */''')
# source helper in songmode (no ISR copies); above track_audio_fire
replace('static void track_audio_fire(struct etch_native_audio *audio,uint8_t lane_mask){', '''/* Follow the lane's SOURCE pattern in Song Mode. A composed Song row may
 * combine six patterns, so the bounded FX pool must never be merged into one. */
static const struct track_extra_fx *track_audio_extra(uint8_t lane,uint8_t row,uint8_t note2,uint8_t slot){
 const struct track_pattern *p=&track_state.active;
 if(track_song.enabled&&track_song.composite_active&&track_song.position<track_song.count){
  uint8_t src=track_song.chain[track_song.position].lane[lane];
  if(src>=TRACK_PATTERNS)return 0;
  p=&track_song.patterns[src];row=(uint8_t)(row/(p->length?p->length:16u)*(p->length?p->length:16u)+row%(p->length?p->length:16u));
  row=(uint8_t)(row%(p->length?p->length:16u));
 }
 return track_extra_find(p,track_extra_key(lane,row,note2),slot);
}
static void track_audio_fire(struct etch_native_audio *audio,uint8_t lane_mask){''')
# simplify weird expression
replace('row=(uint8_t)(row/(p->length?p->length:16u)*(p->length?p->length:16u)+row%(p->length?p->length:16u));\n  row=(uint8_t)(row%(p->length?p->length:16u));','row=(uint8_t)(row%(p->length?p->length:16u));')
replace('''    if(fx==TRACK_FX_VOLUME)hit->gain_q8=(uint16_t)((uint32_t)hit->gain_q8*value/100u);
    hit->gain_q8''','''    if(fx==TRACK_FX_VOLUME)hit->gain_q8=(uint16_t)((uint32_t)hit->gain_q8*value/100u);
    /* Additional NOTE1/NOTE2 FX are independent for each drum hit. */
    for(uint8_t slot=0u;slot<2u;slot++){
     const struct track_extra_fx *extra=track_audio_extra(TRACK_DRUM_CHANNEL,position,second_hit,slot);
     if(!extra)continue;
     if(extra->effect==TRACK_FX_DRUM_PITCH){
      int32_t pitch=(int32_t)hit->pitch_semitones+(int32_t)extra->value-12;
      if(pitch< -36)pitch=-36;if(pitch>36)pitch=36;
      hit->pitch_semitones=(int8_t)pitch;track_drum_retune_voice(&track_drum,hit);
     }else if(extra->effect==TRACK_FX_VOLUME)
      hit->gain_q8=(uint16_t)((uint32_t)hit->gain_q8*extra->value/100u);
    }
    hit->gain_q8''')
replace('''  beetle_step_fx_event(&track_chip,c,s,duty,cutoff,track_state.transport.bpm);
  if(s->note==TRACK_NOTE_STOP)''','''  beetle_step_fx_event(&track_chip,c,s,duty,cutoff,track_state.transport.bpm);
  if(s->effect && s->note!=TRACK_NOTE_STOP)
   beetle_step_fx_multi(&track_chip,c,
      track_audio_extra(c,position,0u,0u),track_audio_extra(c,position,0u,1u),
      track_state.transport.bpm);
  if(s->note==TRACK_NOTE_STOP)''')
# track step FX scroll column 3 set primary clear extras if 0
replace('''   st->effect=next;}break;''','''   st->effect=next;
   if(!next)track_extra_clear(&track_state.active,track_extra_key(track_state.transport.edit_channel,track_state.transport.edit_step,0u));}break;''')
replace('''  const char *fx=track_web_stepfx[st->effect<TRACK_FX_COUNT?st->effect:0u];
  char fx3[4]={fx[0],fx[1],fx[2],0};track_pix_cell(fx3,xs[4],y);''','''  const char *fx=track_web_stepfx[st->effect<TRACK_FX_COUNT?st->effect:0u];
  char fx3[4]={fx[0],fx[1],fx[2],0};
  if(st->effect && track_extra_count(&track_state.active,track_extra_key(track_state.transport.edit_channel,row,0u))){
   fx3[0]='M';fx3[1]='U';fx3[2]='L';
  }
  track_pix_cell(fx3,xs[4],y);''')
start=s.index('  uint8_t selected_fx=(track_state.transport.edit_channel==TRACK_DRUM_CHANNEL&&track_fx_drum_lane)?')
end=s.index('  track_pix_text("FX > PUNCH-IN",15u,200u,TRACK_WHITE);return;',start)+len('  track_pix_text("FX > PUNCH-IN",15u,200u,TRACK_WHITE);return;')
s=s[:start]+'''  const uint8_t use2=(uint8_t)(track_state.transport.edit_channel==TRACK_DRUM_CHANNEL&&track_fx_drum_lane);
  const uint8_t key=track_extra_key(track_state.transport.edit_channel,track_state.transport.edit_step,use2);
  char n[12],item[3][38];const char *items[3];
  const uint8_t primary=use2?track_state.active.note2_fx[track_state.transport.edit_step].effect:st->effect;
  const uint8_t pv=use2?track_state.active.note2_fx[track_state.transport.edit_step].value:st->value;
  for(uint8_t j=0u;j<3u;j++){
   const struct track_extra_fx *ext=j?track_extra_find(&track_state.active,key,(uint8_t)(j-1u)):0;
   const uint8_t effect=j?(ext?ext->effect:0u):primary;
   const uint8_t value=j?(ext?ext->value:50u):pv;
   const char *nm=track_web_stepfx[effect<TRACK_FX_COUNT?effect:0u];
   uint8_t at=0u;item[j][at++]='F';item[j][at++]='X';item[j][at++]=(char)('1'+j);item[j][at++]=' ';
   for(uint8_t k=0u;nm[k]&&at<24u;k++)item[j][at++]=nm[k];
   item[j][at++]=' ';track_fmt_int(n,value);
   for(uint8_t k=0u;n[k]&&at<35u;k++)item[j][at++]=n[k];item[j][at]=0;
   items[j]=item[j];
  }
  track_web_list(items,3u,track_fx_slot,80u,26u);
  track_pix_text("K1 PARAM  K3 CHANGE",15u,177u,TRACK_PINK);
  track_pix_text("OCT+ SLOT  OCT- REMOVE",15u,189u,TRACK_PINK);
  track_pix_text(use2?"K2 NOTE1/NOTE2":"K2 FX SLOT",15u,201u,TRACK_WHITE);
  char cap[30]="FX STORAGE ";track_fmt_int(n,track_state.active.extra_count);
  uint8_t at=11u;for(uint8_t k=0u;n[k]&&at<19u;k++)cap[at++]=n[k];cap[at++]='/';cap[at++]='1';cap[at++]='6';cap[at]=0;
  track_pix_text(cap,15u,214u,TRACK_PINK);
  if((int32_t)(track_fx_full_until-etch_ms)>0){
   lcd_fill(13u,95u,214u,53u,TRACK_DARK);
   lcd_fill(13u,95u,214u,2u,TRACK_PINK);
   track_pix_center("FX STORAGE FULL",120u,109u,TRACK_BRIGHT);
   track_pix_center("REMOVE FX TO FREE SPACE",120u,127u,TRACK_PINK);
  }
  return;'''+s[end:]
# remove char redefinition? char n earlier from `char line[32]...char n[4]`; now `char n[12]` same scope! change to nvalue
s=s.replace('  char n[12],item[3][38];const char *items[3];','  char nv[12],item[3][38];const char *items[3];').replace('track_fmt_int(n,value);\n   for(uint8_t k=0u;n[k]&&at<35u;k++)item[j][at++]=n[k];','track_fmt_int(nv,value);\n   for(uint8_t k=0u;nv[k]&&at<35u;k++)item[j][at++]=nv[k];').replace('track_fmt_int(n,track_state.active.extra_count);\n  uint8_t at=11u;for(uint8_t k=0u;n[k]&&at<19u;k++)cap[at++]=n[k];','track_fmt_int(nv,track_state.active.extra_count);\n  uint8_t at=11u;for(uint8_t k=0u;nv[k]&&at<19u;k++)cap[at++]=nv[k];')
# state init
replace('track_fx_drum_lane=0u;track_edit_poly=0u;','track_fx_drum_lane=0u;track_fx_slot=0u;track_fx_full_until=0u;track_edit_poly=0u;')
replace('''    if(track_page!=3u && track_state.transport.edit_channel==TRACK_DRUM_CHANNEL)
     track_fx_drum_lane=(uint8_t)(track_state.transport.edit_column==1u);
    track_page=track_page==3u?4u:3u;''','''    if(track_page!=3u){
     track_fx_slot=0u;
     if(track_state.transport.edit_channel==TRACK_DRUM_CHANNEL)
      track_fx_drum_lane=(uint8_t)(track_state.transport.edit_column==1u);
    }
    track_page=track_page==3u?4u:3u;''')
start=s.index('   if(track_page==3u){\n    if(k3&&!track_state.transport.playing)beetle_edit_begin')
end=s.index('   if(track_page==5u){',start)
s=s[:start]+'''   if(track_page==3u){
    if(k2){
     if(track_state.transport.edit_channel==TRACK_DRUM_CHANNEL)track_fx_drum_lane^=1u;
     else track_fx_slot=(uint8_t)(((int32_t)track_fx_slot+(k2%3)+3)%3);
    }
    if(k1)track_fx_cursor=(uint8_t)((int32_t)track_fx_cursor+k1>0?1u:0u);
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
       const struct track_extra_fx *first=track_extra_find(&track_state.active,key,0u);
       if(first){if(use2){f2->effect=first->effect;f2->value=first->value;}
        else{st->effect=first->effect;st->value=first->value;}
        track_extra_remove(&track_state.active,key,0u);
       }else{if(use2){f2->effect=0u;f2->value=50u;}else{st->effect=0u;st->value=50u;}}
      }else track_extra_remove(&track_state.active,key,(uint8_t)(track_fx_slot-1u));
      (void)beetle_edit_commit(&beetle_ed,&track_song,&track_state);
      fm1_irq_enable_all();track_state.dirty=1u;
     }
    }
    if(pressed&2u)track_fx_slot=(uint8_t)((track_fx_slot+1u)%3u);
    if(k3 && !track_state.transport.playing){
     fm1_irq_off();beetle_edit_begin(&beetle_ed,&track_song,&track_state);
     uint8_t lane=track_state.transport.edit_channel,row=track_state.transport.edit_step;
     struct track_step *st=&track_state.active.ch[lane][row];
     struct track_note2_fx *f2=&track_state.active.note2_fx[row];
     uint8_t use2=(uint8_t)(lane==TRACK_DRUM_CHANNEL&&track_fx_drum_lane);
     uint8_t key=track_extra_key(lane,row,use2);
     uint8_t primary=use2?f2->effect:st->effect;
     struct track_extra_fx *ex=track_fx_slot?track_extra_get(&track_state.active,key,(uint8_t)(track_fx_slot-1u)):0;
     uint8_t oldfx=track_fx_slot?(ex?ex->effect:0u):primary;
     uint8_t oldval=track_fx_slot?(ex?ex->value:50u):(use2?f2->value:st->value);
     if(track_fx_cursor==0u){
      int32_t v=(int32_t)oldfx+k3;
      uint8_t next=(uint8_t)((v%TRACK_FX_COUNT+TRACK_FX_COUNT)%TRACK_FX_COUNT);
      if(next==TRACK_FX_VOLUME&&oldfx!=TRACK_FX_VOLUME)oldval=100u;
      if(next==TRACK_FX_DRUM_PITCH&&oldfx!=TRACK_FX_DRUM_PITCH)oldval=12u;
      if(!track_fx_slot){
       if(use2){f2->effect=next;f2->value=oldval;}else{st->effect=next;st->value=oldval;}
       if(!next)track_extra_clear(&track_state.active,key);
      }else if(ex){
       if(next){ex->effect=next;ex->value=oldval;}
       else track_extra_remove(&track_state.active,key,(uint8_t)(track_fx_slot-1u));
      }else if(next && primary){
       int rc=track_extra_add(&track_state.active,key,(uint8_t)(track_fx_slot-1u),next,oldval);
       if(rc<0)track_fx_full_until=etch_ms+1800u;
      }else if(next)track_fx_full_until=etch_ms+1800u;
     }else if(track_fx_slot==0u || ex){
      int32_t v=(int32_t)oldval+k3;
      if(v<0)v=0;if(v>(oldfx==TRACK_FX_DRUM_PITCH?24:100))v=oldfx==TRACK_FX_DRUM_PITCH?24:100;
      if(ex)ex->value=(uint8_t)v;
      else if(use2)f2->value=(uint8_t)v;else st->value=(uint8_t)v;
     }
     if(beetle_edit_commit(&beetle_ed,&track_song,&track_state))track_state.dirty=1u;
     fm1_irq_enable_all();
    }
   }
'''+s[end:]
p.write_text(s)
print('runtime done')
