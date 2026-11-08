"""Verify the warm cookie through the real CRT, rather than mocked startup."""
from pathlib import Path
import argparse,ctypes as C,json,subprocess
from test_preview import Emulator
from boot_entry import install_black_entry
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--core',type=Path,required=True);a=p.parse_args()
    work=ROOT/'work/boot-return-test';work.mkdir(parents=True,exist_ok=True)
    sdk=Path('C:/devkitPro');gcc=sdk/'devkitARM/bin/arm-none-eabi-gcc.exe'
    elf=work/'boot-return.elf';rom=work/'boot-return.gba'
    subprocess.run([str(gcc),'-Os','-mcpu=arm7tdmi','-marm','-specs=gba.specs','-I'+str(ROOT/'common'),'-I'+str(sdk/'libgba/include'),str(ROOT/'tests/boot_return_harness.c'),'-L'+str(sdk/'libgba/lib'),'-lgba','-o',str(elf)],check=True)
    subprocess.run([str(sdk/'devkitARM/bin/arm-none-eabi-objcopy.exe'),'-O','binary',str(elf),str(rom)],check=True)
    subprocess.run([str(sdk/'tools/bin/gbafix.exe'),str(rom)],check=True,stdout=subprocess.DEVNULL)
    install_black_entry(rom,sdk,work)
    e=Emulator(a.core);e.load(rom);e.run(80)
    def result(i):return C.c_uint.from_address(e.ewram[0]+0x3f000+i*4).value
    assert result(0)==0x42543132 and result(1)==0,'cold start was mistaken for ROM return'
    C.c_uint.from_address(e.ewram[0]+0x3f040).value=1;e.run(80)
    assert result(0)==0x42543132 and result(1)==1,'actual CRT erased the warm cookie'
    assert result(2)==0,'ordinary work RAM was not cleared'
    e.lib.retro_reset();e.run(80)
    assert result(0)==0x42543132 and result(1)==0,'hard reset replayed the warm cookie'
    e.close()
    report={'status':'pass','checks':4,'scope':'Patched real devkitARM CRT: cold boot rejects cookie; soft re-entry preserves cookie while clearing other EWRAM; emulator hard reset removes cookie. Cartridge firmware before kernel entry still requires hardware verification.'}
    (ROOT/'validation/boot-return-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
