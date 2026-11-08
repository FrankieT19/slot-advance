"""Test the actual one-shot SRAM reader, including retained SRAM on cold start."""
from pathlib import Path
import argparse,ctypes as C,json,subprocess
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--tcc',type=Path,required=True);a=p.parse_args()
    work=ROOT/'work/return-io-test';work.mkdir(parents=True,exist_ok=True);dll=work/'return-io.dll'
    subprocess.run([str(a.tcc.resolve()/'tcc.exe'),'-shared','-I'+str(ROOT/'common'),str(ROOT/'tests/return_io_host.c'),'-o',str(dll)],check=True)
    lib=C.CDLL(str(dll));lib.run_tests.restype=C.c_uint;failed=lib.run_tests();assert not failed
    report={'status':'pass','checks':63,'failed':0,'scope':'Actual Slot_ReadReturn/Slot_ArmReturn with SRAM and volatile-cookie mocks: one-shot consumption, cold start with retained SRAM, torn/bad cookies and records, safe basename, disabled arming and banking/buffer guards.'}
    (ROOT/'validation/return-io-validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
