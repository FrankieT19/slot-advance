"""Capture real fade/reset/CRT frames and reject a white or un-faded handoff."""
from pathlib import Path
import argparse,ctypes as C,json,subprocess
from test_preview import Emulator
from boot_entry import install_black_entry
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--core',type=Path,required=True);a=p.parse_args()
    out=ROOT/'work/emulator-reset-test';out.mkdir(parents=True,exist_ok=True)
    sdk=Path('C:/devkitPro');gcc=sdk/'devkitARM/bin/arm-none-eabi-gcc.exe';elf=out/'reset.elf';rom=out/'reset.gba'
    subprocess.run([str(gcc),'-Os','-mcpu=arm7tdmi','-marm','-specs=gba.specs','-I'+str(ROOT/'common'),'-I'+str(sdk/'libgba/include'),str(ROOT/'tests/emulator_reset_harness.c'),str(ROOT/'common/slot_emulator_reset.s'),'-L'+str(sdk/'libgba/lib'),'-lgba','-o',str(elf)],check=True)
    subprocess.run([str(sdk/'devkitARM/bin/arm-none-eabi-objcopy.exe'),'-O','binary',str(elf),str(rom)],check=True)
    subprocess.run([str(sdk/'tools/bin/gbafix.exe'),str(rom)],check=True,stdout=subprocess.DEVNULL)
    install_black_entry(rom,sdk,out)
    e=Emulator(a.core);e.load(rom);e.run(80)
    def result(i):return C.c_uint.from_address(e.ewram[0]+0x3f000+i*4).value
    assert result(0)==0x42524644 and result(1)==0
    C.c_uint.from_address(e.ewram[0]+0x3f040).value=1
    frames=[];maxima=[]
    for i in range(90):
        e.run(1);raw=e.frame.tobytes();frames.append(round(sum(raw)/len(raw),3));maxima.append(max(raw))
    assert result(0)==0x42524644 and result(1)==1,'actual reset did not re-enter the CRT with the warm cookie'
    assert max(maxima)<200,('white display during handoff',max(maxima))
    black=frames.index(0)
    assert len(set(frames[:black]))>=5,('no gradual fade out',frames)
    last_black=max(i for i,x in enumerate(frames) if x==0)
    assert len(set(frames[last_black+1:]))>=5,('no gradual fade in',frames)
    assert all(frames[i+1]<=frames[i] for i in range(black)),('fade-out brightened',frames)
    assert all(frames[i+1]>=frames[i] for i in range(last_black,len(frames)-1)),('fade-in darkened',frames)
    e.close()
    report={'status':'pass','checks':6,'frames':frames,'max_channel':max(maxima),'scope':'Actual shared fade, copied production reset assembly, BIOS reset calls and black-entry CRT in mGBA. No mapper/reset shim; register writes target the emulator cart bus. Physical BIOS/cartridge display timing still requires hardware verification.'}
    (ROOT/'validation/emulator-reset-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='frames'},indent=2))
if __name__=='__main__':main()
