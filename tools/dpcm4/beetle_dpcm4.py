#!/usr/bin/env python3
"""BEETLE's own BD4S adaptive 4-bit mono codec. Offline tools only.

Container: 16-byte <4sBBHII> header (BD4S, version=1, mono=1,
22050 Hz, frames, CRC32 of compressed blocks), then independent blocks.
Block: <hHH> for predictor, quantizer step, frame count (1..256), followed
by ceil((count-1)/2) 4-bit signed delta codes, low nibble first.

DO NOT send these bytes to FM1 flash. Hardware storage/prefetch NOT enabled.
"""
from __future__ import annotations
import argparse
import math
import struct
import wave
import zlib
from pathlib import Path

RATE=22050
BLOCK=256
HEADER=struct.Struct('<4sBBHII')
BH=struct.Struct('<hHH')

class CodecError(ValueError):pass

def clip(x,lo,hi):return min(hi,max(lo,x))

def update_step(step,mag):
    if mag<=1:step-=step//16
    elif mag>=5:step+=step*(mag-4)//4
    return clip(step,8,8192)

def encode_block(samples:list[int])->bytes:
    if not samples or len(samples)>BLOCK:raise CodecError('invalid block length')
    # Fresh adaptive step at each independent block; source is 16-bit signed PCM.
    deltas=[abs(samples[i]-samples[i-1]) for i in range(1,min(len(samples),65))]
    step=clip(max(8, sum(deltas)//max(1,len(deltas))//2),8,8192)
    pred=clip(int(samples[0]),-32768,32767)
    encoded=bytearray(BH.pack(pred,step,len(samples)))
    lo=None
    for s in samples[1:]:
        target=int(s)
        # Deterministic nearest-reconstruction quantization, saturating to
        # -8..+7. Tie-breaker prefers smaller-magnitude delta.
        q=min(range(-8,8),key=lambda k:(abs(target-clip(pred+k*step,-32768,32767)),abs(k)))
        pred=clip(pred+q*step,-32768,32767)
        step=update_step(step,abs(q))
        if lo is None:lo=q&15
        else:
            encoded.append(lo|((q&15)<<4));lo=None
    if lo is not None:encoded.append(lo)
    assert len(encoded)==6+(len(samples)//2)
    return bytes(encoded)

def decode_block(data:bytes)->list[int]:
    if len(data)<6:raise CodecError('truncated block')
    pred,step,frames=BH.unpack_from(data)
    if not 1<=frames<=BLOCK or not 8<=step<=8192 or len(data)!=6+frames//2:raise CodecError('bad block header/length')
    out=[pred]
    for i in range(frames-1):
        nib=(data[6+i//2]>>(4*(i&1)))&15
        q=nib if nib<8 else nib-16
        pred=clip(pred+q*step,-32768,32767)
        out.append(pred)
        step=update_step(step,abs(q))
    return out

def encode(samples:list[int])->bytes:
    if not samples:raise CodecError('no samples')
    if len(samples)>RATE*60*10:raise CodecError('unbounded source')
    payload=b''.join(encode_block(samples[i:i+BLOCK]) for i in range(0,len(samples),BLOCK))
    return HEADER.pack(b'BD4S',1,1,RATE,len(samples),zlib.crc32(payload)&0xffffffff)+payload

def decode(data:bytes)->list[int]:
    if len(data)<HEADER.size:raise CodecError('truncated header')
    magic,version,channels,rate,frames,checksum=HEADER.unpack_from(data)
    if (magic,version,channels,rate)!=(b'BD4S',1,1,RATE) or not frames or frames>RATE*60*10:
        raise CodecError('invalid container')
    if zlib.crc32(data[HEADER.size:])&0xffffffff!=checksum:raise CodecError('CRC mismatch')
    offset=HEADER.size;out=[]
    while len(out)<frames:
        if offset+6>len(data):raise CodecError('truncated block header')
        count=struct.unpack_from('<H',data,offset+4)[0]
        if not 1<=count<=BLOCK or count>frames-len(out):raise CodecError('bad block frame count')
        size=6+count//2
        if offset+size>len(data):raise CodecError('truncated block')
        out.extend(decode_block(data[offset:offset+size]));offset+=size
    if offset!=len(data):raise CodecError('trailing data')
    return out

def wav_mono_22050(path:Path)->list[int]:
    with wave.open(str(path),'rb') as w:
        channels=w.getnchannels();rate=w.getframerate();width=w.getsampwidth()
        if channels not in (1,2) or width!=2 or rate not in (22050,44100):
            raise CodecError('WAV must be 16-bit PCM mono/stereo, 22050 or 44100 Hz (offline stage)')
        raw=w.readframes(w.getnframes())
    values=struct.unpack('<'+'h'*(len(raw)//2),raw)
    if channels==2:
        mono=[(values[i]+values[i+1])//2 for i in range(0,len(values),2)]
    else:mono=list(values)
    if rate==44100:
        # Symmetric 5-tap 1/16 [1,4,6,4,1] low-pass before 2:1 decimation.
        # Simple deterministic host reference. More rates require a properly
        # characterized resampler in the next design stage.
        old=mono;mono=[]
        for i in range(0,len(old),2):
            p=lambda j: old[clip(j,0,len(old)-1)]
            mono.append(clip((p(i-2)+4*p(i-1)+6*p(i)+4*p(i+1)+p(i+2))//16,-32768,32767))
    return mono

def write_wav(path:Path,samples:list[int]):
    with wave.open(str(path),'wb') as w:
        w.setnchannels(1);w.setsampwidth(2);w.setframerate(RATE)
        w.writeframes(struct.pack('<'+'h'*len(samples),*samples))

def main():
    p=argparse.ArgumentParser(description='BEETLE original BD4S offline codec (NOT FM1 flash transfer)')
    p.add_argument('wav',type=Path);p.add_argument('output',type=Path)
    p.add_argument('--preview',type=Path,help='optional reconstructed WAV preview')
    args=p.parse_args()
    pcm=wav_mono_22050(args.wav);packed=encode(pcm);args.output.write_bytes(packed)
    rebuilt=decode(packed)
    if args.preview:write_wav(args.preview,rebuilt)
    error=sum((a-b)**2 for a,b in zip(pcm,rebuilt));power=sum(a*a for a in pcm)
    snr=10*math.log10(power/error) if error and power else float('inf')
    print(f'BEETLE BD4S: {len(pcm)} frames, {len(pcm)/RATE:.3f}s, {len(packed)} bytes, SNR={snr:.1f} dB')
    print('HOST/LAB ONLY: no transfer, no FM1 playback, no new firmware sample capacity')
if __name__=='__main__':main()
