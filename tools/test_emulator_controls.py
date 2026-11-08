"""Check the emulator controls' hold threshold, release gate and VBlank timing."""
from pathlib import Path
import argparse,ctypes as C,json,subprocess
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--tcc',type=Path,required=True);a=p.parse_args()
    work=ROOT/'work/emulator-controls-test';work.mkdir(exist_ok=True,parents=True);dll=work/'controls.dll'
    subprocess.run([str(a.tcc.resolve()/'tcc.exe'),'-shared','-I'+str(ROOT/'common'),str(ROOT/'tests/emulator_controls_host.c'),'-o',str(dll)],check=True)
    lib=C.CDLL(str(dll));lib.run_tests.restype=C.c_uint;lib.check_count.restype=C.c_uint
    assert lib.run_tests()==0
    report={'status':'pass','checks':lib.check_count(),'scope':'Select hold at 180 physical VBlanks, Start+Select priority, short presses and release reset, menu release gate, disabled hold, fast-forward repeated samples, unsigned clock wrap and removal of L+R menu chord.'}
    (ROOT/'validation/emulator-controls-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
