"""Compile the actual DE startup save function against fault-injected host IO."""
from pathlib import Path
import argparse,ctypes as C,json,tempfile
ROOT=Path(__file__).resolve().parents[1]
STUBS=r'''
#include <string.h>
typedef unsigned u32;typedef unsigned short u16;typedef char TCHAR;
#define FR_OK 0
#define FR_EXIST 8
#define SAVER_FOLDER "/SYSTEM/SAVER"
static unsigned char SAV_info_buffer[512];
static unsigned flag=0x11,cleared,writes,errors,fail_write,fail_path;
static u16 Read_sav_info(unsigned i){if(i==0)return flag;if(i==1)return 2;return i==2?'A':0;}
static void UIAudio_StopForSharedBufferUse(void){}
static unsigned f_mkdir(const char *p){return fail_path?1:FR_EXIST;}
static unsigned f_chdir(const char *p){return 0;}
static unsigned Save_savefile(TCHAR *name,unsigned size){writes++;return !fail_write && name[0]=='A' && size==1024;}
static void Save_sav_info(void *p,unsigned n){flag=0;cleared++;}
static void Show_error_num(unsigned n){errors++;}
'''
TESTS=r'''
__declspec(dllexport) unsigned run_tests(void){
 unsigned failed=0;
 Check_save_flag();if(cleared!=1 || writes!=1 || flag!=0 || errors)failed++;
 Check_save_flag();if(writes!=1 || cleared!=1)failed++;
 flag=0x11;fail_write=1;Check_save_flag();if(flag!=0x11 || cleared!=1 || errors!=1)failed++;
 fail_write=0;fail_path=1;Check_save_flag();if(writes!=2 || flag!=0x11 || errors!=2)failed++;
 fail_path=0;Check_save_flag();if(flag || cleared!=2 || writes!=3)failed++;
 return failed;
}
'''
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--tcc',type=Path,required=True);a=p.parse_args()
    source=(ROOT/'kernels/omega-de/source/ezkernelnew.c').read_bytes().decode('latin1')
    start=source.index('void Check_save_flag(void)\n{');end=source.index('//---------------------------------------------------------------------------------',start)
    with tempfile.TemporaryDirectory(dir=ROOT.parent.parent/'work') as tmp:
        folder=Path(tmp);file=folder/'save.c';file.write_text(STUBS+source[start:end]+TESTS,encoding='utf-8')
        base=a.tcc.resolve();tcc=C.CDLL(str(base/'libtcc.dll'));tcc.tcc_new.restype=C.c_void_p
        for name,args in [('tcc_set_lib_path',[C.c_void_p,C.c_char_p]),('tcc_add_include_path',[C.c_void_p,C.c_char_p]),('tcc_add_library_path',[C.c_void_p,C.c_char_p]),('tcc_set_output_type',[C.c_void_p,C.c_int]),('tcc_add_file',[C.c_void_p,C.c_char_p]),('tcc_output_file',[C.c_void_p,C.c_char_p]),('tcc_delete',[C.c_void_p])]:getattr(tcc,name).argtypes=args
        state=tcc.tcc_new()
        try:
            tcc.tcc_set_lib_path(state,str(base).encode())
            tcc.tcc_add_include_path(state,str(base/'include').encode());tcc.tcc_add_library_path(state,str(base/'lib').encode())
            assert tcc.tcc_set_output_type(state,3)==0
            assert tcc.tcc_add_file(state,str(file).encode())==0
            dll=folder/'save.dll';assert tcc.tcc_output_file(state,str(dll).encode())==0
        finally:tcc.tcc_delete(state)
        lib=C.CDLL(str(dll));lib.run_tests.restype=C.c_uint;failed=lib.run_tests();handle=lib._handle;del lib
        C.windll.kernel32.FreeLibrary.argtypes=[C.c_void_p];C.windll.kernel32.FreeLibrary(handle)
        assert failed==0,failed
    report={'status':'pass','checks':5,'failed':0,'scope':'Actual DE Check_save_flag compiled against mocked save-info and SD IO: absent flag, success, failed write, failed path, retry. No physical save write-back claim.'}
    (ROOT/'validation/startup-save-validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');print(json.dumps(report))
if __name__=='__main__':main()
