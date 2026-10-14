/* BEETLE V0.44 · PCM ROM digital drum kit with 6-voice 44.1kHz rendering.
 * No legacy quantized samples, no FP/libm, no malloc, no flash writes.
 * Master output attenuation is applied to the completed stereo mix by
 * audio_target.c; per-lane gain here is always <=100%.
 * GPL-3.0-only.
 */
#include "track_drum.h"
#include "track_mixer.h"
#include "track_core.h"
#include "beetle_drum_rom.h"
#include <string.h>
static const char *const names[TRACK_DRUM_COUNT]={"KIK","SNR","HHC","HHO","TM1","TM2","TM3","CRS","RDE","CLP","PC1","PC2","PC3","RIM"};
/* Physical synthesis variations. Snare and hi-hat alternate without repeating. */
static const uint8_t physical[TRACK_DRUM_COUNT]={0u,1u,6u,10u,14u,12u,11u,13u,15u,3u,5u,7u,9u,4u};
const char *track_drum_name(uint8_t id){return id==TRACK_NOTE_STOP?"STP":id<TRACK_DRUM_COUNT?names[id]:"---";}
static uint32_t rnd(uint32_t *state){uint32_t x=*state;x^=x<<13;x^=x>>17;x^=x<<5;*state=x?x:0x9E3779B9u;return *state;}
/* Studio-balanced default digital kit. EQ 50 is neutral; 0..100 is a
 * cut/boost control. All data are project-owned and user-editable. */
static const struct track_drum_timbre beetle_factory[TRACK_DRUM_COUNT]={
 /* ATT DEC SUS TAIL LOW MID HIGH GAIN PITCH */
 {0,66,0,53,72,48,59,100, 0}, /* kick: round sub + defined beater */
 {0,62,0,54,48,64,72,100, 0}, /* snare: shell + rattling wires */
 {0,43,0,32,35,48,73, 88, 0}, /* HHC: very short, bright */
 {0,67,0,72,35,48,72, 87, 0}, /* HHO: smooth extended wash */
 {0,64,0,62,70,57,47, 94,-1}, /* low tom */
 {0,60,0,56,65,58,49, 94, 0}, /* mid tom */
 {0,55,0,49,59,60,51, 93, 1}, /* high tom */
 {0,77,0,79,35,53,65, 88, 0}, /* crash */
 {0,83,0,90,41,57,56, 91, 0}, /* ride bell + wash */
 {0,53,0,52,39,61,69, 99, 0}, /* clap: staggered bursts */
 {0,55,0,45,55,61,45, 95, 0}, /* woodblock */
 {0,70,0,64,40,59,58, 90, 0}, /* cowbell */
 {0,43,0,37,32,57,67, 94, 0}, /* clave */
 {0,41,0,31,40,57,75, 91, 0}  /* rim */
};
void track_drum_component_default(struct track_drum_timbre *p,uint8_t id){
 if(p&&id<TRACK_DRUM_COUNT)*p=beetle_factory[id];
}
void track_drum_timbre_default(struct track_drum_timbre *p){
 if(!p)return;*p=(struct track_drum_timbre){0u,50u,0u,50u,50u,50u,50u,100u,0};
}
void track_drum_init(struct track_drum_engine *d){
 if(!d)return;memset(d,0,sizeof(*d));d->rng=0xA17C9E23u;d->sn_last=255u;d->hh_last=255u;d->step_gain=100u;
 for(uint8_t i=0u;i<TRACK_DRUM_COUNT;i++)d->components[i]=beetle_factory[i];
}

static uint8_t choose(struct track_drum_engine *d,uint8_t index){
 if(index==1u){uint8_t id=(uint8_t)(1u+(rnd(&d->rng)&1u));if(id==d->sn_last)id=(uint8_t)(3u-id);d->sn_last=id;return id;}
 if(index==6u){uint8_t id=(rnd(&d->rng)&1u)?8u:6u;if(id==d->hh_last)id=id==6u?8u:6u;d->hh_last=id;return id;}
 return index;
}
static uint8_t hat(uint8_t id){return id==6u||id==8u||id==10u;}

/* µ-law ROM at 22050 Hz, linearly interpolated at 44100 Hz. The original
 * attack/body/tail is rendered offline by tools/create_v044_kit.py. This
 * replaces the former same-sine-plus-noise engine, without touching flash
 * storage or the persisted drum component structure. */
static const uint32_t tune_q16[73]={8192u,8679u,9195u,9742u,10321u,10935u,11585u,12274u,13004u,13777u,14596u,15464u,16384u,17358u,18390u,19484u,20643u,21870u,23170u,24548u,26008u,27554u,29193u,30929u,32768u,34716u,36781u,38968u,41285u,43740u,46341u,49097u,52016u,55109u,58386u,61858u,65536u,69433u,73562u,77936u,82570u,87480u,92682u,98193u,104032u,110218u,116772u,123715u,131072u,138866u,147123u,155872u,165140u,174960u,185364u,196386u,208064u,220436u,233544u,247431u,262144u,277732u,294247u,311744u,330281u,349920u,370728u,392772u,416128u,440872u,467088u,494862u,524288u};
static uint32_t rate_step(int8_t semi){
 if(semi<-36)semi=-36;if(semi>36)semi=36;
 return (uint32_t)(((uint64_t)32768u*tune_q16[(uint8_t)(semi+36)]+32768u)>>16u);
}
/* Internal physical drum IDs are deliberately unchanged for sequencer,
 * alternative snare/hats, and legacy test compatibility. The ROM bank holds
 * the 14 user-visible instruments, so map 16 physical IDs to 14 ROM entries. */
static const uint8_t rom_index[16]={0u,1u,14u,9u,13u,10u,2u,11u,15u,12u,3u,4u,5u,7u,6u,8u};
static uint32_t sample_frames(uint8_t id){const uint8_t i=rom_index[id];return beetle_drum_offsets[i+1u]-beetle_drum_offsets[i];}
static void update_length(struct track_drum_voice *v,const struct track_drum_timbre *p){
 uint32_t step=v->phase_step;if(step==0u)step=1u;
 uint32_t playback=(sample_frames(v->id)*512u)/(step>>7u);
 /* Tail control changes the one-shot length without reading out of ROM. */
 playback=playback*(50u+p->release)/100u;
 if(playback>350000u)playback=350000u;
 v->length=(uint32_t)(playback<64u?64u:playback);
 uint32_t decay=(uint32_t)(150u-p->decay); /* 50 default = gentle 1-fold attenuation */
 uint32_t loss=(decay*65536u)/(100u*(v->length?v->length:1u));
 if(loss<1u)loss=1u;if(loss>65535u)loss=65535u;
 v->env_loss_q16=(uint16_t)loss;
}
int track_drum_trigger(struct track_drum_engine *d,uint8_t index){
 if(!d||index>=TRACK_DRUM_COUNT)return 0;
 const uint8_t id=choose(d,physical[index]);
 if(hat(id))for(uint8_t i=0;i<TRACK_DRUM_POLY;i++){
  struct track_drum_voice *old=&d->voices[i];
  if(old->active&&hat(old->id)&&(!old->release_frames||old->release_frames>48u))old->release_frames=48u;
 }
 unsigned target=TRACK_DRUM_POLY;
 for(unsigned i=0;i<TRACK_DRUM_POLY;i++)if(!d->voices[i].active){target=i;break;}
 if(target==TRACK_DRUM_POLY){uint32_t oldest=0xFFFFFFFFu;
  for(unsigned i=0;i<TRACK_DRUM_POLY;i++){
   if(d->voices[i].release_frames){target=i;break;}
   if(d->voices[i].sequence<oldest){oldest=d->voices[i].sequence;target=i;}
  }
 }
 struct track_drum_voice *v=&d->voices[target];
 *v=(struct track_drum_voice){0};
 v->sequence=++d->trigger_serial;v->id=id;v->component=index;
 const struct track_drum_timbre *p=&d->components[index];
 v->gain_q8=(uint16_t)(256u*p->gain/100u);
 v->env_q24=16777215u;v->noise_state=rnd(&d->rng);
 v->phase_step=rate_step(p->pitch);v->pitch_semitones=p->pitch;v->active=1u;
 update_length(v,p);return 1;
}
int track_drum_trigger_pair(struct track_drum_engine *d,uint8_t first,uint8_t second){
 const uint8_t a=first<TRACK_DRUM_COUNT,b=second<TRACK_DRUM_COUNT;
 if(!a&&!b)return 0;
 if(a&&b){
  if(first==second&&hat(physical[first]))return track_drum_trigger(d,first);
  if(first==2u&&second==3u)return track_drum_trigger(d,first);
  if(first==3u&&second==2u)return track_drum_trigger(d,second);
 }
 int n=0;if(a)n+=track_drum_trigger(d,first);if(b)n+=track_drum_trigger(d,second);return n;
}
int track_drum_trigger_pair_percent(struct track_drum_engine *d,uint8_t a,uint8_t b,uint8_t percent){
 if(!d||percent==0u)return 0;if(percent>100u)percent=100u;
 uint32_t serial=d->trigger_serial;int n=track_drum_trigger_pair(d,a,b);
 for(uint8_t i=0;i<TRACK_DRUM_POLY;i++){
  struct track_drum_voice *v=&d->voices[i];
  if(v->active&&(int32_t)(v->sequence-serial)>0)v->gain_q8=(uint16_t)((v->gain_q8*(uint32_t)percent+50u)/100u);
 }return n;
}
int track_drum_trigger_pair_tuned(struct track_drum_engine *d,uint8_t a,uint8_t b,int8_t semitones){
 if(!d)return 0;if(semitones<-12)semitones=-12;if(semitones>12)semitones=12;
 uint32_t before=d->trigger_serial;int n=track_drum_trigger_pair(d,a,b);
 for(uint8_t i=0;i<TRACK_DRUM_POLY;i++){
  struct track_drum_voice *v=&d->voices[i];
  if(v->active&&(int32_t)(v->sequence-before)>0){
   int32_t combined=(int32_t)v->pitch_semitones+semitones;
   if(combined<-36)combined=-36;if(combined>36)combined=36;
   v->pitch_semitones=(int8_t)combined;v->phase_step=rate_step(v->pitch_semitones);
   update_length(v,&d->components[v->component]);
  }
 }return n;
}
void track_drum_stop(struct track_drum_engine *d){if(d)for(unsigned i=0;i<TRACK_DRUM_POLY;i++)d->voices[i].active=0u;}
static int16_t clamp16(int32_t x){return (int16_t)(x>32767?32767:x<-32768?-32768:x);}
static int32_t get_pcm(uint8_t id,uint32_t sample){
 uint8_t i=rom_index[id];
 uint32_t off=beetle_drum_offsets[i]+sample;
 if(off>=beetle_drum_offsets[i+1u])return 0;
 return beetle_ulaw_decode[beetle_drum_rom[off]];
}
void track_drum_render(struct track_drum_engine *d,const struct track_mixer *mix,int16_t *dst,uint32_t frames){
 if(!d||!dst)return;
 const struct track_mixer_channel *m=mix?&mix->ch[TRACK_DRUM_CHANNEL]:0;
 uint8_t any_solo=0u;if(mix)for(uint8_t i=0;i<TRACK_CHANNELS;i++)any_solo|=mix->ch[i].solo;
 const uint32_t gain=!m?80u:(m->mute||(any_solo&&!m->solo)?0u:m->volume);
 const int32_t pan=m?m->pan:0;
 for(uint32_t f=0;f<frames;f++){
  int32_t mono=0;
  for(uint8_t i=0;i<TRACK_DRUM_POLY;i++){
   struct track_drum_voice *v=&d->voices[i];if(!v->active)continue;
   const struct track_drum_timbre *ctrl=&d->components[v->component<TRACK_DRUM_COUNT?v->component:0u];
   const uint32_t idx=v->phase>>16u;
   if(idx>=sample_frames(v->id)||v->age>=v->length){v->active=0u;continue;}
   const uint32_t frac=v->phase&65535u;
   const int32_t one=get_pcm(v->id,idx),two=get_pcm(v->id,idx+1u);
   const int32_t wave=one+(int32_t)(((int64_t)(two-one)*frac)>>16u);
   /* Real 3-band sample EQ (low/mid/high); all voice state is independent. */
   v->noise_lp+= (wave-v->noise_lp)>>5u; /* approx 220 Hz */
   v->noise_lp2+=(wave-v->noise_lp2)>>2u;/* approx 2 kHz */
   int32_t low=v->noise_lp,mid=v->noise_lp2-v->noise_lp,hi=wave-v->noise_lp2;
   int32_t level=(int32_t)(((int64_t)low*(50u+ctrl->eq_low)+(int64_t)mid*(50u+ctrl->eq_mid)+(int64_t)hi*(50u+ctrl->eq_high))/100u);
   if(v->age<16u)level=level*(int32_t)(v->age+1u)/16;
   if(ctrl->attack && v->age<(uint32_t)ctrl->attack*25u){uint32_t a=v->age*32768u/((uint32_t)ctrl->attack*25u);level=(int32_t)(((int64_t)level*a)>>15u);}
   level=(int32_t)(((int64_t)level*v->env_q24)>>24u);
   level=(int32_t)(((int64_t)level*v->gain_q8)>>8u);
   if(v->release_frames)level=level*v->release_frames/48;
   mono+=level/3;  /* six simultaneous drumvoices and synthvoice headroom */
   v->phase+=v->phase_step;v->age++;
   uint32_t loss=(uint32_t)(((uint64_t)v->env_q24*v->env_loss_q16+32768u)>>16u);
   if(!loss&&v->env_q24)loss=1u;
   uint32_t floor=(uint32_t)ctrl->sustain*16777215u/100u;
   uint32_t next=v->env_q24>loss?v->env_q24-loss:0u;
   v->env_q24=next<floor?floor:next;
   if(v->release_frames){v->release_frames--;if(!v->release_frames)v->active=0u;}
  }
  mono=(mono*(int32_t)gain*(int32_t)d->step_gain)/10000;
  uint32_t peak=(uint32_t)(mono<0?-(int64_t)mono:mono);
  if(peak>32767u)peak=32767u;
  if(peak>track_lane_pcm_peak[TRACK_DRUM_CHANNEL])track_lane_pcm_peak[TRACK_DRUM_CHANNEL]=(uint16_t)peak;
  int32_t l=dst[f*2u]+mono*(100-pan)/100;
  int32_t r=dst[f*2u+1u]+mono*(100+pan)/100;
  dst[f*2u]=clamp16(l);dst[f*2u+1u]=clamp16(r);
 }
}
