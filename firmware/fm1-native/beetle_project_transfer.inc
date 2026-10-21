/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE project-only USB-MIDI transport V1, in the normal application.
 * The OTA loader is NEVER used for project transfer. No arbitrary flash
 * addresses, no sector provisioning, and no flash writes prior to COMMIT.
 * BEGIN, DATA, COMMIT are guarded by the existing journal's ownership,
 * full-project decoder, two-bank CRC, and A/B commit path.
 * Wire: stock JieLi packed-7 envelope [00 59 cmd len24 body checksum].
 * First body byte is the matching host transaction sequence (0..127).
 * 0x50 HELLO, 0x51 LIST, 0x52 READ_BEGIN, 0x53 READ,
 * 0x54 FINISH, 0x55 WRITE_BEGIN, 0x56 WRITE, 0x57 COMMIT, 0x58 ABORT,
 * 0x59 DELETE_SLOT (V2); 0x5A RECOVER_CORRUPT_SLOT (V3);
 * 0x5B READ-ONLY MARKER AUDIT, 0x5C EXPLICIT MARKER REPAIR (V4).
 */
#define BEETLE_XFER_CHUNK 24u
#define BEETLE_XFER_TTL_MS 120000u
#define BEETLE_XFER_CAP TRACK_STORE_V4_MAX
#define BERR_STORAGE 1u
#define BERR_BUSY 2u
#define BERR_SLOT 3u
#define BERR_EMPTY 4u
#define BERR_LENGTH 5u
#define BERR_CRC 6u
#define BERR_FORMAT 7u
#define BERR_OCCUPIED 8u
#define BERR_IO 9u
#define BERR_STATE 10u
#define BERR_OWNER 11u
#define BERR_CAPACITY 12u

struct beetle_transfer_state {
 uint32_t last_ms, expected_crc;
 uint16_t length, position;
 uint8_t kind, slot; /* 1 = read, 2 = staged upload */
};
static struct beetle_transfer_state beetle_xfer;
static uint32_t beetle_xfer_rd32(const uint8_t *p){
 return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void beetle_xfer_wr32(uint8_t *p,uint32_t x){
 for(uint8_t j=0;j<4u;j++)p[j]=(uint8_t)(x>>(8u*j));
}
static void beetle_xfer_reset(void){beetle_xfer.kind=0u;beetle_xfer.position=0u;beetle_xfer.length=0u;}
static int beetle_transfer_active(void){
 if(beetle_xfer.kind && (uint32_t)(etch_ms-beetle_xfer.last_ms)>BEETLE_XFER_TTL_MS)
  beetle_xfer_reset();
 return beetle_xfer.kind!=0u;
}
static void beetle_xfer_reply(uint8_t cmd,const uint8_t *p,uint32_t n){
 if(n<=32u)ota_send_msg(cmd,p,n);
}
static void beetle_xfer_error(uint8_t seq,uint8_t original,uint8_t code){
 uint8_t v[3]={seq,original,code};beetle_xfer_reply(0x6Fu,v,sizeof(v));
}
static uint8_t beetle_xfer_ready(void){
#if BEETLE_NATIVE_STORAGE
 return (uint8_t)(!native_safe&&!native_tripped&&!native_updating &&
                        !track_state.transport.playing);
#else
 return 0u;
#endif
}
/* Return 1 when the message was handled (or refused), 0 for OTA frames. */
static int beetle_transfer_handle(uint32_t d){
 const uint8_t cmd=ota_dec[2];
 if(cmd<0x50u || cmd>0x5Cu)return 0;
 const uint8_t *arg=ota_dec+6u;
 const uint32_t n=d-7u;
 if(n<1u)return 1;
 const uint8_t seq=arg[0];
 uint8_t out[32]={seq};
 if(cmd==0x50u){
  if(n!=1u){beetle_xfer_error(seq,cmd,BERR_LENGTH);return 1;}
  out[1]=4u;out[2]=TRACK_JOURNAL_SLOTS;out[3]=TRACK_STORE_VERSION;
#if BEETLE_NATIVE_STORAGE
  out[4]=1u; /* hardware features compiled; actual ownership tested separately */
#else
  out[4]=0u;
#endif
  beetle_xfer_reply(0x60u,out,5u);return 1;
 }
 /* V4: READ-ONLY audit works even when global marker is missing/corrupt.
  * Status 0=owned, 1=can repair after explicit consent, 2=foreign/unknown,
  * 3=no provably BEETLE-owned banks, 4=physical read error. */
 if(cmd==0x5Bu){
  if(n!=1u){beetle_xfer_error(seq,cmd,BERR_LENGTH);return 1;}
#if BEETLE_NATIVE_STORAGE
  if(beetle_transfer_active()||!beetle_xfer_ready()){
   beetle_xfer_error(seq,cmd,BERR_BUSY);return 1;
  }
  int audit=beetle_hal_marker_audit();
  out[1]=(uint8_t)(audit==0?0:audit==1?1:audit==-3?3:audit==-2?4:2);
#else
  out[1]=4u;
#endif
  beetle_xfer_reply(0x6Bu,out,2u);return 1;
 }
 if(cmd==0x58u || cmd==0x54u){
  beetle_xfer_reset();beetle_xfer_reply(0x64u,out,1u);return 1;
 }
#if !BEETLE_NATIVE_STORAGE
 beetle_xfer_error(seq,cmd,BERR_STORAGE);return 1;
#else
 if(beetle_transfer_active() && cmd!=0x53u && cmd!=0x56u && cmd!=0x57u){
  beetle_xfer_error(seq,cmd,BERR_BUSY);return 1;
 }
 if(!beetle_xfer_ready()){
  beetle_xfer_error(seq,cmd,BERR_BUSY);return 1;
 }
 /* Probe once per command that begins or commits a transaction. Chunk
  * reads must not re-scan the entire ownership marker 500+ times. */
 if(cmd==0x5Cu){
  if(n!=7u||arg[1]!=0xB4u||arg[2]!=0x19u||arg[3]!=0xA7u||
     arg[4]!=0x5Cu||arg[5]!=0xE2u||arg[6]!=0x08u){
   beetle_xfer_error(seq,cmd,BERR_FORMAT);return 1;
  }
  if(beetle_hal_marker_audit()!=1){beetle_xfer_error(seq,cmd,BERR_OWNER);return 1;}
  native_pause();beetle_write_permit=1u;
  int restored=beetle_hal_restore_marker();
  beetle_write_permit=0u;native_start();
  if(restored){beetle_xfer_error(seq,cmd,restored==-2?BERR_IO:BERR_OWNER);return 1;}
  out[1]=1u;beetle_xfer_reply(0x6Cu,out,2u);return 1;
 }
 if((cmd==0x51u||cmd==0x52u||cmd==0x55u||cmd==0x57u||cmd==0x59u||cmd==0x5Au) &&
    beetle_hal_probe_ownership()!=BEETLE_CLAIM_EXISTING){
  beetle_xfer_reset();beetle_xfer_error(seq,cmd,BERR_OWNER);return 1;
 }
 if(cmd==0x5Au){
  /* V3 RECOVER: explicit six-byte confirmation, no live project, verified
   * BEETLE ownership marker and both physical banks individually audited. */
  if(n!=6u||arg[1]>=TRACK_PROJECTS||arg[2]!=0xD3u||arg[3]!=0x7Eu||
     arg[4]!=0xC9u||arg[5]!=0x34u){
   beetle_xfer_error(seq,cmd,BERR_FORMAT);return 1;
  }
  const uint8_t slot=arg[1];
  if(track_session_saved_slot==slot||
     (track_state.dirty&&track_state.project_slot==slot)){
   beetle_xfer_error(seq,cmd,BERR_BUSY);return 1;
  }
  native_pause();
  beetle_write_permit=1u;
  int r=track_journal_delete_corrupt(&beetle_hal_journal_io,slot);
  beetle_write_permit=0u;
  if(r==TRACK_JOURNAL_OK){track_slot_seen[slot]=0u;track_load_status[slot]=2u;}
  native_start();
  if(r!=TRACK_JOURNAL_OK){
   beetle_xfer_error(seq,cmd,r==TRACK_JOURNAL_NO_PROJECT?BERR_EMPTY:
    r==TRACK_JOURNAL_FOREIGN?BERR_OWNER:r==TRACK_JOURNAL_INVALID?BERR_FORMAT:BERR_IO);return 1;
  }
  out[1]=slot;beetle_xfer_reply(0x66u,out,2u);return 1;
 }
 if(cmd==0x59u){
  /* V2 physical DELETE: 0x59, seq, slot, 0xD3, 0x7E.
   * Never use this command as a way to clear unowned flash. */
  if(n!=4u||arg[1]>=TRACK_PROJECTS||arg[2]!=0xD3u||arg[3]!=0x7Eu){
   beetle_xfer_error(seq,cmd,BERR_FORMAT);return 1;
  }
  const uint8_t slot=arg[1];
  /* A live session must be detached via NEW or another LOAD on FM1 first. */
  if(track_session_saved_slot==slot ||
     (track_state.dirty&&track_state.project_slot==slot)){
   beetle_xfer_error(seq,cmd,BERR_BUSY);return 1;
  }
  /* Full decode verifies that the slot contains a legitimate project.
   * The journal DELETE routine independently validates both banks. */
  int r=track_journal_load(&beetle_hal_journal_io,slot,&beetle_staged,
             beetle_codec_buffer,sizeof(beetle_codec_buffer),&beetle_last_journal);
  if(r!=TRACK_JOURNAL_OK){beetle_xfer_error(seq,cmd,
   r==TRACK_JOURNAL_NO_PROJECT?BERR_EMPTY:
   r==TRACK_JOURNAL_FOREIGN?BERR_OWNER:BERR_IO);return 1;}
  native_pause();
  beetle_write_permit=1u;
  r=track_journal_delete(&beetle_hal_journal_io,slot);
  beetle_write_permit=0u;
  if(r==TRACK_JOURNAL_OK){
   track_slot_seen[slot]=0u;track_load_status[slot]=2u;
  }
  native_start();
  if(r!=TRACK_JOURNAL_OK){beetle_xfer_error(seq,cmd,
   r==TRACK_JOURNAL_NO_PROJECT?BERR_EMPTY:
   r==TRACK_JOURNAL_FOREIGN?BERR_OWNER:BERR_IO);return 1;}
  out[1]=slot;beetle_xfer_reply(0x66u,out,2u);return 1;
 }
 if(cmd==0x51u){
  if(n!=2u||arg[1]>=TRACK_PROJECTS){beetle_xfer_error(seq,cmd,BERR_SLOT);return 1;}
  uint8_t slot=arg[1];char title[13]={0};
  int r=track_journal_preview(&beetle_hal_journal_io,slot,title);
  out[1]=slot;out[2]=(uint8_t)(r==TRACK_JOURNAL_OK?1u:r==TRACK_JOURNAL_NO_PROJECT?0u:2u);
  for(uint8_t j=0u;j<12u;j++)out[3u+j]=(uint8_t)(out[2]==1u?title[j]:' ');
  beetle_xfer_reply(0x61u,out,15u);return 1;
 }
 if(cmd==0x52u){
  if(n!=2u||arg[1]>=TRACK_PROJECTS){beetle_xfer_error(seq,cmd,BERR_SLOT);return 1;}
  int r=track_journal_load(&beetle_hal_journal_io,arg[1],&beetle_staged,
          beetle_codec_buffer,sizeof(beetle_codec_buffer),&beetle_last_journal);
  if(r!=TRACK_JOURNAL_OK){beetle_xfer_error(seq,cmd,
    r==TRACK_JOURNAL_NO_PROJECT?BERR_EMPTY:r==TRACK_JOURNAL_FOREIGN?BERR_OWNER:BERR_IO);return 1;}
  if(beetle_last_journal.payload_bytes>BEETLE_XFER_CAP){beetle_xfer_error(seq,cmd,BERR_CAPACITY);return 1;}
  beetle_xfer.kind=1u;beetle_xfer.slot=arg[1];beetle_xfer.position=0u;
  beetle_xfer.length=(uint16_t)beetle_last_journal.payload_bytes;
  beetle_xfer.expected_crc=track_store_crc32(beetle_codec_buffer,beetle_xfer.length);
  beetle_xfer.last_ms=etch_ms;
  out[1]=beetle_xfer.slot;out[2]=(uint8_t)beetle_xfer.length;out[3]=(uint8_t)(beetle_xfer.length>>8);
  beetle_xfer_wr32(out+4u,beetle_xfer.expected_crc);
  out[8]=(uint8_t)beetle_codec_buffer[4];
  for(uint8_t j=0u;j<12u;j++)out[9u+j]=(uint8_t)beetle_staged.title[j];
  beetle_xfer_reply(0x62u,out,21u);return 1;
 }
 if(cmd==0x53u){
  if(n!=4u||beetle_xfer.kind!=1u){beetle_xfer_error(seq,cmd,BERR_STATE);return 1;}
  uint16_t off=(uint16_t)(arg[1]|(uint16_t)arg[2]<<8u);
  uint8_t len=arg[3];
  if(!len||len>BEETLE_XFER_CHUNK||(uint32_t)off+len>beetle_xfer.length){
   beetle_xfer_error(seq,cmd,BERR_LENGTH);return 1;
  }
  out[1]=(uint8_t)off;out[2]=(uint8_t)(off>>8);out[3]=len;
  for(uint8_t j=0u;j<len;j++)out[4u+j]=beetle_codec_buffer[off+j];
  beetle_xfer.last_ms=etch_ms;beetle_xfer_reply(0x63u,out,4u+len);return 1;
 }
 if(cmd==0x55u){
  if(n!=10u || arg[1]>=TRACK_PROJECTS){beetle_xfer_error(seq,cmd,BERR_LENGTH);return 1;}
  uint16_t len=(uint16_t)(arg[2]|(uint16_t)arg[3]<<8u);
  if(len<42u||len>BEETLE_XFER_CAP){beetle_xfer_error(seq,cmd,BERR_CAPACITY);return 1;}
  if(track_state.dirty && (track_session_saved_slot==arg[1] || track_state.project_slot==arg[1])){
   beetle_xfer_error(seq,cmd,BERR_BUSY);return 1;
  }
  uint8_t overwrite=arg[8];
  if(overwrite>1u||arg[9]!=0x42u){beetle_xfer_error(seq,cmd,BERR_FORMAT);return 1;}
  /* Inspect BOTH banks, fully, before authorizing a non-empty overwrite. */
  int found=track_journal_load(&beetle_hal_journal_io,arg[1],&beetle_staged,
                         beetle_codec_buffer,sizeof(beetle_codec_buffer),&beetle_last_journal);
  if(found!=TRACK_JOURNAL_OK && found!=TRACK_JOURNAL_NO_PROJECT){
   beetle_xfer_error(seq,cmd,found==TRACK_JOURNAL_FOREIGN?BERR_OWNER:BERR_IO);return 1;
  }
  if(found==TRACK_JOURNAL_OK&&!overwrite){beetle_xfer_error(seq,cmd,BERR_OCCUPIED);return 1;}
  beetle_xfer.kind=2u;beetle_xfer.slot=arg[1];beetle_xfer.length=len;
  beetle_xfer.expected_crc=beetle_xfer_rd32(arg+4u);beetle_xfer.position=0u;
  beetle_xfer.last_ms=etch_ms;
  out[1]=beetle_xfer.slot;beetle_xfer_reply(0x64u,out,2u);return 1;
 }
 if(cmd==0x56u){
  if(n<5u||beetle_xfer.kind!=2u){beetle_xfer_error(seq,cmd,BERR_STATE);return 1;}
  uint16_t off=(uint16_t)(arg[1]|(uint16_t)arg[2]<<8u);
  uint8_t len=arg[3];
  if(!len||len>BEETLE_XFER_CHUNK||n!=4u+len||(uint32_t)off+len>beetle_xfer.length){
   beetle_xfer_error(seq,cmd,BERR_LENGTH);return 1;
  }
  if(off==beetle_xfer.position){
   for(uint8_t j=0u;j<len;j++)beetle_codec_buffer[off+j]=arg[4u+j];
   beetle_xfer.position=(uint16_t)(off+len);
  }else if((uint32_t)off+len<=beetle_xfer.position){
   for(uint8_t j=0u;j<len;j++)if(beetle_codec_buffer[off+j]!=arg[4u+j]){
    beetle_xfer_error(seq,cmd,BERR_STATE);return 1;
   }
  }else{beetle_xfer_error(seq,cmd,BERR_STATE);return 1;}
  out[1]=(uint8_t)beetle_xfer.position;out[2]=(uint8_t)(beetle_xfer.position>>8);
  beetle_xfer.last_ms=etch_ms;beetle_xfer_reply(0x64u,out,3u);return 1;
 }
 if(cmd==0x57u){
  if(n!=1u||beetle_xfer.kind!=2u){beetle_xfer_error(seq,cmd,BERR_STATE);return 1;}
  if(beetle_xfer.position!=beetle_xfer.length){beetle_xfer_error(seq,cmd,BERR_LENGTH);return 1;}
  if(track_store_crc32(beetle_codec_buffer,beetle_xfer.length)!=beetle_xfer.expected_crc){
   beetle_xfer_reset();beetle_xfer_error(seq,cmd,BERR_CRC);return 1;
  }
  if(beetle_codec_buffer[6u]>=TRACK_PROJECTS || beetle_codec_buffer[7u]!=0u ||
     beetle_codec_buffer[36u]!=beetle_codec_buffer[6u] ||
     track_store_decode(&beetle_staged,beetle_codec_buffer,beetle_xfer.length)!=0){
   beetle_xfer_reset();beetle_xfer_error(seq,cmd,BERR_FORMAT);return 1;
  }
  uint8_t slot=beetle_xfer.slot;
  if(track_state.dirty && (track_session_saved_slot==slot || track_state.project_slot==slot)){
   beetle_xfer_reset();beetle_xfer_error(seq,cmd,BERR_BUSY);return 1;
  }
  beetle_staged.slot=slot; /* reassign slot. Save encoder regenerates TRK1 inner CRC. */
  beetle_xfer_reset(); /* do not accept another chunk during irreversible flash I/O */
  native_pause();
  int r=beetle_project_save_staged(); /* existing journal owns bank checks & A/B commit */
  if(r==TRACK_JOURNAL_OK){
   track_slot_seen[slot]=1u;track_load_status[slot]=0u; /* refresh browser */
   if(track_session_saved_slot==slot){fm1_irq_off();beetle_project_apply();fm1_irq_enable_all();}
  }
  native_start();
  if(r!=TRACK_JOURNAL_OK){beetle_xfer_error(seq,cmd,r==TRACK_JOURNAL_CAPACITY?BERR_CAPACITY:
                 r==TRACK_JOURNAL_FOREIGN?BERR_OWNER:BERR_IO);return 1;}
  out[1]=slot;beetle_xfer_reply(0x65u,out,2u);return 1;
 }
 beetle_xfer_error(seq,cmd,BERR_STATE);return 1;
#endif
}
