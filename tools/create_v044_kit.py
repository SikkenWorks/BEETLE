#!/usr/bin/env python3
"""BEETLE V0.44 factory ROM kit. Original deterministic offline drum synthesis.
No third-party recordings. Generates band-limited 22050-Hz µ-law one-shots and 44.1k WAV references.
Source generators are intentionally kept in the ZIP for reproducibility.
"""
import numpy as np, math, wave, pathlib, hashlib, json
from scipy import signal
ROOT=pathlib.Path(__file__).resolve().parents[2]
FS=22050
RNG=np.random.default_rng(2044033)
NAMES=['KIK','SNR','HHC','HHO','TM1','TM2','TM3','CRS','RDE','CLP','PC1','PC2','PC3','RIM']
DURS=[.66,.46,.090,.72,.55,.49,.42,1.18,1.33,.45,.27,.47,.23,.20]

def white(n): return RNG.standard_normal(n)
def filt(y,cut,mode='highpass',order=3):
 return signal.sosfilt(signal.butter(order,cut,fs=FS,btype=mode,output='sos'),y)
def bp(y,l,h,order=2): return filt(filt(y,l,'highpass',order),h,'lowpass',order)
def expenv(t,tau):return np.exp(-t/tau)
def bend(t,start,end,tau=.050):
 hz=end+(start-end)*np.exp(-t/tau)
 return np.sin(2*np.pi*np.cumsum(hz)/FS)
def partials(t, freq, ratios, amps, tau):
 y=np.zeros(t.shape,dtype='float64')
 for i,(r,a) in enumerate(zip(ratios,amps)):
  y+=a*np.sin(2*np.pi*(freq*r)*t+i*.633+(.03*np.sin(2*np.pi*(1+i)*t)))
 return y*expenv(t,tau)
def burst(t,loc,decay):
 z=t-loc
 return np.where(z>=0,np.exp(-np.maximum(z,0)/decay),0.0)
def body_tom(t,base):
 y=(.79*bend(t,base*2.2,base,.036)+.15*bend(t,base*2.32,base*1.39,.059)+.07*np.sin(2*np.pi*base*2.1*t))*expenv(t,.25 if base<150 else .195)
 y+=.19*bp(white(len(t)),900,8000)*burst(t,0,.008)
 return y

def render(id,t):
 n=len(t)
 if id==0:  # 90s punchy digital kick, harmonic bass, bright transient
  bass=(.86*bend(t,184,52,.021)+.10*bend(t,290,100,.028))*(.72*expenv(t,.17)+.28*expenv(t,.29))
  attack=bp(white(n),900,9000)*.28*burst(t,0,.004)
  y=bass+attack+.1*bend(t,900,180,.002)*burst(t,0,.006)
 elif id==1:  # snare: distinct shell and wire release
  shell=(.46*bend(t,265,177,.027)+.23*np.sin(2*np.pi*330*t)+.06*np.sin(2*np.pi*442*t))*expenv(t,.085)
  noise=bp(white(n),900,10300)*(.54*expenv(t,.125)+.14*expenv(t,.24))
  wire=bp(white(n),3300,10200)*(.28*burst(t,.008,.025)+.14*burst(t,.026,.04))
  y=shell+noise+wire
 elif id in (2,3): # hats: 6 uneven metal oscillators mixed with band limited sizzle
  ratios=[1,1.276,1.485,1.931,2.437,3.111]
  phase=np.zeros(n)
  freq=580 if id==2 else 505
  for k,r in enumerate(ratios):
   ph=2*np.pi*freq*r*t+(k+1)*.789
   phase+=np.sign(np.sin(ph))*.12
  metal=bp(phase,2600,10100)
  noise=bp(white(n),3200,10000)
  if id==2: amp=.42*expenv(t,.019)+.24*expenv(t,.032)
  else: amp=.45*expenv(t,.18)+.20*expenv(t,.31)
  y=(metal*.62+noise*.52)*amp*2.0
 elif id in (4,5,6): # acoustic modal toms, distinctive tuning
  base={4:104,5:143,6:191}[id]
  y=body_tom(t,base)
 elif id==7: # crash: inharmonic cymbal wash + fast metallic transient
  ratios=[1,1.301,1.561,1.993,2.497,2.881,3.679,4.132]
  metallic=partials(t,850,ratios,[.23,.20,.19,.15,.15,.11,.1,.08],.36)
  wash=bp(white(n),1100,10000)*(.45*expenv(t,.38)+.18*expenv(t,.76))
  shimmer=bp(white(n),4600,10200)*(.26*burst(t,.05,.32)+.17*burst(t,.23,.39))
  y=metallic+wash+shimmer
 elif id==8: # ride: woody stick ping + complex persistent bed, NOT crash upshift
  ping=partials(t,1140,[1,2.64,3.89,5.13],[.54,.27,.20,.09],.29)
  bell=partials(t,1740,[1,1.37,2.13],[.24,.14,.10],.65)
  noise=bp(white(n),2100,10000)*(.16*expenv(t,.57)+.10*expenv(t,.87))
  y=ping+bell+noise
 elif id==9: # clap: multi-person early reflection transients
  w=bp(white(n),520,9500)
  w2=bp(white(n),1000,9600)
  y=w*(.38*burst(t,0,.007)+.5*burst(t,.011,.009)+.60*burst(t,.024,.010)+.7*burst(t,.037,.028)+.22*burst(t,.055,.15))
  y+=.08*w2*burst(t,.017,.19)
 elif id==10: # woody knock
  y=partials(t,690,[1,1.51,2.23],[.7,.35,.21],.065)+bp(white(n),800,6500)*.16*burst(t,0,.006)
 elif id==11: # cowbell bimodal with ringy decay
  y=partials(t,570,[1,1.46,2.13,3.32],[.47,.38,.20,.12],.26)+bp(white(n),2500,9800)*.1*burst(t,0,.008)
 elif id==12: # clave digital woody tap
  y=partials(t,2100,[1,1.22,1.71],[.70,.38,.16],.048)+bp(white(n),2200,8500)*.27*burst(t,0,.003)
 else: # rimshot: rim resonator, snappy initial body
  y=partials(t,1560,[1,1.58,2.52],[.42,.26,.16],.048)+bp(white(n),500,10000)*.45*burst(t,0,.005)
 # preserve natural relationships; short fade-in 0.23ms avoids DAC discontinuity
 y=np.nan_to_num(y)
 y*=np.minimum(1,t/.00023)
 tail=min(550,round(.025*FS))
 y[-tail:]*=np.linspace(1,0,tail)**2
 return y

def ulaw_encode(y):
 # 255 µ-law companding, full scale 16-bit, unbiased approx, embedded no runtime floating point
 f=np.sign(y)*np.log1p(255*np.abs(y))/np.log(256)
 return np.uint8(np.clip(np.round(f*127.5+127.5),0,255))

def ulaw_decode(s):
 q=(s.astype('float64')-127.5)/127.5
 return np.sign(q)*np.expm1(np.abs(q)*np.log(256))/255

def main():
 out=ROOT/'rom_audio';out.mkdir(exist_ok=True)
 allout=[]
 for i,(nm,dur) in enumerate(zip(NAMES,DURS)):
  t=np.arange(round(dur*FS),dtype='float64')/FS
  x=render(i,t)
  # sensible relative peak levels (not 14 independent normalize-to-fullscale)
  peaks=[.86,.76,.47,.44,.65,.65,.64,.48,.46,.73,.55,.51,.60,.69]
  maxamp=max(1e-8,np.max(np.abs(x)))
  x=x/maxamp*peaks[i]
  b=ulaw_encode(x)
  allout.append(b)
  with wave.open(str(out/f'{i:02d}-{nm}.wav'),'wb') as w:
   # resample reference to hardware 44.1k with linear interpolation (same as firmware path)
   y=ulaw_decode(b)
   hi=np.empty(len(y)*2,dtype=np.float64)
   hi[::2]=y;hi[1::2]=(y+np.r_[y[1:],0])/2
   pcm=np.int16(np.clip(np.round(hi*32767),-32768,32767))
   w.setnchannels(1);w.setsampwidth(2);w.setframerate(FS*2);w.writeframes(pcm.astype('<i2').tobytes())
 # Two genuine round-robin recordings synthesized with a different seeded noise
 # stream; no aliasing/copying the exact same PCM on repeated snare/hat hits.
 for idx,label in ((1,'SNARE-ALT'),(2,'HHC-ALT')):
  t=np.arange(round(DURS[idx]*FS),dtype='float64')/FS
  x=render(idx,t)
  peak=(.74 if idx==1 else .45)
  x=x/max(1e-8,np.max(np.abs(x)))*peak
  b=ulaw_encode(x);allout.append(b)
  with wave.open(str(out/f'{len(allout)-1:02d}-{label}.wav'),'wb') as w:
   y=ulaw_decode(b);hi=np.empty(len(y)*2,dtype=np.float64)
   hi[::2]=y;hi[1::2]=(y+np.r_[y[1:],0])/2
   pcm=np.int16(np.clip(np.round(hi*32767),-32768,32767))
   w.setnchannels(1);w.setsampwidth(2);w.setframerate(FS*2);w.writeframes(pcm.astype('<i2').tobytes())
 blob=np.concatenate(allout)
 starts=np.cumsum([0]+[len(b) for b in allout])
 hdr=ROOT/'source'/'firmware'/'fm1-native'/'beetle_drum_rom.h'
 hdr.parent.mkdir(parents=True,exist_ok=True)
 lines=['/* BEETLE V0.44: original generated one-shot ROM, 22.05kHz 8-bit µ-law. */','#ifndef BEETLE_DRUM_ROM_H','#define BEETLE_DRUM_ROM_H','#include <stdint.h>',f'#define BEETLE_DRUM_ROM_BYTES {len(blob)}u','#define BEETLE_DRUM_ROM_RATE 22050u','#define BEETLE_DRUM_ROM_COUNT 16u','extern const uint8_t beetle_drum_rom[BEETLE_DRUM_ROM_BYTES];','extern const uint32_t beetle_drum_offsets[BEETLE_DRUM_ROM_COUNT+1u];','extern const int16_t beetle_ulaw_decode[256];','#endif']
 hdr.write_text('\n'.join(lines)+'\n')
 decode=np.clip(np.round(ulaw_decode(np.arange(256))*32767),-32768,32767).astype('int16')
 c=hdr.with_suffix('.c')
 with c.open('w') as f:
  f.write('/* Generated by tools/create_v044_kit.py: original, CC0-free source; GPL-3.0-only as firmware. */\n#include "beetle_drum_rom.h"\n')
  f.write('const uint32_t beetle_drum_offsets[BEETLE_DRUM_ROM_COUNT+1u]={'+','.join(str(int(a))+'u' for a in starts)+'};\n')
  f.write('const int16_t beetle_ulaw_decode[256]={'+','.join(str(int(a)) for a in decode)+'};\n')
  f.write('const uint8_t beetle_drum_rom[BEETLE_DRUM_ROM_BYTES]={\n')
  for k in range(0,len(blob),40): f.write(','.join(str(int(a)) for a in blob[k:k+40])+',\n')
  f.write('};\n')
 stats={'generator':'Original deterministic BEETLE offline drum synth; no third-party recordings','rate':FS,'format':'mu-law 8 bit decoded with 256-byte lookup','bank_bytes':len(blob),'items':[{'name':n,'frames':len(b),'duration_s':len(b)/FS,'sha256':hashlib.sha256(b.tobytes()).hexdigest()} for n,b in zip(NAMES+["SNR-ALT","HHC-ALT"],allout)]}
 (out/'ROM-MANIFEST.json').write_text(json.dumps(stats,indent=2))
 print('ROM bytes',len(blob),'c source',c.stat().st_size)
if __name__=='__main__':main()
