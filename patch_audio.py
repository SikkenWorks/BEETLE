from pathlib import Path
root=Path('/mnt/data/beetle_v134_final/firmware/fm1-native')
def patch(file,old,new,count=1):
 p=root/file;s=p.read_text();assert s.count(old)==count,(file,s.count(old),old[:40]);p.write_text(s.replace(old,new))
patch('track_chip.h',' uint8_t fx_type[TRACK_CHANNELS],fx_value[TRACK_CHANNELS];',' uint8_t fx_type[TRACK_CHANNELS],fx_value[TRACK_CHANNELS];\n uint8_t fx_extra_type[TRACK_CHANNELS][2],fx_extra_value[TRACK_CHANNELS][2];')
patch('track_chip.c',' if(s->fx_type[v->channel]==11u)pitch=(int32_t)((uint32_t)s->fx_value[v->channel]*24u/100u)-12;', ''' if(s->fx_type[v->channel]==11u)pitch=(int32_t)((uint32_t)s->fx_value[v->channel]*24u/100u)-12;
 for(uint8_t j=0u;j<2u;j++)if(s->fx_extra_type[v->channel][j]==11u)
  pitch+=(int32_t)((uint32_t)s->fx_extra_value[v->channel][j]*24u/100u)-12;''')
patch('track_chip.c',''' if(s->fx_type[v->channel]==6u){
  int32_t wave=(int32_t)track_va_sine[(uint8_t)(lfo_phase>>24u)];
  int32_t cents=(wave*(int32_t)s->fx_value[v->channel]*9)/13000;''',''' uint8_t vib=0u;
 if(s->fx_type[v->channel]==6u)vib=s->fx_value[v->channel];
 for(uint8_t j=0u;j<2u;j++)if(s->fx_extra_type[v->channel][j]==6u)vib=s->fx_extra_value[v->channel][j];
 if(vib){
  int32_t wave=(int32_t)track_va_sine[(uint8_t)(lfo_phase>>24u)];
  int32_t cents=(wave*(int32_t)vib*9)/13000;''')
patch('track_chip.c','''static int32_t beetle_step_fx_voice(int32_t mono,const struct track_chip_engine *s,uint8_t channel){
 const uint8_t fx=s->fx_type[channel],v=s->fx_value[channel];''','''static int32_t beetle_step_fx_process(int32_t mono,const struct track_chip_engine *s,
 uint8_t channel,uint8_t fx,uint8_t v){''')
patch('track_chip.c',''' return mono;
}
int16_t *beetle_stem_capture;''',''' return mono;
}
static int32_t beetle_step_fx_voice(int32_t mono,const struct track_chip_engine *s,uint8_t channel){
 mono=beetle_step_fx_process(mono,s,channel,s->fx_type[channel],s->fx_value[channel]);
 for(uint8_t slot=0u;slot<2u;slot++){
  uint8_t fx=s->fx_extra_type[channel][slot];
  if(fx)mono=beetle_step_fx_process(mono,s,channel,fx,s->fx_extra_value[channel][slot]);
 }
 return mono;
}
int16_t *beetle_stem_capture;''')
patch('track_chip.c','''   uint8_t kind=s->fx_type[ch];
   if(kind==6u||kind==9u){
    uint32_t rate=23u+(uint32_t)s->fx_value[ch]*25u;
    beetle_step_phase[ch]+=rate*512u;
   }''','''   uint8_t kind=s->fx_type[ch],depth=s->fx_value[ch];
   for(uint8_t j=0u;j<2u;j++)if(s->fx_extra_type[ch][j]==6u||s->fx_extra_type[ch][j]==9u){
    kind=s->fx_extra_type[ch][j];depth=s->fx_extra_value[ch][j];
   }
   if(kind==6u||kind==9u){
    uint32_t rate=23u+(uint32_t)depth*25u;
    beetle_step_phase[ch]+=rate*512u;
   }''')
patch('track_sustain_fx.h','''  chip->fx_type[lane]=0u;chip->fx_value[lane]=50u;''','''  chip->fx_type[lane]=0u;chip->fx_value[lane]=50u;
  for(uint8_t j=0u;j<2u;j++){chip->fx_extra_type[lane][j]=0u;chip->fx_extra_value[lane][j]=50u;}''')
patch('track_sustain_fx.h','''}
#endif''','''}
/* Apply additional per-step FX in deterministic slot order after FX1.
 * Each extra runs through the same DSP, while static lane parameters (filter,
 * drive, gate, volume, pitch) are composed without replacing the first FX. */
static inline void beetle_step_fx_multi(struct track_chip_engine *chip,
 uint8_t lane,const struct track_extra_fx *a,const struct track_extra_fx *b,uint16_t bpm){
 if(!chip||lane>=TRACK_SYNTH_CHANNELS)return;
 const struct track_extra_fx *entries[2]={a,b};
 uint8_t pitch=0u;
 for(uint8_t j=0u;j<2u;j++){
  const struct track_extra_fx *f=entries[j];if(!f||!f->effect)continue;
  const uint8_t type=f->effect,value=f->value;
  chip->fx_extra_type[lane][j]=type;chip->fx_extra_value[lane][j]=value;
  if(type==1u)chip->cutoff[lane]=(uint8_t)(4u+(uint32_t)value*2u);
  else if(type==2u && value>chip->drive[lane])chip->drive[lane]=value;
  else if(type==8u){
   const uint32_t dur=661500u/(bpm?bpm:120u);
   uint32_t end=(uint32_t)(((uint64_t)dur*value)/100u);
   if(end<chip->cut_after[lane])chip->cut_after[lane]=end;
  }else if(type==TRACK_FX_VOLUME){
   chip->step_gain[lane]=(uint8_t)((uint16_t)chip->step_gain[lane]*value/100u);
  }else if(type==11u)pitch=1u;
 }
 if(pitch)track_chip_repitch_channel(chip,lane);
}
#endif''')
print('audio done')
