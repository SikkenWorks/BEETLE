/* BEETLE V0.52 regression: instrument selection, legacy projects, master FX
 * DSP recovery and repeated compressor operation. GPL-3.0-only. */
#include "track_chip.h"
#include "track_song.h"
#include "track_project_store.h"
#include "track_mixer.h"
#include "track_drum.h"
#include "track_master_fx.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static struct track_store_project p,q;
static struct track_master_fx_state state;
static uint8_t bytes[TRACK_STORE_MAX];
static int16_t output[2048];
static struct track_chip_engine ch;
static uint64_t power(const int16_t *a,uint32_t n){uint64_t total=0;for(uint32_t i=0;i<n;i++)total+=(int64_t)a[i]*a[i];return total;}
static void put16(uint8_t *a,uint16_t u){a[0]=(uint8_t)u;a[1]=(uint8_t)(u>>8u);}
static void put32(uint8_t *a,uint32_t u){for(uint8_t j=0;j<4u;j++)a[j]=(uint8_t)(u>>(j*8u));}
int main(void){
 struct track_engine e;track_init(&e);track_song_init(&p.song,&e);
 track_mixer_init(&p.mixer);track_master_default(&p.master);
 p.slot=3;p.bpm=120;
 for(uint8_t i=0;i<4;i++){track_va_patch_default(&p.instruments[i].va);p.instruments[i].volume=65u;}
 for(uint8_t i=0;i<TRACK_DRUM_COUNT;i++)track_drum_timbre_default(&p.drum[i]);
 for(uint8_t t=TRACK_CHIP_SID_6581;t<=TRACK_CHIP_SID_NOISE;t++){
  p.instruments[0].type=t;
  uint32_t n=0;assert(track_store_encode(&p,bytes,sizeof(bytes),&n)==0);
  assert(track_store_decode(&q,bytes,n)==0);
  assert(q.instruments[0].type==t);
  assert(q.master.comp_attack==28u && q.master.reverb_spread==50u);
 }
 /* V0.51 data is V9 without the appended 2 bytes; restore header CRCs. */
 uint32_t n=0;assert(track_store_encode(&p,bytes,sizeof(bytes),&n)==0);
 uint32_t legacy=n-2u;put16(bytes+4,9u);put32(bytes+8,legacy);
 put32(bytes+12,track_store_crc32(bytes+24,legacy-24));
 put32(bytes+16,track_store_crc32(bytes,16));
 assert(track_store_decode(&q,bytes,legacy)==0);
 assert(q.master.comp_attack==28u && q.master.reverb_spread==50u);
 track_chip_init(&ch);
 struct track_mixer mix;track_mixer_init(&mix);
 uint64_t first=0,variations=0;
 for(uint8_t t=TRACK_CHIP_SID_6581;t<=TRACK_CHIP_SID_NOISE;t++){
  track_chip_silence(&ch);ch.osc_type[0]=t;ch.env_attack[0]=1u;
  assert(track_chip_note(&ch,0,0,48,110));memset(output,0,sizeof(output));
  track_chip_render(&ch,&mix,output,1024);
  uint64_t now=power(output,2048);
  assert(now>1000000u && track_chip_health(&ch));
  if(t>TRACK_CHIP_SID_6581 && now!=first)variations++;
  if(t==TRACK_CHIP_SID_6581)first=now;
 }
 assert(variations>=2u);
 struct track_master_settings cfg;track_master_default(&cfg);
 cfg.comp_threshold=5u;cfg.comp_ratio=100u;cfg.comp_gain=95u;
 cfg.comp_attack=90u;cfg.reverb_spread=96u;cfg.reverb_wet=75u;
 track_master_dsp_init(&state);
 for(uint16_t round=0;round<250u;round++){
  for(uint16_t i=0;i<2048u;i++)output[i]=(int16_t)((i&1u)?17000:-17000);
  track_master_process(&state,&cfg,output,1024u);
  assert(state.delay_pos<TRACK_MASTER_REVERB_FRAMES);
  assert(state.envelope<=65536u);
 }
 /* Intentional state corruption: DSP heals without application reboot. */
 state.delay_pos=TRACK_MASTER_REVERB_FRAMES+9u;state.guard=0xDEADBEEFu;
 for(uint16_t i=0;i<2048u;i++)output[i]=1500;
 track_master_process(&state,&cfg,output,1024u);
 assert(state.recover_count>=1u && state.guard==TRACK_MASTER_GUARD);
 assert(state.delay_pos<TRACK_MASTER_REVERB_FRAMES && state.recover_frames==0u);
 puts("PASS V0.52: 5 SID variants audible, V9 backward load, V10 round-trip, compressor stress + DSP recovery");
 return 0;
}
