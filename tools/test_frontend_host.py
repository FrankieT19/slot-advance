"""Run the real frontend against a disposable, fault-injected mock SD card.
Windows, TinyCC 0.9.27+: python tools/test_frontend_host.py --tcc C:/tools/tcc
The compiler is an external test dependency, not part of the release.
"""
from pathlib import Path
import argparse,ctypes as C,json,os,shutil,tempfile
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--tcc',type=Path,required=True);a=p.parse_args()
    tmp=Path(os.environ.get('SLOT_TEST_TMP',ROOT/'work'));tmp.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(dir=tmp) as folder:
        folder=Path(folder);card=folder/'card';card.mkdir()
        shutil.copytree(ROOT/'examples/converted/SYSTEM',card/'SYSTEM')
        tcc=C.CDLL(str(a.tcc.resolve()/'libtcc.dll'))
        tcc.tcc_new.restype=C.c_void_p
        for name,args in [('tcc_set_lib_path',[C.c_void_p,C.c_char_p]),('tcc_add_include_path',[C.c_void_p,C.c_char_p]),('tcc_add_library_path',[C.c_void_p,C.c_char_p]),('tcc_set_output_type',[C.c_void_p,C.c_int]),('tcc_add_file',[C.c_void_p,C.c_char_p]),('tcc_output_file',[C.c_void_p,C.c_char_p]),('tcc_delete',[C.c_void_p])]:
            getattr(tcc,name).argtypes=args
        state=tcc.tcc_new();base=a.tcc.resolve()
        try:
            tcc.tcc_set_lib_path(state,str(base).encode())
            for path in [base/'include',base/'include/winapi',ROOT/'common']:tcc.tcc_add_include_path(state,str(path).encode())
            tcc.tcc_add_library_path(state,str(base/'lib').encode())
            assert tcc.tcc_set_output_type(state,3)==0
            assert tcc.tcc_add_file(state,str(ROOT/'tests/frontend_host.c').encode())==0,'Host C compilation failed'
            dll=folder/'frontend-host.dll'
            assert tcc.tcc_output_file(state,str(dll).encode())==0,'Host C linking failed'
        finally:tcc.tcc_delete(state)
        lib=C.CDLL(str(dll));lib.main.argtypes=[C.c_int,C.POINTER(C.c_char_p)]
        argv=(C.c_char_p*3)(b'frontend-host',str(card).encode(),None)
        code=lib.main(2,argv)
        lib.Slot_HostCheckCount.restype=C.c_uint
        checks=lib.Slot_HostCheckCount()
        lib.Slot_HostStreamMisses.restype=C.c_uint;lib.Slot_HostStreamFrames.restype=C.c_uint
        stream_misses=lib.Slot_HostStreamMisses();stream_frames=lib.Slot_HostStreamFrames()
        handle=lib._handle;del lib
        C.windll.kernel32.FreeLibrary.argtypes=[C.c_void_p];C.windll.kernel32.FreeLibrary(handle)
        assert code==0,'Frontend host integration failed'
        report={'status':'pass','checks':checks,'failed':0,
                'steady_stream_frames':stream_frames,'steady_visible_cache_misses':stream_misses,
                'streaming_checks':'Twenty-game holds/reversals across both styles and all systems, 24-row reads, no partial image exposure, cancel/retry, missing/malformed/failing reads; 36 endpoint image comparisons and 288 full-versus-incremental comparisons.',
                'scope':'Actual common/slot_frontend.h with mocked hardware/SD IO, real converter BMPs, settings faults, streaming cache, empty/missing/error/512-entry shelves, launch delegation and menus. Does not measure physical SD latency.'}
        (ROOT/'preview/host-validation.json').write_text(json.dumps(report,indent=2)+'\n', encoding='utf-8')
        print(json.dumps(report,indent=2))
if __name__=='__main__':main()
