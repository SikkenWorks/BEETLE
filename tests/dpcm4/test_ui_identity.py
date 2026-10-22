from pathlib import Path
root=Path(__file__).resolve().parents[2]
source=(root/'firmware/fm1-native/track_runtime.h').read_text()
assert 'track_show_storage_full(1u)' in source
assert 'track_show_storage_full(2u)' in source
assert 'track_storage_full_popup_draw();etch_panel_commit()' in source
assert source.count('track_storage_full_popup_draw();etch_panel_commit()')==2
assert 'PROJECT NOT SAVED' in source and 'STORAGE FULL' in source
identity=(root/'firmware/fm1-native/bringup.c').read_text()
assert '#define BEETLE_PRODUCT_ID "FM-1_2123"' in identity
assert "'product':'FM-1_2123'" in (root/'firmware/fm1-native/build.py').read_text()
manager=(root/'BEETLE-Project-Manager.html').read_text()
assert "'STORAGE FULL'" in manager and 'Nothing was overwritten.' in manager
print('V1.4.8 identity and storage-capacity overlay contract: PASS')
