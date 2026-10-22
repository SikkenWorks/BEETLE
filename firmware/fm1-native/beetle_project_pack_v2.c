/* SPDX-License-Identifier: GPL-3.0-only
 * A simple BEETLE-specific bounded LZ/RLE bitstream.
 * DO NOT CALL IN REAL-TIME AUDIO OR FROM ANY INTERRUPT.
 * Preserve and validate the full original TRK1 bytes, including its CRC.
 */
#include "beetle_project_pack_v2.h"
#include <string.h>

static uint32_t crc32_buffer(const uint8_t *b, size_t n)
{
    uint32_t crc = 0xffffffffu;
    for (size_t i = 0; i < n; ++i) {
        crc ^= b[i];
        for (unsigned j = 0; j < 8; ++j)
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}
static uint16_t crc16_header(const uint8_t *p, size_t n)
{
    uint16_t crc=0xffffu;
    for(size_t i=0;i<n;i++) {
        crc ^= (uint16_t)p[i] << 8;
        for(unsigned j=0;j<8;j++)
            crc = (uint16_t)((crc<<1) ^ ((crc & 0x8000u) ? 0x1021u : 0));
    }
    return crc;
}
static void u16(uint8_t *p, uint16_t v) {p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void u32(uint8_t *p, uint32_t v) {u16(p,(uint16_t)v);u16(p+2,(uint16_t)(v>>16));}
static uint16_t r16(const uint8_t *p) {return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t r32(const uint8_t *p) {return (uint32_t)r16(p)|((uint32_t)r16(p+2)<<16);}
static unsigned hash3(const uint8_t *p)
{
    return ((unsigned)p[0] * 0x45d9u ^ (unsigned)p[1] * 0x9e37u ^ (unsigned)p[2]*0x85ebu) & 1023u;
}
static void remember(const uint8_t *src, size_t n, size_t pos, uint16_t *tab)
{
    if (pos + 2 >= n) return;
    unsigned h=hash3(src+pos)*2u;
    tab[h+1]=tab[h];
    tab[h]=(uint16_t)(pos+1u);
}
static size_t match_len(const uint8_t *b,size_t n,size_t i,size_t pos)
{
    if(pos>=i || i-pos>4096u) return 0;
    size_t len=0;
    while(len<34 && i+len<n && b[pos+len]==b[i+len]) ++len;
    return len;
}
static int emit(uint8_t *out,size_t cap,size_t *p,const uint8_t *src,size_t n)
{
    if(n>cap-*p) return 0;
    memcpy(out+*p,src,n);*p+=n;return 1;
}
int beetle_pack_project_v2(const uint8_t *source,size_t size,uint8_t *out,size_t cap,
                          size_t *written,uint16_t tab[BEETLE_PACK_V2_WORK])
{
    if (!source || !out || !written || !tab || !size || size>BEETLE_PACK_V2_MAX_INPUT)
        return BEETLE_PACK_V2_INVALID;
    if(cap < size + BEETLE_PACK_V2_HEADER) return BEETLE_PACK_V2_CAPACITY;
    memset(tab,0,BEETLE_PACK_V2_WORK*sizeof(*tab));
    size_t p=BEETLE_PACK_V2_HEADER, i=0, lit=0, lithead=0;
    /* Literal blocks are closed before a run/match token. */
    while(i<size) {
        if(p+4 > size+BEETLE_PACK_V2_HEADER)break;
        size_t zero=0, repeat=0, best=0, offset=0;
        while(zero<66 && i+zero<size && source[i+zero]==0) ++zero;
        while(repeat<34 && i+repeat<size && source[i+repeat]==source[i]) ++repeat;
        if(i+2<size) {
            unsigned h=hash3(source+i)*2u;
            for(unsigned j=0;j<2;j++) {
                if(!tab[h+j]) continue;
                size_t pos=tab[h+j]-1u;
                size_t len=match_len(source,size,i,pos);
                if(len>best) {best=len;offset=i-pos;}
            }
        }
        unsigned kind=0;size_t run=0;
        if(zero>=3 && zero>=best) {kind=1;run=zero;}
        else if(repeat>=4 && repeat>=best) {kind=2;run=repeat;}
        else if(best>=4) {kind=3;run=best;}
        if(kind) {
            if(lit) {out[lithead]=(uint8_t)(lit-1u);lit=0;}
            uint8_t token[3];size_t token_len;
            if(kind==1) {token[0]=(uint8_t)(0x80u | (run-3u));token_len=1;}
            else if(kind==2) {token[0]=(uint8_t)(0xc0u | (run-3u));token[1]=source[i];token_len=2;}
            else {token[0]=(uint8_t)(0xe0u | (run-3u));u16(token+1,(uint16_t)offset);token_len=3;}
            if(!emit(out,cap,&p,token,token_len)) break;
            for(size_t k=0;k<run;k++)remember(source,size,i+k,tab);
            i+=run;
        } else {
            if(!lit) {if(p>=cap)break;lithead=p++;}
            if(p>=cap)break;
            out[p++]=source[i];++lit;
            remember(source,size,i,tab);++i;
            if(lit==128) {out[lithead]=127;lit=0;}
        }
        /* Even a badly compressible stream must be no larger than raw. */
        if(p > size + BEETLE_PACK_V2_HEADER) break;
    }
    if(lit)out[lithead]=(uint8_t)(lit-1u);
    unsigned mode=1;
    if(i!=size || p>=size+BEETLE_PACK_V2_HEADER) {
        memcpy(out+BEETLE_PACK_V2_HEADER,source,size);
        p=size+BEETLE_PACK_V2_HEADER;mode=0;
    }
    memcpy(out,"BPK1",4);
    out[4]=1;out[5]=(uint8_t)mode;
    u16(out+6,(uint16_t)size);
    u16(out+8,(uint16_t)(p-BEETLE_PACK_V2_HEADER));
    u32(out+10,crc32_buffer(source,size));
    u16(out+14,crc16_header(out,14));
    *written=p;
    return BEETLE_PACK_V2_OK;
}
int beetle_unpack_project_v2(const uint8_t *packed,size_t n,uint8_t *out,size_t cap,size_t *written)
{
    if(!packed || !out || !written || n<BEETLE_PACK_V2_HEADER || memcmp(packed,"BPK1",4) ||
       packed[4]!=1 || packed[5]>1 ||
       r16(packed+14)!=crc16_header(packed,14))return BEETLE_PACK_V2_INVALID;
    size_t olen=r16(packed+6),clen=r16(packed+8);
    if(!olen || olen>BEETLE_PACK_V2_MAX_INPUT || clen!=n-BEETLE_PACK_V2_HEADER)
        return BEETLE_PACK_V2_INVALID;
    if(olen>cap)return BEETLE_PACK_V2_CAPACITY;
    if(!packed[5]) {
        if(clen!=olen)return BEETLE_PACK_V2_INVALID;
        memcpy(out,packed+BEETLE_PACK_V2_HEADER,olen);
    } else {
        size_t i=BEETLE_PACK_V2_HEADER,o=0;
        while(i<n) {
            uint8_t t=packed[i++]; size_t run;
            if(t<0x80) {
                run=(size_t)t+1u;
                if(run>n-i || run>olen-o)return BEETLE_PACK_V2_INVALID;
                memcpy(out+o,packed+i,run);o+=run;i+=run;
            } else if(t<0xc0) {
                run=(size_t)(t&63u)+3u;
                if(run>olen-o)return BEETLE_PACK_V2_INVALID;
                memset(out+o,0,run);o+=run;
            } else if(t<0xe0) {
                run=(size_t)(t&31u)+3u;
                if(i>=n || run>olen-o)return BEETLE_PACK_V2_INVALID;
                memset(out+o,packed[i++],run);o+=run;
            } else {
                run=(size_t)(t&31u)+3u;
                if(n-i<2 || run>olen-o)return BEETLE_PACK_V2_INVALID;
                size_t off=r16(packed+i);i+=2;
                if(off==0 || off>4096 || off>o)return BEETLE_PACK_V2_INVALID;
                for(size_t j=0;j<run;j++)out[o+j]=out[o+j-off];
                o+=run;
            }
        }
        if(o!=olen)return BEETLE_PACK_V2_INVALID;
    }
    if(crc32_buffer(out,olen)!=r32(packed+10))return BEETLE_PACK_V2_CHECKSUM;
    *written=olen;return BEETLE_PACK_V2_OK;
}
