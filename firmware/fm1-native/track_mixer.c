/* SPDX-License-Identifier: GPL-3.0-only */
#include "track_mixer.h"
volatile uint16_t track_lane_pcm_peak[TRACK_CHANNELS];
static int32_t clip(int32_t v,int32_t lo,int32_t hi){return v<lo?lo:v>hi?hi:v;}
void track_mixer_init(struct track_mixer *m){if(!m)return;for(uint8_t i=0;i<TRACK_MIXER_CHANNELS;i++){m->ch[i].volume=80u;m->ch[i].pan=0;m->ch[i].mute=0;m->ch[i].solo=0;m->ch[i].swing=0;}m->row=0;m->col=0;}
void track_mixer_move(struct track_mixer *m,int32_t rows,int32_t columns){if(!m)return;m->row=(uint8_t)clip((int32_t)m->row+rows,0,TRACK_MIXER_CHANNELS-1);m->col=(uint8_t)clip((int32_t)m->col+columns,0,4);}
int track_mixer_change(struct track_mixer *m,int32_t delta){if(!m||!delta||m->row>=TRACK_MIXER_CHANNELS||m->col>=TRACK_MIXER_FIELDS)return 0;struct track_mixer_channel *s=&m->ch[m->row];switch(m->col){case 0:s->volume=(uint8_t)clip((int32_t)s->volume+delta,0,100);break;case 1:s->pan=(int8_t)clip((int32_t)s->pan+delta,-100,100);break;case 2:{uint8_t desired=delta>0?1u:0u;if(s->mute==desired)return 0;s->mute=desired;break;}
 case 3:{uint8_t desired=delta>0?1u:0u;if(s->solo==desired)return 0;s->solo=desired;break;}
 case 4:{int32_t desired=clip((int32_t)s->swing+delta*2,0,100);if(s->swing==(uint8_t)desired)return 0;s->swing=(uint8_t)desired;break;}}
 return 1;}
uint8_t track_mixer_velocity(const struct track_mixer *m,uint8_t ch,uint8_t input){if(!m||ch>=TRACK_MIXER_CHANNELS)return 0;uint8_t any_solo=0;for(uint8_t i=0;i<TRACK_MIXER_CHANNELS;i++)any_solo|=m->ch[i].solo;if(m->ch[ch].mute||(any_solo&&!m->ch[ch].solo))return 0;uint32_t v=((uint32_t)input*m->ch[ch].volume+50u)/100u;return (uint8_t)(v>127u?127u:v);}
