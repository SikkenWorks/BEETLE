/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE original, transaction-bound BD4S transfer protocol. Main loop only.
 * 0x70 INFO, 0x71 BEGIN, 0x72 DATA, 0x73 COMMIT, 0x74 ABORT,
 * 0x75 DELETE, 0x76 STORAGE CAPACITY (read-only). Replies 0x78-0x7E, errors 0x6F.
 * This protocol is intentionally distinct from OTA and never chooses an
 * arbitrary flash address. The destination is one verified V2 sample slot.
 */
#if defined(BEETLE_NATIVE_V2) && BEETLE_NATIVE_V2
static uint8_t beetle_sample_xfer_active;
static uint32_t beetle_sample_xfer_last;
/* Project staging is otherwise idle during a stopped transfer, saving .pool
 * RAM. It is locked from other SAVE/LOAD operations while active. */
static uint8_t *beetle_sx_ram(void){return beetle_v2_workspace();}
static struct beetle_v2_writer *beetle_sx_writer(void){return (struct beetle_v2_writer *)(void *)(beetle_sx_ram()+16384u);}
static struct beetle_v2_sample_upload *beetle_sx_up(void){return (struct beetle_v2_sample_upload *)(void *)(beetle_sx_ram()+17024u);}
static uint8_t *beetle_sx_index(void){return beetle_sx_ram();}
static uint8_t *beetle_sx_other(void){return beetle_sx_ram()+4096u;}
static uint8_t *beetle_sx_chunk(void){return beetle_sx_ram()+8192u;}
static uint8_t *beetle_sx_page(void){return beetle_sx_ram()+24576u;}
static void beetle_sx_abort(void){
 uint8_t was_active=beetle_sample_xfer_active;
 beetle_sample_xfer_active=0u;
 beetle_v2_sample_upload_abort(beetle_sx_up());
 if(was_active){beetle_bd4_live_bind(beetle_v2_workspace(),sizeof(beetle_staged));native_start();}
}
/* A vanished host must never leave FM1 paused indefinitely. */
static void beetle_sample_transfer_maintenance(void){
 if(beetle_sample_xfer_active &&
    (uint32_t)(etch_ms-beetle_sample_xfer_last)>BEETLE_XFER_TTL_MS)
  beetle_sx_abort();
}
static int beetle_sx_open(void){
 return beetle_v2_writer_open(beetle_sx_writer(),beetle_v2_hal_read,
  beetle_v2_hal_program,beetle_v2_hal_erase,0,
  beetle_sx_index(),beetle_sx_other());
}
#endif
static int beetle_sample_transfer_handle(uint8_t cmd,const uint8_t *arg,uint32_t n){
 if(cmd<0x70u||cmd>0x76u)return 0;
 if(n<1u)return 1;
 uint8_t seq=arg[0],out[16]={seq};
#if !defined(BEETLE_NATIVE_V2) || !BEETLE_NATIVE_V2
 beetle_xfer_error(seq,cmd,BERR_STORAGE);return 1;
#else
 if(beetle_sample_xfer_active && (uint32_t)(etch_ms-beetle_sample_xfer_last)>BEETLE_XFER_TTL_MS)
  beetle_sx_abort();
 if(cmd==0x74u){beetle_sx_abort();beetle_xfer_reply(0x7Cu,out,1u);return 1;}
 if(cmd==0x70u){
  if(n!=2u||arg[1]>=BEETLE_V2_SAMPLES){beetle_xfer_error(seq,cmd,BERR_SLOT);return 1;}
  if(beetle_sample_xfer_active||beetle_transfer_active()||!beetle_xfer_ready()){
   beetle_xfer_error(seq,cmd,BERR_BUSY);return 1;}
  if(beetle_v2_mode()!=1){beetle_xfer_error(seq,cmd,BERR_OWNER);return 1;}
  struct beetle_v2_device d;struct beetle_v2_sample info;
  /* INFO uses project scratch that temporarily backs DPCM4 stream state.
   * Stop audio ownership before a single byte of that scratch is touched. */
  native_pause();
  uint8_t *scratch=beetle_v2_workspace();
  int opened=beetle_v2_open(&d,beetle_v2_hal_read,0,scratch);
  int r=opened?-1:beetle_v2_sample_info(&d,arg[1],&info,scratch);
  beetle_bd4_live_bind(beetle_v2_workspace(),sizeof(beetle_staged));
  native_start();
  if(r<0){beetle_xfer_error(seq,cmd,BERR_IO);return 1;}
  out[1]=arg[1];out[2]=(uint8_t)(r==1);
  beetle_xfer_wr32(out+3u,r==1?info.byte_length:0u);
  beetle_xfer_wr32(out+7u,r==1?info.crc:0u);
  beetle_xfer_wr32(out+11u,r==1?info.generation:0u);
  beetle_xfer_reply(0x78u,out,15u);return 1;
 }
 if(cmd==0x76u){
  /* Read-only truthful flash budget. Use exactly the same union-of-two-indexes
   * ownership model as the sample writer. No writes or erases. */
  if(n!=1u){beetle_xfer_error(seq,cmd,BERR_FORMAT);return 1;}
  if(beetle_sample_xfer_active||beetle_transfer_active()||!beetle_xfer_ready()){
   beetle_xfer_error(seq,cmd,BERR_BUSY);return 1;}
  if(beetle_v2_mode()!=1){beetle_xfer_error(seq,cmd,BERR_OWNER);return 1;}
  native_pause();beetle_bd4_live_invalidate();
  int r=beetle_sx_open();
  struct beetle_v2_sample_capacity cap;
  if(!r)r=beetle_v2_get_sample_capacity(beetle_sx_writer(),
     beetle_sx_index(),beetle_sx_page(),&cap);
  beetle_bd4_live_bind(beetle_v2_workspace(),sizeof(beetle_staged));native_start();
  if(r){beetle_xfer_error(seq,cmd,BERR_IO);return 1;}
  out[1]=1u;/* capacity protocol */
  beetle_xfer_wr32(out+2u,cap.current_sample_bytes);
  beetle_xfer_wr32(out+6u,cap.estimated_additional_bytes);
  out[10]=(uint8_t)cap.erased_free_sectors;
  out[11]=BEETLE_V2_PROJECT_RESERVE_SECTORS;
  out[12]=BEETLE_V2_GUARD_SECTORS;
  out[13]=(uint8_t)cap.sample_blank_pages;
  out[14]=(uint8_t)(cap.sample_blank_pages>>8);
  beetle_xfer_reply(0x7Eu,out,15u);return 1;
 }
 if(cmd==0x71u){
  /* seq,slot,len32,CRC32,0x42. Reject before any NOR write. */
  if(n!=11u||arg[1]>=BEETLE_V2_SAMPLES||arg[10]!=0x42u){
   beetle_xfer_error(seq,cmd,BERR_FORMAT);return 1;}
  if(beetle_sample_xfer_active||beetle_transfer_active()||!beetle_xfer_ready()||track_state.dirty){
   beetle_xfer_error(seq,cmd,BERR_BUSY);return 1;}
  if(beetle_v2_mode()!=1){beetle_xfer_error(seq,cmd,BERR_OWNER);return 1;}
  uint32_t length=beetle_xfer_rd32(arg+2u),crc=beetle_xfer_rd32(arg+6u);
  if(length<22u||length>250000u){beetle_xfer_error(seq,cmd,BERR_CAPACITY);return 1;}
  /* Reserve scratch ownership BEFORE opening the flash writer. */
  native_pause();beetle_bd4_live_invalidate();
  int r=beetle_sx_open();
  if(!r)r=beetle_v2_sample_upload_begin(beetle_sx_writer(),beetle_sx_up(),
     arg[1],length,crc,beetle_sx_index(),beetle_sx_other(),beetle_sx_page());
  if(r){
   beetle_bd4_live_bind(beetle_v2_workspace(),sizeof(beetle_staged));
   native_start();beetle_xfer_error(seq,cmd,r==-2?BERR_CAPACITY:BERR_IO);return 1;
  }
  beetle_io_input_begin(&track_io_input_guard,etch_ms);
  native_edit_keys_blocked=1u;
  beetle_sample_xfer_active=1u;beetle_sample_xfer_last=etch_ms;
  out[1]=arg[1];beetle_xfer_reply(0x79u,out,2u);return 1;
 }
 if(cmd==0x72u){
  if(!beetle_sample_xfer_active||n<7u){beetle_xfer_error(seq,cmd,BERR_STATE);return 1;}
  uint32_t offset=beetle_xfer_rd32(arg+1u);uint8_t len=arg[5];
  struct beetle_v2_sample_upload *u=beetle_sx_up();
  if(!len||len>BEETLE_XFER_CHUNK||n!=6u+len||offset!=u->received){
   beetle_xfer_error(seq,cmd,BERR_STATE);return 1;}
  /* Flash writes are allowed only for the prepared, bounded V2 transaction.
   * The audio DMA is paused around each full chunk, never accessed in ISR. */
  beetle_write_permit=1u;
  int r=beetle_v2_sample_upload_feed(beetle_sx_writer(),u,arg+6u,len,
    beetle_sx_index(),beetle_sx_page(),beetle_sx_chunk());
  beetle_write_permit=0u;
  if(r){beetle_sx_abort();beetle_xfer_error(seq,cmd,BERR_IO);return 1;}
  beetle_xfer_wr32(out+1u,u->received);
  beetle_sample_xfer_last=etch_ms;
  beetle_xfer_reply(0x7Au,out,5u);return 1;
 }
 if(cmd==0x73u){
  if(!beetle_sample_xfer_active||n!=1u){beetle_xfer_error(seq,cmd,BERR_STATE);return 1;}
  struct beetle_v2_sample_upload *u=beetle_sx_up();
  uint8_t slot=(uint8_t)u->slot;
  if(u->received!=u->total){beetle_xfer_error(seq,cmd,BERR_LENGTH);return 1;}
  beetle_write_permit=1u;
  int r=beetle_v2_sample_upload_commit(beetle_sx_writer(),u,
   beetle_sx_index(),beetle_sx_other(),beetle_sx_page(),beetle_sx_chunk());
  beetle_write_permit=0u;beetle_bd4_live_invalidate();beetle_sx_abort();
  if(r){beetle_xfer_error(seq,cmd,r==-4?BERR_CRC:BERR_IO);return 1;}
  out[1]=slot;beetle_xfer_reply(0x7Bu,out,2u);return 1;
 }
 if(cmd==0x75u){
  if(n!=4u||arg[1]>=BEETLE_V2_SAMPLES||arg[2]!=0xD3u||arg[3]!=0x7Eu){
   beetle_xfer_error(seq,cmd,BERR_FORMAT);return 1;}
  if(beetle_sample_xfer_active||beetle_transfer_active()||!beetle_xfer_ready()||track_state.dirty){
   beetle_xfer_error(seq,cmd,BERR_BUSY);return 1;}
  if(beetle_v2_mode()!=1){beetle_xfer_error(seq,cmd,BERR_OWNER);return 1;}
  native_pause();beetle_bd4_live_invalidate();
  int r=beetle_sx_open();
  if(!r){beetle_io_input_begin(&track_io_input_guard,etch_ms);
   beetle_write_permit=1u;
   r=beetle_v2_remove_sample(beetle_sx_writer(),arg[1],
    beetle_sx_index(),beetle_sx_other(),beetle_sx_page());
   beetle_write_permit=0u;
  }
  beetle_bd4_live_bind(beetle_v2_workspace(),sizeof(beetle_staged));native_start();
  if(r){beetle_xfer_error(seq,cmd,r==-2?BERR_CAPACITY:BERR_IO);return 1;}
  out[1]=arg[1];beetle_xfer_reply(0x7Du,out,2u);return 1;
 }
 beetle_xfer_error(seq,cmd,BERR_STATE);return 1;
#endif
}
