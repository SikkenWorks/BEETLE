#!/usr/bin/env python3
"""Regression: native V2 DELETE removes one slot, preserves others and global samples."""
import ctypes,subprocess,sys,tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools/storage_v2'))
from storage_v2_sim import Nor,Storage,ProjectCodec
U8=ctypes.c_uint8
RD=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,ctypes.POINTER(U8),ctypes.c_uint32)
PG=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,ctypes.POINTER(U8),ctypes.c_uint32)
ER=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32)
class Device(ctypes.Structure):
 _fields_=[('read',ctypes.c_void_p),('context',ctypes.c_void_p),('index_address',ctypes.c_uint32),('generation',ctypes.c_uint32)]
class Writer(ctypes.Structure):
 _fields_=[('device',Device),('program',ctypes.c_void_p),('erase',ctypes.c_void_p),('context',ctypes.c_void_p),('domain',U8*72),('occupied',ctypes.c_uint16*72),('active_meta',U8)]
with tempfile.TemporaryDirectory() as td:
 p=root/'firmware/fm1-native';so=Path(td)/'v2.so'
 subprocess.run(['cc','-std=c11','-O2','-fPIC','-shared',str(p/'beetle_storage_v2_writer.c'),str(p/'beetle_storage_v2_reader.c'),str(p/'beetle_project_pack_v2.c'),'-o',str(so)],check=True)
 lib=ctypes.CDLL(str(so));codec=ProjectCodec(lib)
 nor=Nor(authorized=True);store=Storage(nor,codec)
 project0=(root/'tests/fixtures/MOUNTAINKING-SLOT-01.trk').read_bytes()
 project1=(root/'tests/fixtures/NUGGIE-SLOT-07.trk').read_bytes()
 store.save_project(0,project0);store.save_project(1,project1)
 sample=b'VALID GLOBAL SAMPLE DATA'*200;store.replace_sample(0,sample)
 def rd(ctx,address,dst,length):
  off=address-0x97000
  if off<0 or off+length>len(nor.bytes):return -1
  ctypes.memmove(dst,bytes(nor.bytes[off:off+length]),length);return 0
 def pg(ctx,address,src,length):
  try:nor.program_page(address-0x97000,bytes(src[:length]));return 0
  except Exception:return -1
 def er(ctx,address):
  try:nor.erase_sector((address-0x97000)//4096);return 0
  except Exception:return -1
 callbacks=(RD(rd),PG(pg),ER(er));w=Writer();idx=(U8*4096)();scratch=(U8*20480)()
 lib.beetle_v2_writer_open.argtypes=[ctypes.POINTER(Writer),RD,PG,ER,ctypes.c_void_p,ctypes.POINTER(U8),ctypes.POINTER(U8)]
 lib.beetle_v2_remove_project.argtypes=[ctypes.POINTER(Writer),U8,ctypes.POINTER(U8),ctypes.POINTER(U8),ctypes.POINTER(U8)]
 assert lib.beetle_v2_writer_open(ctypes.byref(w),*callbacks,None,idx,scratch)==0
 assert lib.beetle_v2_remove_project(ctypes.byref(w),0,idx,scratch,ctypes.cast(ctypes.byref(scratch,4096),ctypes.POINTER(U8)))==0
 reread=Storage(Nor(bytes(nor.bytes),authorized=True),codec)
 assert reread.load_project(0) is None
 assert reread.load_project(1)==project1
 assert reread.load_sample(0)==sample
 print('PASS V1.6 native V2 delete: selected project gone, other project and sample unchanged')
