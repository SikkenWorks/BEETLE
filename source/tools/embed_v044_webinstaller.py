#!/usr/bin/env python3
"""Update only embedded binary + immutable expected SHA in existing installer.
Requires source installer .html generated from audited V0.43 stock patch.
Does not modify OTA-loader or firmware contents.
"""
import re,hashlib,base64,sys,pathlib
html=pathlib.Path(sys.argv[1]).read_text();fw=pathlib.Path(sys.argv[2]).read_bytes()
ver=sys.argv[3] if len(sys.argv)>3 else 'FM-1_2044'
html=re.sub(r"HASH_ET: '[0-9a-f]{64}'",f"HASH_ET: '{hashlib.sha256(fw).hexdigest()}'",html,count=1)
html=re.sub(r"ID_ET: 'FM-1_\d+'",f"ID_ET: '{ver}'",html,count=1)
html,n=re.subn(r'(<script id="firmware-embedded" type="application/octet-stream">\s*)([\s\S]*?)(\s*</script>)',lambda m:m.group(1)+base64.b64encode(fw).decode()+m.group(3),html,count=1)
if n!=1:sys.exit('firmware-embedded missing')
pathlib.Path(sys.argv[4]).write_text(html)
