"""Build the ordinary GBA rendering-preview ROM with devkitPro gba-dev."""
from pathlib import Path
import argparse,shutil,os
from build import ROOT,tool_path,run

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--devkitpro',type=Path,default=Path('C:/devkitPro' if os.name=='nt' else '/opt/devkitpro'))
    parser.add_argument('--build-root',type=Path,default=ROOT/'work/preview')
    args=parser.parse_args();p=args.devkitpro.resolve();out=args.build_root.resolve();out.mkdir(parents=True,exist_ok=True)
    gcc=tool_path(p/'devkitARM/bin','arm-none-eabi-gcc')
    run([gcc,'-Os','-g','-mcpu=arm7tdmi','-mthumb','-mthumb-interwork','-ffunction-sections','-fdata-sections','-I'+str(ROOT/'common'),'-I'+str(p/'libgba/include'),'-specs=gba.specs',ROOT/'tests/preview.c',ROOT/'tests/hold_reset_harness.s',ROOT/'common/slot_blit.s','-Wa,-I'+str(ROOT/'common'),'-L'+str(p/'libgba/lib'),'-lgba','-Wl,-Map,'+str(out/'preview.map'),'-o',out/'slot-preview.elf'],out/'build.log')
    run([tool_path(p/'devkitARM/bin','arm-none-eabi-objcopy'),'-O','binary',out/'slot-preview.elf',out/'slot-preview.gba'],out/'objcopy.log')
    run([tool_path(p/'tools/bin','gbafix'),out/'slot-preview.gba','-tSLOT PREVIEW'],out/'gbafix.log')
    from boot_entry import install_black_entry
    install_black_entry(out/'slot-preview.gba',p,out)
    release=ROOT/'preview';release.mkdir(exist_ok=True);shutil.copy2(out/'slot-preview.gba',release/'slot-preview.gba')
    print('Built',release/'slot-preview.gba')
if __name__=='__main__':main()
