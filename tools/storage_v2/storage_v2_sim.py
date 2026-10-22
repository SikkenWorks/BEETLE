#!/usr/bin/env python3
"""BEETLE independent project + global sample namespaces on a shared NOR volume. NEVER speaks to FM1 hardware.

Models 288 KiB, 4 KiB sector erase, 256-byte 1->0 page programming,
alternating CRC-protected metadata, immutable record generations, 12 project
slots, 13 shared sample slots, plus strict non-overlapping PROJECT/SAMPLE
sector ownership with dynamic allocation and four-sector recovery reserve.
No firmware flash map / migration or USB transport is enabled.
"""
from __future__ import annotations
from dataclasses import dataclass
import ctypes
import math
import struct
import zlib
from pathlib import Path
from typing import Optional

SECTOR=4096
PAGE=256
SECTORS=72
FLASH_BYTES=SECTOR*SECTORS
META_SECTORS=2
SCRATCH_SECTORS=4
PROJECT_SLOTS=12
SAMPLE_SLOTS=13
SAMPLE_CHUNK=SECTOR-28  # record header 24 + payload and 4 bytes page padding
RECORD_HEAD=struct.Struct('<4sBBHIIII')  # 24 bytes
META_HEAD=struct.Struct('<4sHHIII')  # 20 bytes
REF_FMT=struct.Struct('<III')
MAX_SAMPLE_CHUNKS=128

class PowerFailure(Exception): pass
class UnsafeLayout(Exception): pass
class NotEnoughSafeSpace(Exception): pass
class CorruptData(Exception): pass

@dataclass(frozen=True)
class Ref:
    offset: int
    length: int
    checksum: int

@dataclass(frozen=True)
class ProjectRef:
    generation: int
    ref: Ref

@dataclass(frozen=True)
class SampleRef:
    generation: int
    size: int
    checksum: int
    chunks: tuple[Ref,...]

@dataclass(frozen=True)
class Snapshot:
    generation: int
    projects: tuple[tuple[Optional[ProjectRef],Optional[ProjectRef]],...]
    samples: tuple[Optional[SampleRef],...]
    sector: int = -1

@dataclass(frozen=True)
class SampleLease:
    """Immutable voice snapshot. Caller MUST release before power off / device migration.

    A lease identifies the sample record generation, rather than the mutable slot.
    The storage owner pins all referenced sectors until explicit release.
    """
    token: int
    slot: int
    entry: SampleRef



def crc(b: bytes)->int:
    return zlib.crc32(b)&0xffffffff

def blank_snapshot()->Snapshot:
    return Snapshot(0,tuple((None,None) for _ in range(PROJECT_SLOTS)),
                    tuple(None for _ in range(SAMPLE_SLOTS)))

class Nor:
    def __init__(self, image: Optional[bytes]=None, *, authorized=False):
        self.bytes=bytearray(image if image is not None else b'\xff'*FLASH_BYTES)
        if len(self.bytes)!=FLASH_BYTES: raise ValueError('wrong NOR capacity')
        self.authorized=authorized  # simulator-only model of independently established BEETLE ownership
        self.mutations=0
        self.fault_after=None

    def _mutation(self):
        self.mutations+=1
        if self.fault_after is not None and self.mutations>=self.fault_after:
            raise PowerFailure(f'power cut after NOR mutation {self.mutations}')

    def read(self, offset:int,n:int)->bytes:
        if offset<0 or n<0 or offset+n>FLASH_BYTES:raise ValueError('read out of partition')
        return bytes(self.bytes[offset:offset+n])

    def program_page(self, offset:int, data:bytes):
        if not self.authorized: raise UnsafeLayout('flash ownership not independently verified')
        if offset%PAGE or not data or len(data)>PAGE or offset+len(data)>FLASH_BYTES:
            raise ValueError('unaligned page')
        for i,b in enumerate(data):
            if (self.bytes[offset+i]&b)!=b:
                raise UnsafeLayout('NOR cannot change a zero bit back to one')
        for i,b in enumerate(data):self.bytes[offset+i]&=b
        self._mutation()

    def erase_sector(self, sector:int):
        if not self.authorized:raise UnsafeLayout('flash ownership not independently verified')
        if sector<0 or sector>=SECTORS:raise ValueError('invalid erase')
        self.bytes[sector*SECTOR:(sector+1)*SECTOR]=b'\xff'*SECTOR
        self._mutation()


def _refbytes(ref:Optional[Ref])->bytes:
    return REF_FMT.pack(*( (ref.offset,ref.length,ref.checksum) if ref else (0,0,0)))

def _parse_ref(b:bytes,p:int)->tuple[Optional[Ref],int]:
    a,n,c=REF_FMT.unpack_from(b,p)
    return (Ref(a,n,c) if n else None),p+REF_FMT.size

def _snapshot_bytes(s:Snapshot)->bytes:
    payload=bytearray()
    for pair in s.projects:
        for ent in pair:
            payload+=struct.pack('<I',ent.generation if ent else 0)
            payload+=_refbytes(ent.ref if ent else None)
    for ent in s.samples:
        if ent:
            payload+=struct.pack('<IIIH',ent.generation,ent.size,ent.checksum,len(ent.chunks))
            for ref in ent.chunks: payload+=_refbytes(ref)
        else:payload+=struct.pack('<IIIH',0,0,0,0)
    if len(payload)>SECTOR-META_HEAD.size:raise NotEnoughSafeSpace('superblock index overflow')
    base=struct.pack('<4sHHII',b'B2SB',1,0,s.generation,len(payload))
    header=base+struct.pack('<I',crc(base+payload))
    return (header+payload).ljust(SECTOR-4,b'\xff')+b'B2OK'

def _parse_snapshot(buf:bytes,sector:int)->Optional[Snapshot]:
    if len(buf)!=SECTOR or buf[:4]!=b'B2SB' or buf[-4:]!=b'B2OK':return None
    try:
        magic,version,reserved,gen,n,checksum=META_HEAD.unpack_from(buf)
        if version!=1 or reserved!=0 or gen==0 or n>SECTOR-META_HEAD.size-4 or crc(buf[:META_HEAD.size-4]+buf[META_HEAD.size:META_HEAD.size+n])!=checksum:
            return None
        b=buf[META_HEAD.size:META_HEAD.size+n];p=0; projects=[];samples=[]
        for _ in range(PROJECT_SLOTS):
            pair=[]
            for __ in range(2):
                g,=struct.unpack_from('<I',b,p);p+=4
                ref,p=_parse_ref(b,p)
                if (g==0)!=(ref is None):return None
                pair.append(ProjectRef(g,ref) if ref else None)
            projects.append(tuple(pair))
        chunks_total=0
        for _ in range(SAMPLE_SLOTS):
            g,sz,ch,nchunks=struct.unpack_from('<IIIH',b,p);p+=14
            if not g:
                if sz or ch or nchunks:return None
                samples.append(None);continue
            if not sz or not nchunks or chunks_total+nchunks>MAX_SAMPLE_CHUNKS:return None
            chunks=[]
            for __ in range(nchunks):
                ref,p=_parse_ref(b,p)
                if ref is None:return None
                chunks.append(ref)
            chunks_total+=nchunks
            samples.append(SampleRef(g,sz,ch,tuple(chunks)))
        if p!=len(b):return None
        return Snapshot(gen,tuple(projects),tuple(samples),sector)
    except (ValueError,struct.error):return None


def _pack_record(kind:int,slot:int,part:int,gen:int,payload:bytes)->bytes:
    if kind not in (1,2) or not payload or len(payload)>(12256+16 if kind==1 else SAMPLE_CHUNK):
        raise ValueError('invalid record')
    head=RECORD_HEAD.pack(b'B2RC',kind,slot,part,gen,len(payload),crc(payload),0)
    head=head[:-4]+struct.pack('<I',crc(head[:-4]))
    return (head+payload).ljust(math.ceil((len(head)+len(payload))/PAGE)*PAGE,b'\xff')

class ProjectCodec:
    def __init__(self,lib):
        self.lib=lib
        self.encode=lib.beetle_pack_project_v2
        self.decode=lib.beetle_unpack_project_v2
        for fn in (self.encode,self.decode):fn.restype=ctypes.c_int

    def pack(self,src:bytes)->bytes:
        out=ctypes.create_string_buffer(len(src)+16);written=ctypes.c_size_t()
        table=(ctypes.c_uint16*2048)()
        err=self.encode(src,len(src),out,len(out),ctypes.byref(written),table)
        if err:raise ValueError(f'packer failed {err}')
        return out.raw[:written.value]

    def unpack(self,b:bytes)->bytes:
        if len(b)<16:raise CorruptData('packed project header too short')
        size=int.from_bytes(b[6:8],'little')
        if not 0<size<=12256:raise CorruptData('bad length')
        out=ctypes.create_string_buffer(size);written=ctypes.c_size_t()
        err=self.decode(b,len(b),out,len(out),ctypes.byref(written))
        if err or written.value!=size:raise CorruptData('project pack checksum / format')
        return out.raw[:written.value]

class Storage:
    def __init__(self,nor:Nor,codec:ProjectCodec):
        self.nor=nor; self.codec=codec
        self._leases: dict[int, SampleLease]={}
        self._next_lease_token=1
        self.available=self._read_valid_snapshots()
        self.current=max(self.available,key=lambda s:s.generation) if self.available else blank_snapshot()
        # Refuse a non-empty partition with no valid metadata: cannot establish ownership or layout.
        if not self.available and any(b!=255 for b in nor.bytes):
            raise UnsafeLayout('unknown flash content or partial first initialization: explicit safe recovery needed')

    def _read_valid_snapshots(self)->list[Snapshot]:
        results=[]
        for i in range(META_SECTORS):
            candidate=_parse_snapshot(self.nor.read(i*SECTOR,SECTOR),i)
            if candidate and self._valid_snapshot(candidate):results.append(candidate)
        return results

    def _read_record(self,ref:Ref,kind:int,slot:int,part:int,gen:int)->bytes:
        if ref.offset < META_SECTORS*SECTOR or ref.offset % PAGE or ref.length==0 or ref.length>(12256+16 if kind==1 else SAMPLE_CHUNK):
            raise CorruptData('bad record reference')
        count=math.ceil((RECORD_HEAD.size+ref.length)/PAGE)*PAGE
        if ref.offset+count>FLASH_BYTES:raise CorruptData('record outside partition')
        h=self.nor.read(ref.offset,RECORD_HEAD.size)
        magic,k,s,p,g,n,c,header_crc=RECORD_HEAD.unpack(h)
        if (magic!=b'B2RC' or k!=kind or s!=slot or p!=part or g!=gen or n!=ref.length or
            c!=ref.checksum or crc(h[:-4])!=header_crc):raise CorruptData('record header mismatch')
        payload=self.nor.read(ref.offset+RECORD_HEAD.size,n)
        if crc(payload)!=c:raise CorruptData('record payload CRC')
        return payload

    def _project_bytes(self,ref:ProjectRef,slot:int)->bytes:
        return self.codec.unpack(self._read_record(ref.ref,1,slot,0,ref.generation))

    def _sample_bytes(self,entry:SampleRef,slot:int)->bytes:
        b=b''.join(self._read_record(r,2,slot,i,entry.generation) for i,r in enumerate(entry.chunks))
        if len(b)!=entry.size or crc(b)!=entry.checksum:raise CorruptData('sample chain checksum')
        return b

    def _valid_snapshot(self,s:Snapshot)->bool:
        try:
            for slot,pair in enumerate(s.projects):
                if pair[0] is not None:
                    if not any(x is not None and self._project_valid(x,slot) for x in pair):return False
                elif pair[1] is not None:return False
            for slot,sample in enumerate(s.samples):
                if sample:self._sample_bytes(sample,slot)
            return True
        except (CorruptData,struct.error):return False

    def _project_valid(self,x:ProjectRef,slot:int)->bool:
        try:self._project_bytes(x,slot);return True
        except (CorruptData,ValueError):return False

    def load_project(self,slot:int)->Optional[bytes]:
        if not 0<=slot<PROJECT_SLOTS:raise ValueError('slot 0..11')
        for x in self.current.projects[slot]:
            if x and self._project_valid(x,slot):return self._project_bytes(x,slot)
        return None

    def load_sample(self,slot:int)->Optional[bytes]:
        if not 0<=slot<SAMPLE_SLOTS:raise ValueError('slot 0..12')
        x=self.current.samples[slot]
        return self._sample_bytes(x,slot) if x else None

    def acquire_sample(self,slot:int)->SampleLease:
        """Pin an immutable generation before preparing a playback voice.

        Offline scheduler-only operation. A slot replacement/deletion may proceed
        while a pinned voice is playing; its old record sectors are unreclaimable.
        """
        if not 0<=slot<SAMPLE_SLOTS:raise ValueError('slot 0..12')
        entry=self.current.samples[slot]
        if entry is None:raise CorruptData('unassigned sample slot')
        token=self._next_lease_token
        self._next_lease_token+=1
        lease=SampleLease(token,slot,entry)
        self._leases[token]=lease
        return lease

    def release_sample(self,lease:SampleLease)->None:
        """Remove this exact voice lease; stale/double release fails closed."""
        if self._leases.get(lease.token) is not lease:
            raise CorruptData('expired or foreign sample lease')
        del self._leases[lease.token]

    def read_sample_lease(self,lease:SampleLease,offset:int,length:int)->bytes:
        """CRC-check an immutable sample generation after slot replacement."""
        if self._leases.get(lease.token) is not lease:
            raise CorruptData('expired or foreign sample lease')
        return self._read_sample_entry(lease.slot,lease.entry,offset,length)

    def read_sample_range(self,slot:int,offset:int,length:int)->bytes:
        """Read current slot only. Do not use for a long-lived active voice."""
        if not 0<=slot<SAMPLE_SLOTS:raise ValueError('slot 0..12')
        entry=self.current.samples[slot]
        if entry is None:raise CorruptData('unassigned sample slot')
        return self._read_sample_entry(slot,entry,offset,length)

    def _read_sample_entry(self,slot:int,entry:SampleRef,offset:int,length:int)->bytes:
        if offset<0 or length<0 or offset>entry.size or length>entry.size-offset:
            raise CorruptData('sample read outside validated size')
        if not length:return b''
        out=bytearray()
        current=0
        end=offset+length
        for i,ref in enumerate(entry.chunks):
            next_pos=current+ref.length
            if current<end and next_pos>offset:
                part=self._read_record(ref,2,slot,i,entry.generation)
                a=max(offset,current)-current
                z=min(end,next_pos)-current
                out.extend(part[a:z])
            current=next_pos
            if current>=end:break
        if len(out)!=length:raise CorruptData('incomplete sample read')
        return bytes(out)

    @staticmethod
    def _sectors_of_ref(r:Ref)->set[int]:
        a=r.offset//SECTOR
        z=(r.offset+math.ceil((RECORD_HEAD.size+r.length)/PAGE)*PAGE-1)//SECTOR
        return set(range(a,z+1))

    def _live_sectors(self)->set[int]:
        protected=set(range(META_SECTORS))
        for s in self.available:
            for pair in s.projects:
                for x in pair:
                    if x:protected.update(self._sectors_of_ref(x.ref))
            for x in s.samples:
                if x:
                    for r in x.chunks:protected.update(self._sectors_of_ref(r))
        # Never erase a previous generation while a voice still owns it.
        for lease in self._leases.values():
            for ref in lease.entry.chunks:protected.update(self._sectors_of_ref(ref))
        return protected

    def sweep(self)->int:
        """Sector-only reclaim; no relocation and never erases bytes referenced by either valid index."""
        live=self._live_sectors();count=0
        for sector in range(META_SECTORS,SECTORS):
            if sector not in live and any(x!=255 for x in self.nor.read(sector*SECTOR,SECTOR)):
                self.nor.erase_sector(sector);count+=1
        return count

    def _domain_sector_map(self)->dict[int,set[int]]:
        """Identify sectors owned by confirmed index generations and active voices.

        Record kind 1 is a project; kind 2 is a sample. Both share the same
        physical 288 KiB volume but NEVER allocate new records into each
        other's sectors. Ownership is computed dynamically: NO rigid quota
        can silently shrink twelve projects or cap available sample seconds.
        Existing V2 images remain readable even if produced by an older
        page-packing allocator; new writes avoid mixed-domain reuse.
        """
        domains={1:set(),2:set()}
        for snap in self.available:
            for pair in snap.projects:
                for item in pair:
                    if item: domains[1].update(self._sectors_of_ref(item.ref))
            for sample in snap.samples:
                if sample:
                    for ref in sample.chunks:
                        domains[2].update(self._sectors_of_ref(ref))
        for lease in self._leases.values():
            for ref in lease.entry.chunks:
                domains[2].update(self._sectors_of_ref(ref))
        return domains

    def domain_stats(self)->dict[str,int]:
        """Sector usage is unique per domain; legacy mixed sectors reported."""
        d=self._domain_sector_map()
        return {'project_sectors':len(d[1]),'sample_sectors':len(d[2]),
                'shared_legacy_sectors':len(d[1]&d[2]),
                'project_bytes_on_flash':len(d[1])*SECTOR,
                'sample_bytes_on_flash':len(d[2])*SECTOR}

    def _plan(self,payloads:list[bytes])->list[tuple[int,bytes]]:
        """Preflight every record before ANY mutation. Never cross namespaces.

        Sample records may share pages with OTHER samples within one sector.
        Packed projects may share free 256-byte pages with OTHER projects,
        which keeps twelve small slots economical. They never share a NOR
        erase sector with a sample. This is sector-safe reclamation, not two
        fixed quotas; growth of either area uses available unowned sectors.
        """
        pending=set();plan=[]
        domains=self._domain_sector_map()
        # These sets include records planned in this transaction, not merely
        # committed records, so multi-chunk uploads cannot mix ownership.
        sector_owner={}
        for kind,sectors in domains.items():
            for sector in sectors:
                if sector in sector_owner and sector_owner[sector]!=kind:
                    # Legacy V2 data may already contain a shared sector.
                    # Protect it, and refuse NEW allocation into that sector.
                    sector_owner[sector]=0
                else:sector_owner[sector]=kind
        for record in payloads:
            kind=record[4] if len(record)>=RECORD_HEAD.size else 0
            if kind not in (1,2) or len(record)%PAGE:raise ValueError('invalid planned record')
            if kind==2 and len(record)>SECTOR:raise ValueError('sample chunk spans erase sector')
            pages=len(record)//PAGE
            found=None
            # Projects grow from low addresses; sample chunks from the high
            # end. This leaves a flexible gap for either namespace to expand.
            offsets=(range(META_SECTORS*SECTOR,FLASH_BYTES-len(record)+1,PAGE)
                     if kind==1 else
                     range(FLASH_BYTES-len(record),META_SECTORS*SECTOR-1,-PAGE))
            for offset in offsets:
                first=offset//SECTOR
                last=(offset+len(record)-1)//SECTOR
                if kind==2 and first!=last:continue
                if any(sector_owner.get(sec,kind)!=kind for sec in range(first,last+1)):
                    continue
                # A sector without a live owner must be fully erased before
                # it can join either namespace. Otherwise stale orphan pages
                # from a power-cut transaction could leave project bytes in
                # a newly allocated sample sector (or the reverse).
                if any(sec not in sector_owner and
                       self.nor.read(sec*SECTOR,SECTOR)!=b'\xff'*SECTOR
                       for sec in range(first,last+1)):
                    continue
                page_ids=range(offset//PAGE,offset//PAGE+pages)
                if any(page in pending for page in page_ids):continue
                if all(self.nor.read(i*PAGE,PAGE)==b'\xff'*PAGE for i in page_ids):
                    found=offset
                    pending.update(page_ids)
                    for sec in range(first,last+1):sector_owner[sec]=kind
                    break
            if found is None:raise NotEnoughSafeSpace('no safe contiguous pages in requested storage namespace')
            plan.append((found,record))
        # Reserve *whole erased sectors*, not a mere count of unfilled pages.
        remain=sum(all(((sector*SECTOR+offset)//PAGE not in pending) and
                       self.nor.read(sector*SECTOR+offset,PAGE)==b'\xff'*PAGE
                       for offset in range(0,SECTOR,PAGE))
                   for sector in range(META_SECTORS,SECTORS))
        if remain<SCRATCH_SECTORS:
            raise NotEnoughSafeSpace('transaction would consume four whole erased safety sectors')
        return plan

    def _write_record_plan(self,plan:list[tuple[int,bytes]], payloads:list[bytes])->list[Ref]:
        refs=[]
        for (offset,record),payload in zip(plan,payloads):
            for pos in range(0,len(record),PAGE):
                self.nor.program_page(offset+pos,record[pos:pos+PAGE])
            if self.nor.read(offset,len(record))!=record:raise CorruptData('NOR readback mismatch')
            refs.append(Ref(offset,len(payload),crc(payload)))
        return refs

    def _commit(self,projects,samples):
        # Write into the older/empty metadata sector: the other copy stays fully committed.
        if len(self.available)<2:
            existing={s.sector for s in self.available}
            target=next(i for i in range(2) if i not in existing)
        else:target=min(self.available,key=lambda s:s.generation).sector
        snap=Snapshot(self.current.generation+1,tuple(projects),tuple(samples),target)
        raw=_snapshot_bytes(snap)
        current_sector=self.nor.read(target*SECTOR,SECTOR)
        if current_sector!=b'\xff'*SECTOR:self.nor.erase_sector(target)
        for pos in range(0,SECTOR-PAGE,PAGE):
            page=raw[pos:pos+PAGE]
            if page!=b'\xff'*PAGE:self.nor.program_page(target*SECTOR+pos,page)
        # Commit marker is deliberately programmed LAST, in a separate NOR page.
        self.nor.program_page(target*SECTOR+SECTOR-PAGE,raw[-PAGE:])
        if self.nor.read(target*SECTOR,SECTOR)!=raw:raise CorruptData('index readback failed')
        refreshed=self._read_valid_snapshots()
        if not any(x.sector==target and x.generation==snap.generation for x in refreshed):
            raise CorruptData('committed index validation failed')
        self.available=refreshed;self.current=max(refreshed,key=lambda x:x.generation)

    def save_project(self,slot:int,data:bytes):
        if not 0<=slot<PROJECT_SLOTS:raise ValueError('project slot 0..11')
        if not self.nor.authorized:raise UnsafeLayout('no write permission')
        packed=self.codec.pack(data)
        p=list(self.current.projects);new_gen=(p[slot][0].generation+1 if p[slot][0] else 1)
        record=_pack_record(1,slot,0,new_gen,packed)
        # Never mutate flash if safe-space preflight will refuse this transaction.
        # Metadata capacity checked before a page write; a rejected save
        # must never leave a half-filled flash volume.
        _snapshot_bytes(Snapshot(self.current.generation+1,
            tuple(p[:slot]+[(ProjectRef(new_gen,Ref(8192,len(packed),crc(packed))),p[slot][0])]+p[slot+1:]),
            self.current.samples))
        plan=self._plan([record]);ref=self._write_record_plan(plan,[packed])[0]
        new=ProjectRef(new_gen,ref)
        p[slot]=(new,p[slot][0])
        self._commit(p,self.current.samples)

    def replace_sample(self,slot:int,encoded:bytes):
        if not 0<=slot<SAMPLE_SLOTS:raise ValueError('sample slot 0..12')
        if not encoded:raise ValueError('encoded sample may not be empty')
        if not self.nor.authorized:raise UnsafeLayout('no write permission')
        old=self.current.samples[slot];gen=old.generation+1 if old else 1
        chunks=[encoded[i:i+SAMPLE_CHUNK] for i in range(0,len(encoded),SAMPLE_CHUNK)]
        if len(chunks)>MAX_SAMPLE_CHUNKS:raise NotEnoughSafeSpace('index capacity exceeded')
        records=[_pack_record(2,slot,i,gen,c) for i,c in enumerate(chunks)]
        samples=list(self.current.samples)
        trial_refs=tuple(Ref(8192,len(chunk),crc(chunk)) for chunk in chunks)
        samples[slot]=SampleRef(gen,len(encoded),crc(encoded),trial_refs)
        _snapshot_bytes(Snapshot(self.current.generation+1,self.current.projects,tuple(samples)))
        # Reclaim is explicit/checkpointed: rejected transactions must not touch NOR.
        plan=self._plan(records);refs=self._write_record_plan(plan,chunks)
        samples[slot]=SampleRef(gen,len(encoded),crc(encoded),tuple(refs))
        self._commit(self.current.projects,samples)

    def delete_sample(self,slot:int):
        if not 0<=slot<SAMPLE_SLOTS:raise ValueError('sample slot 0..12')
        if not self.nor.authorized:raise UnsafeLayout('no write permission')
        samples=list(self.current.samples);samples[slot]=None
        self._commit(self.current.projects,samples)

    def delete_project(self,slot:int):
        """Delete project without modifying ANY global sample slot.

        Like sample deletion this is a transactionally committed metadata
        change; the old generation is retained until an explicit checkpoint.
        """
        if not 0<=slot<PROJECT_SLOTS:raise ValueError('project slot 0..11')
        if not self.nor.authorized:raise UnsafeLayout('no write permission')
        projects=list(self.current.projects)
        projects[slot]=(None,None)
        self._commit(projects,self.current.samples)

    def checkpoint(self):
        """Commit identical index again to retire older obsolete references, then sweep."""
        self._commit(self.current.projects,self.current.samples)
        return self.sweep()

    def stats(self):
        live=self._live_sectors()
        erased=sum(self.nor.read(i*SECTOR,SECTOR)==b'\xff'*SECTOR for i in range(2,SECTORS))
        dirty=sum(self.nor.read(i*SECTOR,SECTOR)!=b'\xff'*SECTOR and i not in live for i in range(2,SECTORS))
        return dict(total_bytes=FLASH_BYTES,live_sectors=len(live)-2,
                    **self.domain_stats(),
                    erased_sectors=erased,unreferenced_dirty_sectors=dirty,
                    pinned_generations=len(self._leases),
                    guard_erased_sectors=SCRATCH_SECTORS)
