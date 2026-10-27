/* SPDX-License-Identifier: GPL-3.0-only
 * Native Punch-In FX regression: audible amplitude+rate crush, and radio
 * lowcut/noise. Host DSP verification is NOT hardware acoustic validation. */
#include "track_chip.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define N 8192u
#define PI 3.14159265358979323846
static int16_t test_buffer[N*2];
static struct track_chip_engine dsp;
static void sine_fill(double hz,int amplitude){
 for(unsigned i=0;i<N;i++){
  int16_t s=(int16_t)(sin((2*PI*hz*i)/44100.0)*amplitude);
  test_buffer[i*2]=s;test_buffer[i*2+1]=s;
 }
}
static double rms(unsigned start){double ss=0;
 for(unsigned i=start;i<N;i++){double v=test_buffer[i*2];ss+=v*v;}
 return sqrt(ss/(N-start));
}
static uint64_t absolute_change(double hz,int amp,uint16_t effect,uint8_t param){
 sine_fill(hz,amp);static int16_t reference[N*2];memcpy(reference,test_buffer,sizeof(reference));
 track_chip_init(&dsp);dsp.punch_param[effect== (1u<<9)?9:14]=param;
 for(unsigned i=0;i<N;i+=128)track_chip_process_punch(&dsp,effect,test_buffer+2*i,128);
 uint64_t changed=0;for(unsigned i=256;i<N;i++){
  int32_t delta=(int32_t)test_buffer[i*2]-reference[i*2];
  changed+=(uint32_t)(delta<0?-delta:delta);
 }
 assert(track_chip_health(&dsp));return changed;
}
int main(void){
 /* At the default K1=50, a 2kHz waveform must be VERY different after
  * sample-and-hold, even though a pure 8-bit quantizer was nearly inaudible. */
 uint64_t diff=absolute_change(2000.0,1800,1u<<9,50);
 assert(diff>1500000ull);printf("BITCRUSH K1=50 changed=%llu\n",(unsigned long long)diff);
 diff=absolute_change(440.0,900,1u<<9,50);
 assert(diff>250000ull);printf("BITCRUSH quiet wave changed=%llu\n",(unsigned long long)diff);
 /* An actual lowcut attenuates 200Hz much more than 3000Hz. */
 sine_fill(200.0,11000);double inLow=rms(512);
 track_chip_init(&dsp);dsp.punch_param[14]=50;
 for(unsigned i=0;i<N;i+=128)track_chip_process_punch(&dsp,1u<<14,test_buffer+2*i,128);
 double low=rms(512)/inLow;
 sine_fill(3000.0,11000);double inHigh=rms(512);
 track_chip_init(&dsp);dsp.punch_param[14]=50;
 for(unsigned i=0;i<N;i+=128)track_chip_process_punch(&dsp,1u<<14,test_buffer+2*i,128);
 double high=rms(512)/inHigh;
 printf("RADIO lowcut 200Hz=%.3f, 3000Hz=%.3f\n",low,high);
 assert(low<0.48 && high>0.75 && high>2.1*low);
 /* The hiss exists without replacing the underlying music and stays subtle. */
 memset(test_buffer,0,sizeof test_buffer);track_chip_init(&dsp);dsp.punch_param[14]=50;
 for(unsigned i=0;i<N;i+=128)track_chip_process_punch(&dsp,1u<<14,test_buffer+2*i,128);
 double hiss=rms(512);printf("RADIO idle hiss RMS = %.2f / 32768\n",hiss);
 assert(hiss>35 && hiss<230);
 puts("PASS: V1.4.27 real quantization + downsample and lowcut with quiet hiss");
 return 0;
}
