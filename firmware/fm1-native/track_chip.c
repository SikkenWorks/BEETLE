/* TRACK 0.5 native chip-inspired oscillator DSP, GPL-3.0-only. */
#include "track_chip.h"
#include "beetle_sid_lite.h"
#include "beetle_nes_apu.h"
#include "beetle_wave_balance.h"
#include "beetle_punch_timing.h"
#include "beetle_voice_headroom.h"
#include "beetle_stereo_pan.h"
#include "track_mixer.h"
#include <string.h>
/* Exact note-to-Q32 increments generated offline; no libm in firmware. */
static const uint32_t note_steps[128]={
 796254u,843601u,893765u,946911u,1003217u,1062871u,1126073u,1193033u,
 1263974u,1339134u,1418763u,1503127u,1592507u,1687203u,1787529u,1893821u,
 2006434u,2125742u,2252146u,2386065u,2527948u,2678268u,2837526u,3006254u,
 3185015u,3374406u,3575058u,3787642u,4012867u,4251485u,4504291u,4772130u,
 5055896u,5356535u,5675051u,6012507u,6370030u,6748811u,7150117u,7575285u,
 8025735u,8502970u,9008582u,9544261u,10111792u,10713070u,11350103u,12025015u,
 12740059u,13497623u,14300233u,15150569u,16051469u,17005939u,18017165u,19088521u,
 20223584u,21426141u,22700205u,24050030u,25480119u,26995246u,28600467u,30301139u,
 32102938u,34011878u,36034330u,38177043u,40447168u,42852281u,45400411u,48100060u,
 50960238u,53990491u,57200933u,60602278u,64205876u,68023757u,72068660u,76354085u,
 80894335u,85704563u,90800821u,96200119u,101920476u,107980983u,114401866u,121204555u,
 128411753u,136047513u,144137319u,152708170u,161788671u,171409126u,181601643u,192400238u,
 203840952u,215961966u,228803732u,242409110u,256823506u,272095026u,288274639u,305416341u,
 323577341u,342818251u,363203285u,384800477u,407681904u,431923931u,457607465u,484818220u,
 513647012u,544190053u,576549277u,610832681u,647154683u,685636503u,726406571u,769600953u,
 815363807u,863847862u,915214929u,969636441u,1027294024u,1088380105u,1153098554u,1221665363u,
};
/* DMG NR13/NR14 11-bit period quantization (f = 131072 / (2048 - period)).
 * Precomputed outside the realtime path. MIDI note tracking retains Game Boy tuning. */
static const uint32_t dmg_note_steps[128]={
 6236104u,6236104u,6236104u,6236104u,6236104u,6236104u,6236104u,6236104u,
 6236104u,6236104u,6236104u,6236104u,6236104u,6236104u,6236104u,6236104u,
 6236104u,6236104u,6236104u,6236104u,6236104u,6236104u,6236104u,6236104u,
 6236104u,6236104u,6236104u,6236104u,6236104u,6236104u,6236104u,6236104u,
 6236104u,6236104u,6236104u,6236104u,6369913u,6750558u,7151431u,7575849u,
 8023448u,8504534u,9008684u,9547723u,10115139u,10709149u,11346938u,12020061u,
 12739825u,13493980u,14294854u,15142711u,16056988u,16997743u,18004662u,19081173u,
 20230277u,21418297u,22714066u,24040123u,25479651u,26987960u,28621760u,30321390u,
 32073631u,34040814u,36060184u,38219476u,40396535u,42836594u,45428132u,48170963u,
 51061220u,54090276u,57243520u,60499076u,64147262u,67900559u,72120368u,76438952u,
 80793070u,85673188u,90534078u,95979737u,102122441u,108180551u,113975938u,121574334u,
 128942475u,135801118u,143430394u,151967917u,161586140u,172504123u,182361501u,193413713u,
 202623890u,216361103u,227951876u,240854813u,255306101u,271602235u,290120570u,303935835u,
 327315515u,345008245u,364723002u,386827426u,411784034u,425510169u,455903752u,490973272u,
 510612203u,555013264u,580241139u,607871670u,638265253u,671858161u,709183615u,750900298u,
 797831567u,851020338u,911807505u,981946544u,1063775422u,1063775422u,1160482279u,1276530507u,
};
/* 4-bit-style retro wavetable, plus harmonic-limited VA tables. */
#include "track_va_table.h"
#include "track_va_sine.h"
#define TRACK_CHIP_COOKIE 0x5452414Bu /* TRAK */
static const uint8_t retro_wave[32]={8,10,12,14,15,15,14,13,12,12,12,9,6,3,1,0,0,1,2,3,3,3,3,4,5,5,6,6,7,8,8,8};
static int32_t bound(int32_t v,int32_t lo,int32_t hi){return v<lo?lo:v>hi?hi:v;}
static uint8_t va_tier(uint32_t inc){
 /* choose harmonic limit by oscillator increment (approx Nyquist envelope) */
 if(inc>0x13000000u)return 7u;
 if(inc>0x0A000000u)return 6u;
 if(inc>0x05000000u)return 5u;
 if(inc>0x03000000u)return 4u;
 if(inc>0x02000000u)return 3u;
 if(inc>0x01200000u)return 2u;
 if(inc>0x00900000u)return 1u;
 return 0u;
}
static int32_t va_table(uint8_t wave,uint32_t phase,uint8_t tier){
 const int16_t *t=track_va_waves[wave][tier];
 uint8_t n=(uint8_t)(phase>>24u);uint32_t fract=(phase>>16u)&255u;
 return (int32_t)t[n]+(((int32_t)t[(uint8_t)(n+1u)]-(int32_t)t[n])*(int32_t)fract>>8u);
}
void track_va_patch_default(struct track_va_patch *p){
 if(!p)return;
 *p=(struct track_va_patch){.wave_a=0u,.wave_b=1u,.coarse=0,.fine=0,
  .detune=12u,.mix=50u,.pulse_width=50u,.morph=0u,
  .coarse_b=0,.fine_b=0,.pulse_width_b=50u,.drive=0u,.sub=0u,.noise=0u,.fm_depth=0u};
}
/* Project-local editable starting presets: INST 5 bass and INST 6 soft pad
 * deliberately differ from the standard VA OSC / VA OSC2 defaults. */
void track_va_patch_factory(uint8_t slot,struct track_va_patch *p){
 track_va_patch_default(p);
 if(!p)return;
 if(slot==4u){ /* VA bass: sub octave on B, focused midrange */
  p->wave_a=0u;p->wave_b=3u;p->coarse_b=-12;p->detune=8u;
  p->mix=35u;p->sub=18u;p->drive=10u;
 }else if(slot==5u){ /* VA pad: soft sine/triangle detuned */
  p->wave_a=3u;p->wave_b=1u;p->detune=22u;p->mix=60u;
  p->fine_b=7;p->morph=30u;p->drive=2u;
 }
}
int track_va_patch_valid(const struct track_va_patch *p){
 return p&&p->wave_a<TRACK_VA_SHAPES&&p->wave_b<TRACK_VA_SHAPES&&
 p->coarse>=-24&&p->coarse<=24&&p->fine>=-100&&p->fine<=100&&
 p->detune<=100u&&p->mix<=100u&&p->pulse_width>=5u&&p->pulse_width<=95u&&p->morph<=100u&&
 p->coarse_b>=-24&&p->coarse_b<=24&&p->fine_b>=-100&&p->fine_b<=100&&
 p->pulse_width_b>=5u&&p->pulse_width_b<=95u&&
 p->drive<=100u&&p->sub<=100u&&p->noise<=100u&&p->fm_depth<=100u;
}
static uint32_t tune_step(uint32_t step,int cents){
 /* One-time cents approximation, <0.4% relative error over +/-100 cents.
  * No floating point or heap. Integer Q17: 76/131072 ~ ln(2)/1200. */
 int64_t offset=((int64_t)step*cents*76)/131072;
 int64_t result=(int64_t)step+offset;
 if(result<1)result=1;
 if(result>0xFFFFFFFFLL)result=0xFFFFFFFFLL;
 return (uint32_t)result;
}
/* Constant-time phase-width lookup. Replaces a 64-bit divide in the
 * 44.1 kHz dual-oscillator sample loop, bit-identical for widths 5..95. */
static const uint32_t beetle_pulse_phase_width[96]={
 0u,42949672u,85899345u,128849018u,171798691u,214748364u,257698037u,300647710u,
 343597383u,386547056u,429496729u,472446402u,515396075u,558345748u,601295421u,644245094u,
 687194767u,730144440u,773094113u,816043786u,858993459u,901943132u,944892805u,987842478u,
 1030792151u,1073741824u,1116691496u,1159641169u,1202590842u,1245540515u,1288490188u,1331439861u,
 1374389534u,1417339207u,1460288880u,1503238553u,1546188226u,1589137899u,1632087572u,1675037245u,
 1717986918u,1760936591u,1803886264u,1846835937u,1889785610u,1932735283u,1975684956u,2018634629u,
 2061584302u,2104533975u,2147483648u,2190433320u,2233382993u,2276332666u,2319282339u,2362232012u,
 2405181685u,2448131358u,2491081031u,2534030704u,2576980377u,2619930050u,2662879723u,2705829396u,
 2748779069u,2791728742u,2834678415u,2877628088u,2920577761u,2963527434u,3006477107u,3049426780u,
 3092376453u,3135326126u,3178275799u,3221225472u,3264175144u,3307124817u,3350074490u,3393024163u,
 3435973836u,3478923509u,3521873182u,3564822855u,3607772528u,3650722201u,3693671874u,3736621547u,
 3779571220u,3822520893u,3865470566u,3908420239u,3951369912u,3994319585u,4037269258u,4080218931u
};
static int32_t va_shape(uint8_t wave,uint32_t phase,uint8_t tier,uint8_t width){
 if(wave<3u)return beetle_level_q8(va_table(wave,phase,tier),beetle_shape_gain_q8(wave));
 if(wave==3u){uint8_t n=(uint8_t)(phase>>24);uint32_t f=(phase>>16)&255u;
  return beetle_level_q8((int32_t)track_va_sine[n]+((((int32_t)track_va_sine[(uint8_t)(n+1u)]-track_va_sine[n])*(int32_t)f)>>8),beetle_shape_gain_q8(wave));}
 uint32_t pwidth=beetle_pulse_phase_width[width<5u?5u:width>95u?95u:width];
 return beetle_level_q8((va_table(0u,phase,tier)-va_table(0u,phase+pwidth,tier))/2,beetle_shape_gain_q8(wave));
}
void track_chip_init(struct track_chip_engine *s){
 if(!s)return;
 memset(s,0,sizeof(*s));s->health_cookie=TRACK_CHIP_COOKIE;s->phrase_div_q8=256u;s->punch_bpm=120u;s->realtime_voice_budget=TRACK_CHIP_VOICES;
 for(unsigned i=0;i<16;i++)s->punch_param[i]=50u;
 s->punch_fx_seed=0xA718934Fu;s->punch_focus=255u;
 s->punch_param[1]=20u;s->punch_param[2]=0u;s->punch_param[4]=50u;
 for(unsigned i=0;i<TRACK_CHANNELS;i++){s->step_gain[i]=100u;s->fx_type[i]=0u;s->fx_value[i]=50u;s->duty[i]=2u;s->cutoff[i]=96u;s->cut_after[i]=0xFFFFFFFFu;}
 for(unsigned i=0;i<TRACK_CHIP_SLOTS;i++){s->osc_type[i]=(uint8_t)(i==4u?TRACK_CHIP_VA_OSC:i==5u?TRACK_CHIP_VA_OSC2:i==6u?TRACK_CHIP_SID_6581:i==7u?TRACK_CHIP_NES_PULSE:i);track_va_patch_factory((uint8_t)i,&s->patches[i]);
  s->env_attack[i]=(uint8_t)(i==5u?18u:1u);
  s->env_decay[i]=(uint8_t)(i==4u?25u:i==5u?32u:14u);
  s->env_sustain[i]=(uint8_t)(i==4u?70u:i==5u?80u:100u);
  s->env_release[i]=(uint8_t)(i==4u?18u:i==5u?48u:16u);}
}
int track_chip_health(const struct track_chip_engine *s){
 if(!s||s->health_cookie!=TRACK_CHIP_COOKIE||s->voice_count>TRACK_CHIP_VOICES||s->last_voice_index>=TRACK_CHIP_VOICES||s->realtime_voice_budget<4u||s->realtime_voice_budget>TRACK_CHIP_VOICES)return 0;
 for(uint8_t k=0u;k<TRACK_CHANNELS;k++){
  if(s->step_gain[k]>100u)return 0;
 }
 for(uint8_t k=0u;k<TRACK_CHIP_SLOTS;k++){
  if(!track_va_patch_valid(&s->patches[k]))return 0;
  if(s->env_attack[k]>99u||s->env_decay[k]>99u||s->env_sustain[k]>100u||s->env_release[k]>99u)return 0;
 }
 for(uint8_t i=0;i<s->voice_count;i++)if(s->voices[i].active&&(s->voices[i].channel>=TRACK_SYNTH_CHANNELS||s->voices[i].instrument>=TRACK_CHIP_SLOTS||s->voices[i].type>=TRACK_CHIP_TYPES||s->voices[i].velocity==0u))return 0;
 return 1;
}
void track_chip_recover(struct track_chip_engine *s){
 if(!s)return;
 /* Keep oscillator settings, restart only damaged DSP state and its history. */
 uint8_t duty[TRACK_CHANNELS],cutoff[TRACK_CHANNELS],drive[TRACK_CHANNELS],types[TRACK_CHIP_SLOTS],step_gain[TRACK_CHANNELS],fx_type[TRACK_CHANNELS],fx_value[TRACK_CHANNELS],mono=s->mono;
 uint8_t ea[TRACK_CHIP_SLOTS],ed[TRACK_CHIP_SLOTS],es[TRACK_CHIP_SLOTS],er[TRACK_CHIP_SLOTS],punch_params[5];
 memcpy(punch_params,s->punch_param,sizeof(punch_params));
 memcpy(ea,s->env_attack,sizeof(ea));memcpy(ed,s->env_decay,sizeof(ed));memcpy(es,s->env_sustain,sizeof(es));memcpy(er,s->env_release,sizeof(er));
 struct track_va_patch patches[TRACK_CHIP_SLOTS];memcpy(patches,s->patches,sizeof(patches));
 uint32_t restarts=s->restart_count;
 memcpy(fx_type,s->fx_type,sizeof(fx_type));memcpy(fx_value,s->fx_value,sizeof(fx_value));memcpy(step_gain,s->step_gain,sizeof(step_gain));memcpy(duty,s->duty,sizeof(duty));memcpy(cutoff,s->cutoff,sizeof(cutoff));memcpy(drive,s->drive,sizeof(drive));memcpy(types,s->osc_type,sizeof(types));
 track_chip_init(s);memcpy(s->punch_param,punch_params,sizeof(punch_params));for(uint8_t g=0;g<TRACK_CHANNELS;g++)if(step_gain[g]<=100u)s->step_gain[g]=step_gain[g];memcpy(s->duty,duty,sizeof(duty));memcpy(s->cutoff,cutoff,sizeof(cutoff));
 memcpy(s->drive,drive,sizeof(drive));memcpy(s->fx_type,fx_type,sizeof(fx_type));memcpy(s->fx_value,fx_value,sizeof(fx_value));memcpy(s->osc_type,types,sizeof(types));
 for(uint8_t c=0;c<TRACK_CHIP_SLOTS;c++){
  if(ea[c]<=99u)s->env_attack[c]=ea[c];
  if(ed[c]<=99u)s->env_decay[c]=ed[c];
  if(es[c]<=100u)s->env_sustain[c]=es[c];
  if(er[c]<=99u)s->env_release[c]=er[c];
 }
 for(uint8_t c=0;c<TRACK_CHIP_SLOTS;c++)if(track_va_patch_valid(&patches[c]))s->patches[c]=patches[c];
 for(uint8_t c=0;c<TRACK_CHIP_SLOTS;c++)if(s->osc_type[c]>=TRACK_CHIP_TYPES)s->osc_type[c]=TRACK_CHIP_RETRO_PULSE;
 s->mono=mono;s->restart_count=restarts+1u;
}
void track_chip_begin_step(struct track_chip_engine *s){
 if(!s)return;
 if(!track_chip_health(s))track_chip_recover(s);
 /* A tracker step is NOT a note-off. Keep previous voices alive until STP,
  * ADSR's zero-sustain release, voice stealing, or transport STOP. */
 s->step_samples=0u;
}
void track_chip_silence(struct track_chip_engine *s){
 if(!s)return;
 if(!track_chip_health(s))track_chip_recover(s);
 s->tail_left=s->prev_left;s->tail_right=s->prev_right;s->release_left=64u;
 s->voice_count=0u;memset(s->voices,0,sizeof(s->voices));s->phrase_max_voices=0u;s->phrase_div_q8=256u;
}
/* Emergency-only pressure valve. Under repeated missed DMA deadlines reduce
 * actual BEETLE synth polyphony (the inherited ETCH voice limit does not
 * control the tracker DSP). Pick released/quiet voices first. No allocation,
 * no reconfiguration during ordinary playback, and never touch saved data. */
void track_chip_set_realtime_budget(struct track_chip_engine *s,uint8_t budget){
 if(!s)return;
 if(budget<4u)budget=4u;
 if(budget>TRACK_CHIP_VOICES)budget=TRACK_CHIP_VOICES;
 s->realtime_voice_budget=budget;
 while(s->voice_count>budget){
  uint8_t victim=0u;
  int32_t best=INT32_MAX;
  for(uint8_t i=0u;i<s->voice_count;i++){
   const struct track_chip_voice *v=&s->voices[i];
   int32_t amp=v->last_level<0?-v->last_level:v->last_level;
   if(amp>30000)amp=30000;
   int32_t score=(v->active?0:-100000)+(v->released?0:50000)+amp;
   if(score<best){best=score;victim=i;}
  }
  uint8_t last=(uint8_t)(s->voice_count-1u);
  if(victim!=last)s->voices[victim]=s->voices[last];
  memset(&s->voices[last],0,sizeof(s->voices[last]));
  s->voice_count=last;
  s->steal_count++;
 }
}
void track_chip_set_mono(struct track_chip_engine *s,uint8_t mono){
 if(!s)return;
 s->mono=!!mono;if(mono&&s->voice_count>1u){
  uint8_t recent=0u;for(uint8_t k=1u;k<s->voice_count;k++)if(s->voices[k].birth>s->voices[recent].birth)recent=k;
  struct track_chip_voice keep=s->voices[recent];memset(s->voices,0,sizeof(s->voices));s->voices[0]=keep;s->voice_count=keep.active?1u:0u;
 }
}
static void track_chip_tune_voice(struct track_chip_voice *v,const struct track_chip_engine *s){
 const struct track_va_patch *p=&s->patches[v->instrument];
 int32_t pitch=0;
 if(s->fx_type[v->channel]==11u)pitch=(int32_t)((uint32_t)s->fx_value[v->channel]*24u/100u)-12;
 for(uint8_t j=0u;j<2u;j++)if(s->fx_extra_type[v->channel][j]==11u)
  pitch+=(int32_t)((uint32_t)s->fx_extra_value[v->channel][j]*24u/100u)-12;
 int32_t a=(int32_t)v->note+(int32_t)p->coarse+pitch+(int32_t)s->punch_octave;
 int32_t b=(int32_t)v->note+(int32_t)p->coarse_b+pitch+(int32_t)s->punch_octave;
 if(a<0)a=0;if(a>127)a=127;
 if(b<0)b=0;if(b>127)b=127;
 v->step=tune_step(v->type==TRACK_CHIP_RETRO_PULSE?dmg_note_steps[a]:note_steps[a],p->fine);
 v->step2=tune_step(note_steps[b],(int32_t)p->fine_b+(int32_t)p->detune);
 if(v->type==TRACK_CHIP_NES_NOISE)v->step2=beetle_nes_noise_increment((uint8_t)a);
}
/* Punch octave is a live pitch transformation, not a transposition of
 * stored notes. Changes also retune sustained/held voices immediately.
 * One shared path handles all 12 oscillator types, including NES NOISE. */
void track_chip_set_punch_octave(struct track_chip_engine *s,uint16_t mask){
 if(!s)return;
 const int8_t semitones=(int8_t)(((mask&(1u<<5))?12:0)-((mask&(1u<<6))?12:0));
 if(s->punch_octave==semitones)return;
 s->punch_octave=semitones;
 for(uint8_t i=0u;i<s->voice_count;i++){
  struct track_chip_voice *v=&s->voices[i];
  if(v->active&&v->channel<TRACK_SYNTH_CHANNELS)
   track_chip_tune_voice(v,s);
 }
}
void track_chip_repitch_channel(struct track_chip_engine *s,uint8_t channel){
 if(!s||channel>=TRACK_SYNTH_CHANNELS)return;
 for(uint8_t i=0u;i<s->voice_count;i++){
  struct track_chip_voice *v=&s->voices[i];
  if(v->active&&v->channel==channel)track_chip_tune_voice(v,s);
 }
}
void track_chip_refresh_instrument(struct track_chip_engine *s,uint8_t instrument){
 if(!s||instrument>=TRACK_CHIP_SLOTS)return;
 for(uint8_t i=0u;i<s->voice_count;i++){
  struct track_chip_voice *v=&s->voices[i];
  if(!v->active||v->instrument!=instrument)continue;
  v->type=s->osc_type[instrument]<TRACK_CHIP_TYPES?s->osc_type[instrument]:TRACK_CHIP_RETRO_PULSE;
  track_chip_tune_voice(v,s);
  v->envelope_sustain=(1048576u*s->env_sustain[instrument])/100u;
  uint32_t a=(uint32_t)s->env_attack[instrument]*441u;
  uint32_t d=(uint32_t)s->env_decay[instrument]*441u;
  v->attack_rate=1048576u/(a?a:1u)+1u;
  v->decay_rate=(1048576u-v->envelope_sustain)/(d?d:1u)+1u;
  if(!v->released)v->release_rate=(uint32_t)(s->env_release[instrument]?s->env_release[instrument]:1u)*441u;
  /* Preserve phase, active envelope and note-on serial, preventing retrigger. */
 }
}
void track_chip_note_off(struct track_chip_engine *s,uint8_t channel,uint8_t note){
 if(!s)return;
 for(uint8_t i=0u;i<s->voice_count;i++){
  struct track_chip_voice *v=&s->voices[i];
  if(v->active&&v->channel==channel&&v->note==note){v->released=1u;}
 }
}
void track_chip_release_channel(struct track_chip_engine *s,uint8_t channel){
 if(!s||channel>=TRACK_SYNTH_CHANNELS)return;
 for(uint8_t i=0u;i<s->voice_count;i++)
  if(s->voices[i].active && s->voices[i].channel==channel)s->voices[i].released=1u;
}
void track_chip_stop_channel(struct track_chip_engine *s,uint8_t channel){
 if(!s||channel>=TRACK_SYNTH_CHANNELS)return;
 for(uint8_t i=0u;i<s->voice_count;i++){
  struct track_chip_voice *v=&s->voices[i];
  if(v->active && v->channel==channel){
   v->released=1u;
   /* release_rate is interpreted as sample duration upon entering ADSR R. */
   if(v->env_stage!=3u)v->release_rate=176u;
  }
 }
}
int track_chip_note(struct track_chip_engine *s,uint8_t channel,uint8_t instrument,uint8_t note,uint8_t velocity){
 if(!s||channel>=TRACK_SYNTH_CHANNELS||instrument>=TRACK_CHIP_SLOTS||note>127u||velocity==0u)return 0;
 if(!track_chip_health(s))track_chip_recover(s);
 uint8_t index=TRACK_CHIP_VOICES;
 if(s->mono){
  if(s->voice_count && s->voices[0].active)s->steal_count++;
  s->voice_count=1u;
  index=0u;
 }
 /* Released voices are reused first. Invalid/inactive slots never count. */
 if(!s->mono)for(uint8_t i=0;i<s->voice_count;i++)if(!s->voices[i].active){index=i;break;}
 if(index==TRACK_CHIP_VOICES&&s->voice_count<s->realtime_voice_budget)index=s->voice_count++;
 if(index==TRACK_CHIP_VOICES){
  /* Advanced voice stealing: prefer released/quiet, then oldest and same
   * channel. No heap, nondeterministic selection or silent dropped notes. */
  int32_t best=0x7FFFFFFF;index=0;
  for(uint8_t i=0;i<s->voice_count;i++){
   const struct track_chip_voice *v=&s->voices[i];
   int32_t score=(v->released?0:100000)+(v->channel==channel?0:1000);
   int32_t amp=v->last_level<0?-v->last_level:v->last_level;
   score+=amp>20000?20000:amp;
   uint32_t age=s->alloc_serial-v->birth;
   score-=(int32_t)(age>1000u?1000u:age);
   if(score<best){best=score;index=i;}
  }
  s->steal_count++;
 }
 s->last_voice_index=index;
 struct track_chip_voice *v=&s->voices[index];
 memset(v,0,sizeof(*v));v->active=1u;v->channel=channel;
 v->type=s->osc_type[instrument]<TRACK_CHIP_TYPES?s->osc_type[instrument]:TRACK_CHIP_RETRO_PULSE;v->note=note;v->instrument=instrument;
 track_chip_tune_voice(v,s);
 v->velocity=velocity;v->birth=++s->alloc_serial;
 v->phase2=0x80000000u;
 v->lfsr=(v->type==TRACK_CHIP_NES_NOISE)?1u:0x7ffff8u;
 if(v->type==TRACK_CHIP_NES_NOISE)v->step2=beetle_nes_noise_increment((uint8_t)bound((int32_t)note+(int32_t)s->punch_octave,0,127));
 /* Envelope rates are computed once on note-on, never divided in the DMA loop. */
 const uint32_t unit=1048576u;
 uint32_t a=(uint32_t)s->env_attack[instrument]*441u;
 uint32_t d=(uint32_t)s->env_decay[instrument]*441u;
 uint32_t r=(uint32_t)s->env_release[instrument]*441u;
 if(!a)a=1u;if(!d)d=1u;if(!r)r=1u;
 v->attack_rate=unit/a+1u;
 v->envelope_sustain=(unit*(uint32_t)s->env_sustain[instrument])/100u;
 v->decay_rate=(unit-v->envelope_sustain)/d+1u;
 v->release_rate=r; /* samples: compute live level delta on note-off */
 return 1;
}
static int32_t sample_one(struct track_chip_voice *v,const struct track_chip_engine *s, uint32_t lfo_phase, const struct beetle_sid_prepared *sid_cfg, uint8_t tier_a,uint8_t tier_b,uint8_t vib){
 /* VIBRATO changes the live note pitch. The broader +/-9% maximum
  * excursion produces a clearly audible step effect, even at medium depth. */
 uint32_t step=v->step,step2=v->step2;
 /* V1.4.2 DMA P2: this lane vibrato selection is now done once per render chunk. */
 if(vib){
  int32_t wave=(int32_t)track_va_sine[(uint8_t)(lfo_phase>>24u)];
  int32_t cents=(wave*(int32_t)vib*9)/13000;
  int32_t mul=10000+cents;
  step=(uint32_t)(((uint64_t)step*(uint32_t)mul)/10000u);
  step2=(uint32_t)(((uint64_t)step2*(uint32_t)mul)/10000u);
 }
 v->phase+=step;
 int32_t level=0;
 switch(v->type){
 case TRACK_CHIP_NES_PULSE:
  level=beetle_nes_pulse(v->phase,s->duty[v->channel]);break;
 case TRACK_CHIP_NES_TRIANGLE:
  /* Match NES PULSE perceived level without relying on the MASTER limiter. */
  level=(int32_t)beetle_nes_triangle(v->phase)*19/10;break;
 case TRACK_CHIP_NES_NOISE:{
  uint32_t previous=v->noise_clock;
  v->noise_clock+=step2;
  if(v->noise_clock<previous){
   level=beetle_nes_noise(&v->lfsr,(uint8_t)(s->duty[v->channel]>=2u));
   v->lowpass=level;
  }else level=v->lowpass;
 }break;
 case TRACK_CHIP_RETRO_PULSE:{
  /* Game Boy DMG CH1/CH2: 8-step duty sequencer, not generic PWM.
   * Bit patterns are the DMG NR11/NR21 pulse duty sequences.
   * Do not reuse NES pulse's 8-bit phase mapping or bipolar DAC. */
  static const uint8_t dmg_duty[4]={0x80u,0x81u,0xe1u,0x7eu};
  uint8_t phase=(uint8_t)(v->phase>>29u);
  level=((dmg_duty[s->duty[v->channel]&3u]>>phase)&1u)?1:0;
 }break;
 case TRACK_CHIP_RETRO_WAVE:{
  /* Smooth 32-position wavetable reads rather than jumping abruptly from
   * one 4-bit sample to the next. Retain the same fixed original ROM wave.
   * Band-reduced interpolation removes the worst high-note zipper/aliasing. */
  const uint8_t at=(uint8_t)(v->phase>>27u);
  const uint32_t frac=(v->phase>>11u)&65535u;
  const int32_t a=(int32_t)retro_wave[at]-8;
  const int32_t b=(int32_t)retro_wave[(at+1u)&31u]-8;
  level=(a+(((b-a)*(int32_t)frac)>>16))*1800;
 }break;
 case TRACK_CHIP_SID_6581: case TRACK_CHIP_SID_8580:
 case TRACK_CHIP_SID_DIRTY: case TRACK_CHIP_SID_BASS: case TRACK_CHIP_SID_NOISE:{
  uint8_t style=v->type;
  level=beetle_sid_sample_prepared(sid_cfg,v->phase,step,&v->lfsr,
                                  &v->lowpass,&v->sid_bandpass);
  /* Soft 6581-style asymmetric saturation. Character variations remain
   * bounded to avoid feeding huge peaks into MASTER dynamics. */
  if(style==TRACK_CHIP_SID_DIRTY){level=(level*7)/5;
   if(level>13500)level=13500;if(level< -12500)level=-12500;}
 }break;
 case TRACK_CHIP_VA_OSC:{
  /* Morph between independently tuned and shaped bandlimited oscillators. */
  const struct track_va_patch *p=&s->patches[v->instrument];
  v->phase2+=step2;
  int32_t a=va_shape(p->wave_a,v->phase,tier_a,p->pulse_width);
  uint32_t bphase=v->phase2+(uint32_t)(((int64_t)a*p->fm_depth*8192)/100);
  int32_t b=va_shape(p->wave_b,bphase,tier_b,p->pulse_width_b);
  level=(a*(100-(int)p->morph)+b*(int)p->morph)/100;
  uint32_t cutoff=(uint32_t)s->cutoff[v->channel]+28u;
  if(cutoff>245u)cutoff=245u;
  v->lowpass+=(int32_t)(((int64_t)(level-v->lowpass)*cutoff)>>8);
  level=v->lowpass;
  if(p->sub){int32_t oct=(int32_t)track_va_sine[(uint8_t)(v->phase>>25)];
   level+=(oct*(int32_t)p->sub)/140;}
  if(p->noise){v->lfsr=v->lfsr*1664525u+1013904223u;
   int32_t fuzz=(int32_t)((v->lfsr>>17)&0x7FFFu)-16384;
   level+=(fuzz*(int32_t)p->noise)/250;}
  if(p->drive){int32_t gain=100+(int32_t)p->drive*6;
   level=(int32_t)(((int64_t)level*gain)/100);
   level=bound(level,-12000,12000);}
 }break;
 case TRACK_CHIP_VA_OSC2:{
  /* Independent A and B waveform, width, tuning and mix; no hardcoded saw. */
  const struct track_va_patch *p=&s->patches[v->instrument];
  v->phase2+=step2;
  int32_t a=va_shape(p->wave_a,v->phase,tier_a,p->pulse_width);
  uint32_t bphase=v->phase2+(uint32_t)(((int64_t)a*p->fm_depth*8192)/100);
  int32_t b=va_shape(p->wave_b,bphase,tier_b,p->pulse_width_b);
  /* Morph varies the second oscillator's waveshape continuously toward A. */
  b=(b*(100-(int)p->morph)+a*(int)p->morph)/100;
  level=(a*(100-(int)p->mix)+b*(int)p->mix)/100;
  uint32_t cutoff=(uint32_t)s->cutoff[v->channel]*3u/4u+16u;
  if(cutoff>245u)cutoff=245u;
  v->lowpass+=(int32_t)(((int64_t)(level-v->lowpass)*cutoff)>>8);
  level=v->lowpass;
  if(p->sub){int32_t oct=(int32_t)track_va_sine[(uint8_t)(v->phase>>25)];
   level+=(oct*(int32_t)p->sub)/140;}
  if(p->noise){v->lfsr=v->lfsr*1664525u+1013904223u;
   int32_t fuzz=(int32_t)((v->lfsr>>17)&0x7FFFu)-16384;
   level+=(fuzz*(int32_t)p->noise)/250;}
  if(p->drive){int32_t gain=100+(int32_t)p->drive*6;
   level=(int32_t)(((int64_t)level*gain)/100);
   level=bound(level,-12000,12000);}
 }break;
 default:level=0;break;
 }
 if(v->released && v->env_stage!=3u){
  v->env_stage=3u;
  v->release_rate=v->envelope/(v->release_rate?v->release_rate:1u)+1u;
 }
 if(v->env_stage==0u){
  if(v->envelope+v->attack_rate>=1048576u){v->envelope=1048576u;v->env_stage=1u;}
  else v->envelope+=v->attack_rate;
 }else if(v->env_stage==1u){
  if(v->envelope<=v->envelope_sustain+v->decay_rate){v->envelope=v->envelope_sustain;v->env_stage=2u;if(!v->envelope)v->active=0u;}
  else v->envelope-=v->decay_rate;
 }else if(v->env_stage==3u){
  if(v->envelope<=v->release_rate){v->envelope=0u;v->active=0u;}
  else v->envelope-=v->release_rate;
 }
 if(v->type==TRACK_CHIP_RETRO_PULSE){
  /* DMG 4-bit channel DAC and AC-coupled analog output. Quantizing the
   * existing ADSR to 16 discrete steps gives the characteristic coarse
   * volume changes without adding a second RAM envelope or breaking
   * existing saved patches. Filter state is voice-local, bounded and
   * updated once per sample, including during note release. */
  uint32_t env4=(v->envelope*15u+524288u)>>20u;
  if(env4>15u)env4=15u;
  int32_t discrete=(int32_t)env4*1370;
  int32_t amplitude=discrete;
  if(v->env_stage==3u){
   /* During release, 16 discrete gain steps produce audible low-rate
    * amplitude zippering (the reported buzzy tail). Preserve the original
    * DMG duty oscillator and sustained DAC level, but crossfade for 128
    * samples into a smooth release envelope. Uses only bounded 32-bit math. */
   uint32_t fade=v->release_left;
   if(fade<128u){fade++;v->release_left=(uint16_t)fade;}
   int32_t smooth=(int32_t)((((v->envelope>>8u)*20550u)+2048u)>>12u);
   amplitude=(discrete*(int32_t)(128u-fade)+smooth*(int32_t)fade+64)>>7u;
  }
  int32_t dac=level?amplitude:0;
  v->lowpass+=(dac-v->lowpass)>>8u;
  /* DMG headphone output: AC coupling, a slightly dark single-pole
   * analogue roll-off and smooth saturation. This is intentionally a
   * different timbre from the open/bipolar NES PULSE implementation. */
  int32_t ac=dac-v->lowpass;
  v->sid_bandpass+=(ac-v->sid_bandpass)>>2u;
  int32_t shaped=(v->sid_bandpass*3+ac)/4;
  /* 4-bit headphone DAC clips asymmetrically before a modest gain lift. */
  if(shaped>14500)shaped=14500+(shaped-14500)/4;
  if(shaped< -14500)shaped=-14500+(shaped+14500)/4;
  level=shaped*2;
 }else level=(int32_t)(((int64_t)level*v->envelope)>>20u);
 /* Normalize intrinsic waveform loudness before lane gain and effects.
  * Never normalize based on live RMS (which would flatten performance). */
 level=beetle_level_q8(level,beetle_type_gain_q8(v->type));
 v->last_level=level;
 return (int32_t)(((int64_t)level*v->velocity)/127);
}
/* 5 x 512-frame circular buffers live outside the low 96KiB RAM.
 * No runtime allocations, no display writes, bounded integer DSP. */
#define BEETLE_STEP_DELAY_FRAMES 128u
static int16_t beetle_step_delay[TRACK_SYNTH_CHANNELS][BEETLE_STEP_DELAY_FRAMES]
 __attribute__((section(".beetle_song"),aligned(16)));
static uint16_t beetle_step_write[TRACK_SYNTH_CHANNELS];
static uint32_t beetle_step_phase[TRACK_SYNTH_CHANNELS];
void track_chip_fx_reset(void){
 memset(beetle_step_delay,0,sizeof(beetle_step_delay));
 memset(beetle_step_write,0,sizeof(beetle_step_write));
 memset(beetle_step_phase,0,sizeof(beetle_step_phase));
}
static int32_t beetle_step_fx_process(int32_t mono,const struct track_chip_engine *s,
 uint8_t channel,uint8_t fx,uint8_t v){
 if(!fx)return mono;
 if(fx==9u){ /* Tremolo LFO; 0 slow, 100 fast */
  uint32_t rate=23u+(uint32_t)v*25u;
  uint8_t phase=(uint8_t)(beetle_step_phase[channel]>>24u);
  uint32_t level=(uint32_t)(track_va_sine[phase]+32768)/256u;
  mono=(int32_t)(((int64_t)mono*((int32_t)level+32))/288);
  (void)rate; /* LFO phase advances once per rendered frame, not per voice. */
 }else if(fx==6u){ /* Pitch modulation was applied inside sample_one. */
 }else if(fx==7u){ /* Arpeggio: alternating rhythmic gate, not a no-op. */
  uint32_t segment=200u+(100u-v)*60u;
  if(((s->fx_samples[channel]/segment)&3u)==3u)mono=0;
 }else if(fx==3u||fx==4u||fx==5u){
  /* DELAY/REVERB/CHORUS: independent bounded wet memory per synth lane. */
  uint16_t write=beetle_step_write[channel];
  uint32_t length=fx==3u?35u+(100u-v)/2u:fx==4u?117u:8u+(uint32_t)v/4u;
  uint16_t read=(uint16_t)((write+BEETLE_STEP_DELAY_FRAMES-length)&(BEETLE_STEP_DELAY_FRAMES-1u));
  int32_t old=beetle_step_delay[channel][read];
  int32_t feed=fx==4u?45:fx==3u?35:15;
  int32_t wet=fx==5u?35:fx==4u?40:v/2u;
  int32_t tap=(mono*(100-wet)+old*wet)/100;
  beetle_step_delay[channel][write]=(int16_t)bound((mono+old*feed/100),-32767,32767);
  beetle_step_write[channel]=(uint16_t)((write+1u)&(BEETLE_STEP_DELAY_FRAMES-1u));
  mono=tap;
 }
 return mono;
}
static int32_t beetle_step_fx_voice(int32_t mono,const struct track_chip_engine *s,uint8_t channel){
 mono=beetle_step_fx_process(mono,s,channel,s->fx_type[channel],s->fx_value[channel]);
 for(uint8_t slot=0u;slot<2u;slot++){
  uint8_t fx=s->fx_extra_type[channel][slot];
  if(fx)mono=beetle_step_fx_process(mono,s,channel,fx,s->fx_extra_value[channel][slot]);
 }
 return mono;
}
/* Audio ISR exclusive scratch, not on the small FM1 interrupt stack. */
static struct beetle_sid_prepared beetle_sid_chunk_cache[TRACK_CHIP_VOICES]
 __attribute__((section(".beetle_song"),aligned(4)));
int16_t *beetle_stem_capture;
uint32_t beetle_stem_offset;
void track_chip_render(struct track_chip_engine *s,const struct track_mixer *mix,int16_t *stereo,uint32_t frames){
 if(!s||!stereo)return;
 if(!track_chip_health(s))track_chip_recover(s);
 /* MIXER state is fixed for a render chunk. A full solo scan previously ran
  * for every VOICE and every SAMPLE, a costly hotspot in dense SONG CHAINS.
  * Precompute once per call. ISR owns the mixer for the duration of render. */
 uint8_t mix_solo=0u;
 uint8_t mix_muted[TRACK_SYNTH_CHANNELS]={0};
 int16_t mix_vol[TRACK_SYNTH_CHANNELS];
 /* Cache invariant per-lane gain and drive once per chunk; preserve original
  * 64-bit arithmetic, division order and exact sample output. */
 int32_t lane_gain[TRACK_SYNTH_CHANNELS],lane_drive[TRACK_SYNTH_CHANNELS];
 uint8_t lane_vibrato[TRACK_SYNTH_CHANNELS];
 uint16_t pan_left[TRACK_SYNTH_CHANNELS],pan_right[TRACK_SYNTH_CHANNELS];
 if(mix)for(uint8_t c=0u;c<TRACK_CHANNELS;c++)mix_solo|=mix->ch[c].solo;
 for(uint8_t c=0u;c<TRACK_SYNTH_CHANNELS;c++){
  mix_muted[c]=(uint8_t)(mix&&(mix->ch[c].mute||(mix_solo&&!mix->ch[c].solo)));
  mix_vol[c]=mix?mix->ch[c].volume:100;
  lane_gain[c]=(int32_t)mix_vol[c]*(int32_t)s->step_gain[c];
  lane_drive[c]=100+(int32_t)s->drive[c]*4;
  uint8_t vib=0u;
  if(s->fx_type[c]==6u)vib=s->fx_value[c];
  for(uint8_t j=0u;j<2u;j++)if(s->fx_extra_type[c][j]==6u)vib=s->fx_extra_value[c][j];
  lane_vibrato[c]=vib;
  beetle_stereo_pan_gain(mix?mix->ch[c].pan:0,&pan_left[c],&pan_right[c]);
 }
 /* SID filter mapping, oscillator selection and PWM thresholds are
  * constant within this chunk. In particular SID BASS from NUGGIE no longer
  * recalculates filter and pulse divisions for every sample and voice. */
 struct beetle_sid_prepared *sid_cache=beetle_sid_chunk_cache;
 for(uint8_t vi=0;vi<s->voice_count;vi++){
  const struct track_chip_voice *v=&s->voices[vi];
  if(!v->active || v->type<TRACK_CHIP_SID_6581 || v->type>TRACK_CHIP_SID_NOISE)continue;
  const struct track_va_patch *p=&s->patches[v->instrument];
  uint8_t style=v->type,cut=s->cutoff[v->channel],reso=p->drive,mode=p->morph;
  uint8_t wa=p->wave_a,wb=p->wave_b,mix=p->mix;
  if(style==TRACK_CHIP_SID_DIRTY){reso=(uint8_t)(60u+reso*2u/5u);mode=65u;}
  if(style==TRACK_CHIP_SID_BASS){wa=1u;wb=2u;mode=14u;cut=(uint8_t)(cut/2u+8u);}
  if(style==TRACK_CHIP_SID_NOISE){wa=3u;wb=2u;mix=20u;mode=80u;}
  sid_cache[vi]=beetle_sid_prepare(style,wa,wb,p->pulse_width,p->pulse_width_b,
                                   mix,cut,reso,mode);
 }
 /* The ISR owns these controls until this render chunk completes. The
  * LFO rate never needs to be re-decoded five times per audio sample. */
 uint8_t va_tier_a[TRACK_CHIP_VOICES],va_tier_b[TRACK_CHIP_VOICES];
 for(uint8_t vi=0;vi<s->voice_count;vi++){
  const struct track_chip_voice *v=&s->voices[vi];
  /* Pitch/mip tier changes only on note-on or a chunk-boundary retune. */
  va_tier_a[vi]=va_tier(v->step);va_tier_b[vi]=va_tier(v->step2);
 }
 uint32_t lane_lfo_increment[TRACK_SYNTH_CHANNELS]={0};
 for(uint8_t ch=0;ch<TRACK_SYNTH_CHANNELS;ch++){
  uint8_t kind=s->fx_type[ch],depth=s->fx_value[ch];
  for(uint8_t j=0;j<2u;j++)if(s->fx_extra_type[ch][j]==6u||s->fx_extra_type[ch][j]==9u){
   kind=s->fx_extra_type[ch][j];depth=s->fx_extra_value[ch][j];
  }
  if(kind==6u||kind==9u)lane_lfo_increment[ch]=(23u+(uint32_t)depth*25u)*512u;
 }
 for(uint32_t f=0;f<frames;f++){
  int64_t left=0,right=0;
  int32_t stems[TRACK_SYNTH_CHANNELS]={0};
  uint8_t foreground=0u,active_voices=0u; /* normal and ADSR-release voices counted separately */
  for(uint8_t i=0;i<s->voice_count;i++){
   struct track_chip_voice *v=&s->voices[i];
   if(!v->active)continue;
   int32_t mono=sample_one(v,s,beetle_step_phase[v->channel],&sid_cache[i],va_tier_a[i],va_tier_b[i],lane_vibrato[v->channel]);
   if(s->fx_samples[v->channel]>=s->cut_after[v->channel])continue;
   /* Most tracker rows (including MOUNTAINKING's dense passages) have
    * no active step processing. Avoid three dispatches per VOICE per SAMPLE.
    * No oscillator/envelope tick is skipped; this is PCM-identical. */
   if(s->fx_type[v->channel]||s->fx_extra_type[v->channel][0]||
      s->fx_extra_type[v->channel][1])
    mono=beetle_step_fx_voice(mono,s,v->channel);
   if(s->drive[v->channel]){
    mono=(int32_t)(((int64_t)mono*lane_drive[v->channel])/100);
    mono=bound(mono,-9500,9500);
   }
   if(mix_muted[v->channel])continue;
   const int32_t vol=mix_vol[v->channel];
   mono=(int32_t)(((int64_t)mono*lane_gain[v->channel])/10000);
   if(v->active && vol && s->step_gain[v->channel]){
    active_voices++;if(!v->released)foreground++;
   }
   /* Meter is tapped after lane mute/solo, gain, and step FX. */
   uint32_t peak=(uint32_t)(mono<0?-(int64_t)mono:mono);
   if(peak>32767u)peak=32767u;
   if(peak>track_lane_pcm_peak[v->channel])track_lane_pcm_peak[v->channel]=(uint16_t)peak;
   if(beetle_stem_capture)stems[v->channel]+=mono;
   left+=((int64_t)mono*pan_left[v->channel])>>8;
   right+=((int64_t)mono*pan_right[v->channel])>>8;
  }
  /* V0.46: latch the highest *audible* polyphony in the current phrase.
   * An ADSR note-off cannot halve the divisor and boost the final sustained
   * note by +6 dB. Reset only when all audible voices have ended. Smooth
   * increases (~1 ms) so concurrent onsets cannot jump the normalization. */
  if(foreground>s->phrase_max_voices)s->phrase_max_voices=foreground;
  if(!active_voices)s->phrase_max_voices=0u;
  uint32_t target=beetle_voice_divisor(s->phrase_max_voices);
  if(!s->phrase_div_q8)s->phrase_div_q8=256u;
  if(s->phrase_div_q8<target){
   uint32_t next=(uint32_t)s->phrase_div_q8+8u;
   s->phrase_div_q8=(uint16_t)(next>target?target:next);
  }else if(s->phrase_div_q8>target){
   uint32_t step=foreground?1u:16u;
   s->phrase_div_q8=(uint16_t)(s->phrase_div_q8-target<step?target:s->phrase_div_q8-step);
  }
  if(beetle_stem_capture){
   int16_t *frame=beetle_stem_capture+(beetle_stem_offset+f)*TRACK_CHANNELS;
   for(uint8_t ch=0;ch<TRACK_SYNTH_CHANNELS;ch++){
    int32_t v=(stems[ch]*256)/(int32_t)s->phrase_div_q8;
    frame[ch]=(int16_t)bound(v,-32768,32767);
   }
  }
  left=((int32_t)left*256)/(int32_t)s->phrase_div_q8;
  right=((int32_t)right*256)/(int32_t)s->phrase_div_q8;
  if(s->release_left){
   left+=((int64_t)s->tail_left*s->release_left)/64;
   right+=((int64_t)s->tail_right*s->release_left)/64;
   --s->release_left;
  }
  int32_t l=bound((int32_t)left,-32767,32767);
  int32_t r=bound((int32_t)right,-32767,32767);
  stereo[f*2u]=(int16_t)l;stereo[f*2u+1u]=(int16_t)r;
  s->prev_left=l;s->prev_right=r;s->step_samples++;
  for(uint8_t ch=0u;ch<TRACK_SYNTH_CHANNELS;ch++)
   if(s->fx_samples[ch]!=0xFFFFFFFFu)s->fx_samples[ch]++;
  /* Same LFO phase progression, no per-frame FX type scans or divides. */
  for(uint8_t ch=0;ch<TRACK_SYNTH_CHANNELS;ch++)
   beetle_step_phase[ch]+=lane_lfo_increment[ch];
 }
}

/* BEETLE V0.39: actual frozen audio capture, reversible snapshot,
 * variable-speed tape-stop and independent live amplitude-gate stutter.
 * The history ring becomes READ-ONLY while a snapshot is being processed,
 * preventing the moving-write-head corruption that caused harsh distortion.
 * 3072 stereo frames (~69.7ms at 44100), zero heap allocation.
 */
#define BEETLE_PUNCH_FRAMES 3072u
#define BEETLE_PUNCH_CAPTURE 2816u
static inline uint32_t punch_ring(uint32_t x){while(x>=BEETLE_PUNCH_FRAMES)x-=BEETLE_PUNCH_FRAMES;return x;}
static int16_t beetle_punch_history[BEETLE_PUNCH_FRAMES][2]
 __attribute__((section(".beetle_song"),aligned(16)));
static int32_t punch_lerp(int32_t a,int32_t b,uint32_t fraction){
 return a+(int32_t)(((b-a)*(int32_t)fraction)>>8u);
}
static int32_t punch_clip(int32_t value){return bound(value,-32768,32767);}
static uint16_t punch_sample(uint16_t start,uint32_t offset){
 return (uint16_t)punch_ring((uint32_t)start+offset);
}
void track_chip_process_punch(struct track_chip_engine *s,uint16_t mask,int16_t *dst,uint32_t frames){
 if(!s||!dst)return;
 /* Priority only among tape/snapshot FX; FILTER and mute remain independent. */
 /* Snapshot readers share one immutable 12KiB history. A single voice owns
  * playback while independent lightweight FX remain freely stackable. */
 uint8_t mode=0u;
 const uint16_t snapshots=(uint16_t)(mask&((1u<<0)|(1u<<3)|(1u<<8)|(1u<<11)));
 if(snapshots){
  uint8_t choice=255u;
  if(s->punch_focus<16u&&(snapshots&(1u<<s->punch_focus)))choice=s->punch_focus;
  else for(uint8_t i=0u;i<16u;i++)if(snapshots&(1u<<i))choice=i;
  mode=choice==0u?1u:choice==3u?4u:choice==8u?6u:choice==11u?7u:0u;
 }else mode=(mask&2u)?2u:(mask&16u)?5u:0u;
 if(mode!=s->punch_previous){
  s->punch_previous=mode;
  s->punch_start=(uint16_t)punch_ring((uint32_t)s->punch_write+BEETLE_PUNCH_FRAMES-BEETLE_PUNCH_CAPTURE);
  s->punch_position=0u;s->punch_age=0u;
 }
 /* History stores 22.05 kHz stereo (2-tap decimator): 128 ms of audio
  * within the original 12 KiB RAM budget, played back at original pitch.
  * Minimum reverse window is ~23 ms instead of the distorted 4 ms loop. */
 const uint32_t repeat_len=512u+((100u-s->punch_param[0])*(BEETLE_PUNCH_CAPTURE-512u))/100u;
 /* Reverse captures the entire immutable history, not a short playback
  * window that changes pitch with ALG. Turn ALG to change read speed only. */
 /* Ring modulation does not require any PCM capture or extra RAM. */
 const uint32_t stutter_half=beetle_stutter_half_frames(s->punch_bpm,s->punch_param[4]);
 const uint32_t stutter_edge=stutter_half<512u?32u:128u;
 const uint32_t tape_duration=8820u+((100u-s->punch_param[3])*79380u)/100u; /* 0.2..2 seconds */
 const uint32_t pump_bpm=s->punch_bpm?s->punch_bpm:120u;
 const uint32_t pump_beat=2646000u/pump_bpm; /* once per BLOCK, not per sample */
 const uint32_t pump_period=pump_beat/(1u+(uint32_t)(s->punch_param[10]/17u));
 /* Precompute integer ramp coefficient per audio block. The audio ISR must
  * not pay a variable 32-bit integer division for each PCM sample. */
 const uint32_t pump_low=(pump_period/5u)?pump_period/5u:1u;
 const uint32_t pump_rise=(pump_period>pump_low)?pump_period-pump_low:1u;
 const uint32_t pump_slope=(192u<<12u)/pump_rise;
 for(uint32_t f=0u;f<frames;f++){
  const int32_t dry_l=dst[f*2u],dry_r=dst[f*2u+1u];
  int32_t l=dry_l,r=dry_r,wet_l=l,wet_r=r;
  const uint16_t w=s->punch_write;
  if(mode==1u){
   uint32_t len=repeat_len;
   uint32_t phase=s->punch_position>>8u;
   uint32_t fract=s->punch_position&255u;
   uint32_t offset=BEETLE_PUNCH_CAPTURE-len;
   /* Reverse is literal reverse indexing through the immutable captured PCM,
    * not a negative playback speed on an advancing live ring buffer. */
   uint32_t a=phase;
   uint32_t b=(phase+1u)%len;
   uint16_t ai=punch_sample(s->punch_start,offset+a);
   uint16_t bi=punch_sample(s->punch_start,offset+b);
   wet_l=punch_lerp(beetle_punch_history[ai][0],beetle_punch_history[bi][0],fract);
   wet_r=punch_lerp(beetle_punch_history[ai][1],beetle_punch_history[bi][1],fract);
   /* Declick across loop boundaries, with a short tail/head crossfade. */
   if(phase>=len-32u){
    uint32_t t=phase-(len-32u),head=t;
    uint16_t hi=punch_sample(s->punch_start,offset+head);
    wet_l=punch_lerp(wet_l,beetle_punch_history[hi][0],(t*256u)/32u);
    wet_r=punch_lerp(wet_r,beetle_punch_history[hi][1],(t*256u)/32u);
   }
   s->punch_position+=128u;
   if(s->punch_position>=len*256u)s->punch_position-=len*256u;
  }else if(mode==2u){
   /* Genuine bipolar ring modulation (x * sine carrier), wet/dry set by
    * ALGORITHM. Apply to the post-drum stereo mix even with zero synthvoices.
    * 440 Hz carrier, 32-bit phase, no allocations or division in ISR. */
   int32_t carrier=(int32_t)track_va_sine[(uint8_t)(s->punch_position>>24u)];
   int32_t intensity=(int32_t)s->punch_param[1];
   wet_l=(dry_l*(100-intensity)+(int32_t)(((int64_t)dry_l*carrier*5)>>16u)*intensity)/100;
   wet_r=(dry_r*(100-intensity)+(int32_t)(((int64_t)dry_r*carrier*5)>>16u)*intensity)/100;
   s->punch_position+=42852281u;
  }else if(mode==4u){
   /* True slowing playback head: read rate decreases from 1.0 to 0.0.
    * The same deceleration changes pitch AND time, not just amplitude. */
   uint32_t age=s->punch_age<tape_duration?s->punch_age:tape_duration;
   uint32_t rate=((tape_duration-age)*128u)/tape_duration;
   uint32_t pos=s->punch_position>>8u;
   uint32_t fraction=s->punch_position&255u;
   pos%=BEETLE_PUNCH_CAPTURE;
   uint16_t a=punch_sample(s->punch_start,pos);
   uint16_t b=punch_sample(s->punch_start,(pos+1u)%BEETLE_PUNCH_CAPTURE);
   wet_l=punch_lerp(beetle_punch_history[a][0],beetle_punch_history[b][0],fraction);
   wet_r=punch_lerp(beetle_punch_history[a][1],beetle_punch_history[b][1],fraction);
   if(pos>=BEETLE_PUNCH_CAPTURE-64u){
    uint32_t edge=pos-(BEETLE_PUNCH_CAPTURE-64u);
    uint16_t c=punch_sample(s->punch_start,edge);
    wet_l=punch_lerp(wet_l,beetle_punch_history[c][0],edge*4u);
    wet_r=punch_lerp(wet_r,beetle_punch_history[c][1],edge*4u);
   }
   wet_l=(wet_l*(int32_t)rate)>>8u;
   wet_r=(wet_r*(int32_t)rate)>>8u;
   s->punch_position+=rate;
  }else if(mode==6u){
   /* SCRATCH: triangular bidirectional transport of a frozen mono-time slice.
    * Playback speed controls both duration and pitch as on a physical deck. */
   const uint32_t length=768u+((uint32_t)s->punch_param[8]*2048u)/100u;
   const uint32_t cycle=2u*(length-1u)*256u;
   uint32_t cursor=s->punch_position%cycle;
   const uint32_t q8=cursor<(length-1u)*256u?cursor:(cycle-cursor);
   const uint32_t pos=q8>>8u,fract=q8&255u;
   uint16_t ai=punch_sample(s->punch_start,pos),bi=punch_sample(s->punch_start,(pos+1u)%length);
   wet_l=punch_lerp(beetle_punch_history[ai][0],beetle_punch_history[bi][0],fract);
   wet_r=punch_lerp(beetle_punch_history[ai][1],beetle_punch_history[bi][1],fract);
   s->punch_position=(cursor+64u+(uint32_t)s->punch_param[8]*4u)%cycle;
  }else if(mode==7u){
   /* GLITCH: small immutable fragments, deterministic pseudo-random jumps,
    * each with a short zero-cross-compatible amplitude taper. */
   const uint32_t len=48u+(uint32_t)(100u-s->punch_param[11])*3u;
   const uint32_t phase=s->punch_position%len;
   if(!phase){uint32_t v=s->punch_fx_seed;v^=v<<13;v^=v>>17;v^=v<<5;s->punch_fx_seed=v?v:0xB142C23Du;
     s->punch_start=(uint16_t)punch_ring((uint32_t)s->punch_write+BEETLE_PUNCH_FRAMES-BEETLE_PUNCH_CAPTURE+(s->punch_fx_seed%(BEETLE_PUNCH_CAPTURE-384u)));}
   uint16_t ai=punch_sample(s->punch_start,phase);
   int32_t taper=256;
   if(phase<12u)taper=(int32_t)phase*256/12;
   else if(phase+12u>=len)taper=(int32_t)(len-phase)*256/12;
   wet_l=beetle_punch_history[ai][0]*taper/256;
   wet_r=beetle_punch_history[ai][1]*taper/256;
   s->punch_position++;
  }else if(mode==5u){
   /* Stutter is a LIVE bipolar amplitude gate, always locked to BPM. */
   uint32_t phase=s->punch_position;
   if(phase>=2u*stutter_half)phase=0u;
   uint32_t gate=0u;
   if(phase<stutter_half){
    gate=256u;
    if(phase<stutter_edge)gate=phase*256u/stutter_edge;
    else if(phase>stutter_half-stutter_edge)gate=(stutter_half-phase)*256u/stutter_edge;
   }
   wet_l=(dry_l*(int32_t)gate)>>8u;
   wet_r=(dry_r*(int32_t)gate)>>8u;
   s->punch_position=phase+1u;
  }
  /* Smooth transitions at key press/release; no full-wet discontinuity. */
  const uint8_t target=mode?255u:0u;
  if(s->punch_wet<target)s->punch_wet++;
  else if(s->punch_wet>target)s->punch_wet--;
  uint32_t wmix=s->punch_wet;
  l=(dry_l*(int32_t)(256u-wmix)+wet_l*(int32_t)wmix)>>8u;
  r=(dry_r*(int32_t)(256u-wmix)+wet_r*(int32_t)wmix)>>8u;
  /* FILTER is an independent smoothed band-pass and keeps ALGORITHM value. */
  if(mask&4u){if(s->punch_filter_wet<224u)s->punch_filter_wet++;}
  else if(s->punch_filter_wet)s->punch_filter_wet--;
  const int32_t base_l=l,base_r=r;
  if(mask&4u){
   uint8_t value=s->punch_param[2];
   int32_t fast=16+value*2,slow=3+value/3;
   s->punch_bp_l+=((l-s->punch_bp_l)*fast)>>8u;
   s->punch_bp_r+=((r-s->punch_bp_r)*fast)>>8u;
   s->punch_hp_l+=((s->punch_bp_l-s->punch_hp_l)*slow)>>8u;
   s->punch_hp_r+=((s->punch_bp_r-s->punch_hp_r)*slow)>>8u;
   l=(s->punch_bp_l-s->punch_hp_l)*2;
   r=(s->punch_bp_r-s->punch_hp_r)*2;
  }else{
   s->punch_bp_l=l;s->punch_bp_r=r;s->punch_hp_l=l;s->punch_hp_r=r;
  }
  const int32_t fw=s->punch_filter_wet;
  l=(base_l*(256-fw)+l*fw)>>8u;
  r=(base_r*(256-fw)+r*fw)>>8u;
  /* Page 2: all heavy modulation uses fixed point, bounded CPU and the
   * existing shared PCM history, with no allocation or per-frame division. */
  if(mask&(1u<<9)){ /* BITCRUSH: 4..16 effective bits */
   uint8_t drop=(uint8_t)((100u-s->punch_param[9])*12u/100u);
   const int32_t grain=(int32_t)(1u<<drop);
   l=(l>>drop)*grain;r=(r>>drop)*grain;
  }
  if(mask&(1u<<10)){ /* PUMP: tempo locked duck/recovery envelope */
   uint32_t period=pump_period?pump_period:1u;
   uint32_t ph=s->punch_pump_phase;
   if(ph>=period)ph=0u;
   uint32_t g=ph<pump_low?64u:(64u+(((ph-pump_low)*pump_slope)>>12u));
   if(g>256u)g=256u;
   l=(l*(int32_t)g)>>8u;r=(r*(int32_t)g)>>8u;
   s->punch_pump_phase=ph+1u;
  }else s->punch_pump_phase=0u;
  if(mask&(1u<<12)){ /* WOW: continuously written tape head and slow delay modulation */
   uint8_t idx=(uint8_t)(s->punch_mod_phase>>24u);
   int32_t mod=((int32_t)track_va_sine[idx]*(int32_t)s->punch_param[12])>>16u;
   uint32_t behind=(uint32_t)(96+mod);
   if(behind<2u)behind=2u;
   uint16_t pos=(uint16_t)punch_ring((uint32_t)s->punch_write+BEETLE_PUNCH_FRAMES-behind);
   l=(l+(int32_t)beetle_punch_history[pos][0])/2;
   r=(r+(int32_t)beetle_punch_history[pos][1])/2;
  }
  if(mask&(1u<<13)){ /* two allpass stages per side, moving phaser not flanger */
   int32_t a=80+(((int32_t)track_va_sine[(uint8_t)(s->punch_mod_phase>>24u)]+32768)*100>>16u);
   for(unsigned j=0;j<2u;j++){
    int32_t yl=s->punch_phase_l[j]-((l*a)>>8u);
    int32_t yr=s->punch_phase_r[j]-((r*a)>>8u);
    s->punch_phase_l[j]=l+((yl*a)>>8u);s->punch_phase_r[j]=r+((yr*a)>>8u);
    l=(l+yl)/2;r=(r+yr)/2;
   }
  }
  if(mask&(1u<<14)){ /* RADIO: narrow AM-like band and deterministic dust */
   int32_t in[2]={l,r};
   for(unsigned ch=0;ch<2u;ch++){
    s->punch_radio_lp[ch]+=(in[ch]-s->punch_radio_lp[ch])>>3u;
    s->punch_radio_hp[ch]+=(s->punch_radio_lp[ch]-s->punch_radio_hp[ch])>>7u;
    int32_t v=s->punch_radio_lp[ch]-s->punch_radio_hp[ch];
    v=(v*3)/2;
    if(ch==0u)l=punch_clip(v);else r=punch_clip(v);
   }
   s->punch_fx_seed^=s->punch_fx_seed<<13u;s->punch_fx_seed^=s->punch_fx_seed>>17u;
   s->punch_fx_seed^=s->punch_fx_seed<<5u;
   int32_t hiss=(int32_t)((s->punch_fx_seed>>23u)&511u)-256;
   l+=hiss;r+=hiss;
  }
  if(mask&(1u<<15)){ /* DJ ISOLATOR: bass/mid/high attenuation modes */
   unsigned kind=1u+s->punch_param[15]/15u;if(kind>7u)kind=7u;
   int32_t in[2]={l,r};
   for(unsigned ch=0;ch<2u;ch++){
    s->punch_iso_lp[ch]+=(in[ch]-s->punch_iso_lp[ch])>>6u;
    s->punch_iso_hp[ch]+=(in[ch]-s->punch_iso_hp[ch])>>2u;
    int32_t bass=s->punch_iso_lp[ch],high=in[ch]-s->punch_iso_hp[ch];
    int32_t mid=in[ch]-bass-high;
    int32_t out=((kind&1u)?0:bass)+((kind&2u)?0:mid)+((kind&4u)?0:high);
    if(ch==0u)l=out;else r=out;
   }
  }
  if(mask&128u){l=0;r=0;}
  dst[f*2u]=(int16_t)punch_clip(l);
  dst[f*2u+1u]=(int16_t)punch_clip(r);
  /* Freeze capture only for REPEAT / TAPE. Ring Mod and STUTTER
   * always process fresh live audio, including drum-only playback. */
  if(mode!=1u&&mode!=4u&&mode!=6u&&mode!=7u){
   if(!s->punch_decimate_phase){
    s->punch_decimate_l=(int16_t)dry_l;
    s->punch_decimate_r=(int16_t)dry_r;
    s->punch_decimate_phase=1u;
   }else{
    /* Two-tap prefilter before decimating prevents excessive aliasing. */
    beetle_punch_history[w][0]=(int16_t)(((int32_t)s->punch_decimate_l+dry_l)/2);
    beetle_punch_history[w][1]=(int16_t)(((int32_t)s->punch_decimate_r+dry_r)/2);
    s->punch_write=(uint16_t)punch_ring((uint32_t)w+1u);
    s->punch_decimate_phase=0u;
   }
  }
  if(mask&((1u<<12)|(1u<<13)))s->punch_mod_phase+=12000u+(uint32_t)s->punch_param[12]*5000u;
  if(s->punch_age<0xFFFFFFFEu)s->punch_age++;
 }
}
