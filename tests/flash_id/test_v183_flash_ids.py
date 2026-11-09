#!/usr/bin/env python3
"""Strict host validation: both FM1 JEDEC IDs and every flash write path."""
from pathlib import Path
import subprocess,tempfile,re
r=Path(__file__).resolve().parents[2]
app=(r/'firmware/fm1-native/hal/fm1_flash.h').read_text()
loader=(r/'firmware/hal/fm1_flash.h').read_text()
pattern=r'#define FL_JEDEC_PUYA_P25Q80[^\n]*\n#define FL_JEDEC_UCUN_UC25HQ80[^\n]*\n#define FL_JEDEC_OK\(id\)[^\n]*\n[^\n]*'
a=re.search(pattern,app); b=re.search(pattern,loader)
assert a and b and a.group()==b.group(), 'App and loader JEDEC policy must be identical'
assert '0x856014u' in a.group() and '0xB36014u' in a.group()
c='#include <stdint.h>\n#include <stdio.h>\n'+a.group()+'''
int main(void){
 if(!FL_JEDEC_OK(0x856014u) || !FL_JEDEC_OK(0xB36014u))return 1;
 uint32_t bad[]={0u,0xffffffffu,0x856015u,0xB36015u,0xffffffu,0x146085u,0x14u,0x186014u};
 for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++)if(FL_JEDEC_OK(bad[i]))return 2;
 return 0;
}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t)/'ids.c';p.write_text(c)
 subprocess.run(['cc','-std=c11','-Wall','-Werror',str(p),'-o',str(Path(t)/'ids')],check=True)
 subprocess.run([str(Path(t)/'ids')],check=True)
s=(r/'firmware/fm1-native/beetle_flash_target.inc').read_text()
for name in ('beetle_hal_journal_erase','beetle_hal_journal_program','beetle_hal_restore_marker',
             'beetle_hal_wipe_projects','beetle_hal_init_exclusive','beetle_v2_hal_program','beetle_v2_hal_erase'):
 i=s.index('static int '+name+'(')
 assert 'beetle_nor_id_ok()' in s[i:i+650],f'missing flash ID guard: {name}'
o=(r/'firmware/fm1-native/ota_target.c').read_text()
for name in ('ota_erase','ota_prog'):
 i=o.index('static int '+name+'(')
 assert 'beetle_nor_id_ok()' in o[i:i+400]
l=(r/'firmware/loader/loader.c').read_text()
assert 'static int ldr_flash_known(void)' in l
assert 'if (ldr_flash_known() && ldr_session() == 0)' in l
print('PASS dual JEDEC ID acceptance, unknown refusal, all BEETLE storage/OTA writes and native loader guard')
