#!/usr/bin/env python3
"""Storage V2 project-reserve capacity invariant. In-memory NOR only."""
import ctypes,tempfile,subprocess,sys
from pathlib import Path
R=Path(__file__).resolve().parents[2];N=R/'firmware/fm1-native'
sys.path.insert(0,str(R/'tools/storage_v2'))
from storage_v2_sim import Nor,Storage,ProjectCodec
U8=ctypes.c_uint8
RD=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,ctypes.POINTER(U8),ctypes.c_uint32)
PG=RD;ER=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32)
class Device(ctypes.Structure):
 _fields_=[('read',ctypes.c_void_p),('context',ctypes.c_void_p),('index_address',ctypes.c_uint32),('generation',ctypes.c_uint32)]
class Writer(ctypes.Structure):
 _fields_=[('device',Device),('program',ctypes.c_void_p),('erase',ctypes.c_void_p),('context',ctypes.c_void_p),('domain',U8*72),('occupied',ctypes.c_uint16*72),('active_meta',U8)]
class Capacity(ctypes.Structure):
 _fields_=[('used',ctypes.c_uint32),('additional',ctypes.c_uint32),('erased',ctypes.c_uint16),('pages',ctypes.c_uint16)]
with tempfile.TemporaryDirectory() as tmp:
 so=Path(tmp)/'writer.so';subprocess.run(['cc','-std=c11','-O2','-fPIC','-shared','-Wall','-Wextra','-Werror',*[str(N/x) for x in ('beetle_storage_v2_writer.c','beetle_storage_v2_reader.c','beetle_project_pack_v2.c')],'-o',str(so)],check=True)
 lib=ctypes.CDLL(str(so));codec=ProjectCodec(lib)
 lib.beetle_v2_writer_open.argtypes=[ctypes.POINTER(Writer),RD,PG,ER,ctypes.c_void_p,ctypes.POINTER(U8),ctypes.POINTER(U8)];lib.beetle_v2_write_sample.argtypes=[ctypes.POINTER(Writer),U8,ctypes.POINTER(U8),ctypes.c_uint32,ctypes.POINTER(U8),ctypes.POINTER(U8),ctypes.POINTER(U8)];lib.beetle_v2_write_project.argtypes=lib.beetle_v2_write_sample.argtypes;lib.beetle_v2_get_sample_capacity.argtypes=[ctypes.POINTER(Writer),ctypes.POINTER(U8),ctypes.POINTER(U8),ctypes.POINTER(Capacity)]
 initial=Nor(authorized=True);store=Storage(initial,codec)
 small=(R/'tests/fixtures/NUGGIE-SLOT-07.trk').read_bytes()
 big=(R/'tests/fixtures/MOUNTAINKING-SLOT-01.trk').read_bytes()
 for k in range(12):store.save_project(k,big if k==0 else small)
 data=Nor(bytes(initial.bytes),authorized=True);idx=(U8*4096)();old=(U8*4096)();page=(U8*256)();w=Writer()
 def rd(ctx,addr,ptr,n):ctypes.memmove(ptr,bytes(data.bytes[addr-0x97000:addr-0x97000+n]),n);return 0
 def pg(ctx,addr,ptr,n):
  try:data.program_page(addr-0x97000,bytes(ptr[:n]));return 0
  except Exception:return -1
 def er(ctx,addr):
  try:data.erase_sector((addr-0x97000)//4096);return 0
  except Exception:return -1
 cb=[RD(rd),PG(pg),ER(er)]
 assert lib.beetle_v2_writer_open(ctypes.byref(w),*cb,None,idx,old)==0
 cap=Capacity();assert lib.beetle_v2_get_sample_capacity(ctypes.byref(w),idx,page,ctypes.byref(cap))==0
 assert cap.used==0 and cap.additional>0,(cap.used,cap.additional)
 recorded=[];denied=None
 for k in range(13):
  payload=bytes((j+k*17)%256 for j in range(23000));buf=(U8*len(payload)).from_buffer_copy(payload)
  before=bytes(data.bytes)
  rc=lib.beetle_v2_write_sample(ctypes.byref(w),k,buf,len(payload),idx,old,page)
  if rc==-2:
   denied=k;assert bytes(data.bytes)==before,'Preflight wrote flash despite storage full'
   break
  assert rc==0,(k,rc)
  recorded.append(payload)
 assert denied is not None,'No sample was blocked by the project reserve test'
 # An existing project can still be re-saved after sample capacity refusal.
 project=codec.pack(small);projbuf=(U8*len(project)).from_buffer_copy(project)
 rc=lib.beetle_v2_write_project(ctypes.byref(w),0,projbuf,len(project),idx,old,page)
 assert rc==0,rc
 check=Storage(Nor(bytes(data.bytes),authorized=True),codec)
 assert check.load_project(0)==small
 for i,p in enumerate(recorded):assert check.load_sample(i)==p
 print('PASS: sample uploads stop before consuming reserved sectors; project rewrite succeeds afterwards; '+str(len(recorded))+' sample slots filled')
