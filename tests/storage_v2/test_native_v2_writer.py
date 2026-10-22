#!/usr/bin/env python3
"""Native C Storage V2 writer integration against original NOR emulator.
Checks byte-exact project/sample round trips, generational commits, power loss,
and fail-closed incomplete writes. No USB access, no actual FM1 flash.
"""
import ctypes, subprocess, tempfile, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
P=ROOT/'firmware/fm1-native'
sys.path.insert(0,str(ROOT/'tools/storage_v2'))
from storage_v2_sim import Nor,Storage,ProjectCodec,PowerFailure,NotEnoughSafeSpace
U8=ctypes.c_uint8
RD=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,ctypes.POINTER(U8),ctypes.c_uint32)
PG=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,ctypes.POINTER(U8),ctypes.c_uint32)
ER=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32)
class Device(ctypes.Structure):
 _fields_=[('read',ctypes.c_void_p),('context',ctypes.c_void_p),('index_address',ctypes.c_uint32),('generation',ctypes.c_uint32)]
class Writer(ctypes.Structure):
 _fields_=[('device',Device),('program',ctypes.c_void_p),('erase',ctypes.c_void_p),('context',ctypes.c_void_p),('domain',U8*72),('occupied',ctypes.c_uint16*72),('active_meta',U8)]

with tempfile.TemporaryDirectory() as tmp:
 so=Path(tmp)/'native.so'
 subprocess.run(['cc','-std=c11','-O2','-fPIC','-shared','-Wall','-Wextra','-Werror',str(P/'beetle_storage_v2_writer.c'),str(P/'beetle_storage_v2_reader.c'),str(P/'beetle_project_pack_v2.c'),'-o',str(so)],check=True)
 lib=ctypes.CDLL(str(so));codec=ProjectCodec(lib)
 lib.beetle_v2_writer_open.argtypes=[ctypes.POINTER(Writer),RD,PG,ER,ctypes.c_void_p,ctypes.POINTER(U8),ctypes.POINTER(U8)]
 lib.beetle_v2_write_project.argtypes=[ctypes.POINTER(Writer),U8,ctypes.POINTER(U8),ctypes.c_uint32,ctypes.POINTER(U8),ctypes.POINTER(U8),ctypes.POINTER(U8)]
 lib.beetle_v2_write_sample.argtypes=lib.beetle_v2_write_project.argtypes
 lib.beetle_v2_remove_sample.argtypes=[ctypes.POINTER(Writer),U8,ctypes.POINTER(U8),ctypes.POINTER(U8),ctypes.POINTER(U8)]
 lib.beetle_v2_collect.argtypes=[ctypes.POINTER(Writer),ctypes.POINTER(U8),ctypes.POINTER(U8)]
 
 baseline=Nor(authorized=True);store=Storage(baseline,codec)
 nuggie=(ROOT/'tests/fixtures/NUGGIE-SLOT-07.trk').read_bytes()
 mountain=(ROOT/'tests/fixtures/MOUNTAINKING-SLOT-01.trk').read_bytes()
 for k in range(12):store.save_project(k, mountain if k==0 else nuggie)
 store.replace_sample(0,b'DPCM4 INITIAL SAMPLE '*200)
 original=bytes(baseline.bytes)
 def make_fixture(image):
  nor=Nor(image,authorized=True);w=Writer()
  idx=(U8*4096)();older=(U8*4096)();page=(U8*256)()
  def rd(_ctx,physical,dst,n):
   rel=physical-0x97000
   if rel<0 or rel+n>len(nor.bytes):print("RD OOB",hex(physical),n);return -1
   
   ctypes.memmove(dst,bytes(nor.bytes[rel:rel+n]),n);return 0
  def pg(_ctx,physical,src,n):
   try:
    nor.program_page(physical-0x97000,bytes(src[:n]));return 0
   except (PowerFailure,ValueError) as e:
    return -1
  def er(_ctx,physical):
   try:nor.erase_sector((physical-0x97000)//4096);return 0
   except (PowerFailure,ValueError) as e:
    return -1
  cb=(RD(rd),PG(pg),ER(er))
  assert lib.beetle_v2_writer_open(ctypes.byref(w),*cb,None,idx,older)==0
  return nor,w,idx,older,page,cb
 def check(nor, expected_project,expected_sample):
  s=Storage(Nor(bytes(nor.bytes),authorized=True),codec)
  assert s.load_project(0)==expected_project
  assert s.load_sample(0)==expected_sample
  assert s.load_project(1)==nuggie
  assert len(s.current.projects)==12 and len(s.current.samples)==13
  return s
 def do_project(w,idx,other,page,b):
  buf=(U8*len(b)).from_buffer_copy(b)
  return lib.beetle_v2_write_project(ctypes.byref(w),0,buf,len(b),idx,other,page)
 def do_sample(w,idx,other,page,b):
  buf=(U8*len(b)).from_buffer_copy(b)
  return lib.beetle_v2_write_sample(ctypes.byref(w),0,buf,len(b),idx,other,page)
 original_sample=b'DPCM4 INITIAL SAMPLE '*200
 new_sample=bytes((i*17+(i//31))%256 for i in range(13500))
 nor,w,idx,other,page,cb=make_fixture(original)
 assert (result := do_project(w,idx,other,page,codec.pack(nuggie)))==0, result
 assert do_sample(w,idx,other,page,new_sample)==0
 check(nor,nuggie,new_sample)
 # Deleting a global sample never mutates ANY project slot.
 assert lib.beetle_v2_remove_sample(ctypes.byref(w),0,idx,other,page)==0
 after=Storage(Nor(bytes(nor.bytes),authorized=True),codec)
 assert after.load_sample(0) is None and after.load_project(0)==nuggie
 assert all(after.load_project(i) is not None for i in range(12))
 print('PASS: native C write project, replace global sample, delete and independent project persistence')
 # All 13 slots and 12 projects in the SAME physical 288KiB store.
 flash,w2,ix,old,pgbuf,cb2=make_fixture(original)
 blobs=[bytes(((j*7)+s*11)%256 for j in range(11200)) for s in range(13)]
 for sl,bdat in enumerate(blobs):
  cbuf=(U8*len(bdat)).from_buffer_copy(bdat)
  rc=lib.beetle_v2_write_sample(ctypes.byref(w2),sl,cbuf,len(bdat),ix,old,pgbuf)
  assert rc==0,(sl,rc)
 allslots=Storage(Nor(bytes(flash.bytes),authorized=True),codec)
 for s,bdat in enumerate(blobs):assert allslots.load_sample(s)==bdat
 for s in range(12):assert allslots.load_project(s)==(mountain if s==0 else nuggie)
 assert allslots.domain_stats()['shared_legacy_sectors']==0
 print('PASS: native C writer preserves 12 projects alongside all 13 user samples')
 # Oversized sample must be rejected BEFORE writing ANY flash bytes.
 before=bytes(flash.bytes)
 big=(U8*(128*4068))()
 rc=lib.beetle_v2_write_sample(ctypes.byref(w2),12,big,len(big),ix,old,pgbuf)
 assert rc<0 and bytes(flash.bytes)==before,(rc,len(flash.bytes))
 print('PASS: full storage is refused without a single NOR mutation')
 # Compare full crash injection across all NOR mutations for a replacement.
 ref_nor,ref_w,a,b,p,callbacks=make_fixture(original)
 assert do_sample(ref_w,a,b,p,new_sample)==0
 operations=ref_nor.mutations
 for cut in range(1,operations+1):
  flash,writer,a,b,p,callbacks=make_fixture(original)
  flash.fault_after=cut
  rc=do_sample(writer,a,b,p,new_sample)
  assert rc!=0
  # Both previous and new committed values are acceptable after simulated power cut.
  try:
   got=Storage(Nor(bytes(flash.bytes),authorized=True),codec)
   assert got.load_project(0)==mountain
   assert got.load_sample(0) in (original_sample,new_sample)
  except Exception as e:
   raise AssertionError(f'power cut {cut}/{operations}: {e}') from e
 print(f'PASS: power cut after every one of {operations} native flash mutations; old project and old/new sample recover')
 # Streaming upload: no full sample RAM, 24-byte USB chunks cross 4068-byte
 # native NOR records. Must preserve 12 projects and verify full CRC.
 import zlib
 import math
 sys.path.insert(0,str(ROOT/'tools/dpcm4'))
 import beetle_dpcm4 as codec_bd4
 class Upload(ctypes.Structure):
  _fields_=[('destinations',ctypes.c_uint32*128),('total',ctypes.c_uint32),
   ('expected_crc',ctypes.c_uint32),('accumulator',ctypes.c_uint32),
   ('received',ctypes.c_uint32),('generation',ctypes.c_uint32),
   ('chunks',ctypes.c_uint16),('filled',ctypes.c_uint16),
   ('current',ctypes.c_uint16),('slot',ctypes.c_uint16),
   ('index_pos',ctypes.c_uint16),('active',U8)]
 lib.beetle_v2_sample_upload_begin.argtypes=[ctypes.POINTER(Writer),ctypes.POINTER(Upload),U8,ctypes.c_uint32,ctypes.c_uint32,ctypes.POINTER(U8),ctypes.POINTER(U8),ctypes.POINTER(U8)]
 lib.beetle_v2_sample_upload_feed.argtypes=[ctypes.POINTER(Writer),ctypes.POINTER(Upload),ctypes.POINTER(U8),ctypes.c_uint16,ctypes.POINTER(U8),ctypes.POINTER(U8),ctypes.POINTER(U8)]
 lib.beetle_v2_sample_upload_commit.argtypes=[ctypes.POINTER(Writer),ctypes.POINTER(Upload),ctypes.POINTER(U8),ctypes.POINTER(U8),ctypes.POINTER(U8),ctypes.POINTER(U8)]
 lib.beetle_v2_sample_upload_abort.argtypes=[ctypes.POINTER(Upload)]
 def streamed(w,ix,old,pg,slot,data,commit=True,crc_override=None):
  up=Upload();chunk=(U8*4068)()
  check=zlib.crc32(data) if crc_override is None else crc_override
  rc=lib.beetle_v2_sample_upload_begin(ctypes.byref(w),ctypes.byref(up),slot,len(data),check,ix,old,pg)
  if rc:return rc
  for start in range(0,len(data),24):
   piece=data[start:start+24];arr=(U8*len(piece)).from_buffer_copy(piece)
   rc=lib.beetle_v2_sample_upload_feed(ctypes.byref(w),ctypes.byref(up),arr,len(piece),ix,pg,chunk)
   if rc:return rc
  if commit:return lib.beetle_v2_sample_upload_commit(ctypes.byref(w),ctypes.byref(up),ix,old,pg,chunk)
  lib.beetle_v2_sample_upload_abort(ctypes.byref(up));return 0
 flash,w3,ix,old,pgbuf,cb3=make_fixture(original)
 long_sample=codec_bd4.encode([int(9000*math.sin(i/22)) for i in range(155000)])
 assert streamed(w3,ix,old,pgbuf,6,long_sample)==0
 s=Storage(Nor(bytes(flash.bytes),authorized=True),codec)
 assert s.load_sample(6)==long_sample
 assert all(s.load_project(i)==(mountain if i==0 else nuggie) for i in range(12))
 prior=bytes(flash.bytes)
 corrupt=codec_bd4.encode([int(8000*math.sin(i/31)) for i in range(9000)])
 assert streamed(w3,ix,old,pgbuf,6,corrupt,crc_override=0x12345678)==-4
 assert Storage(Nor(bytes(flash.bytes),authorized=True),codec).load_sample(6)==long_sample
 print('PASS: bounded streaming transfer with 24-byte packets, full CRC and no incomplete publication')
 # CRC invalid inputs and stale metadata cannot silently authorize writes.
 corrupt=bytearray(original);corrupt[:8192]=b'\x00'*8192
 nr=Nor(bytes(corrupt),authorized=True);w=Writer();idx=(U8*4096)();older=(U8*4096)()
 def bogus_read(ctx,addr,dst,n):
  rel=addr-0x97000
  if rel<0 or rel+n>len(nr.bytes):return -1
  ctypes.memmove(dst,bytes(nr.bytes[rel:rel+n]),n);return 0
 rd=RD(bogus_read)
 def no_writes(ctx,addr,src,n):raise AssertionError('unauthorized flash write')
 pg=PG(no_writes)
 def no_erase(ctx,addr):raise AssertionError('unauthorized erase')
 er=ER(no_erase)
 assert lib.beetle_v2_writer_open(ctypes.byref(w),rd,pg,er,None,idx,older)<0
 print('PASS: V2 writer refuses unknown/corrupt superblocks without flash modifications')
