"""Exercise the real addon assembly with emulator-only cartridge SRAM shims."""
from pathlib import Path
import argparse,ctypes as C,json,subprocess,sys
from test_preview import Emulator
ROOT=Path(__file__).resolve().parents[1]

SHIMS=r'''
SetRampage:
    ldr r1,=0x0203f080
    str r0,[r1]
    bx lr
WriteSram:
    sub r0,r0,#0x0e000000
    ldr r3,=0x0203f080
    ldr r3,[r3]
    cmp r3,#0x20
    add r0,r0,#0x02000000
    addeq r0,r0,#0x00010000
    addne r0,r0,#0x00020000
test_write:
    ldr r3,[r1],#4
    str r3,[r0],#4
    subs r2,r2,#4
    bne test_write
    bx lr
ReadSram:
    sub r0,r0,#0x0e000000
    ldr r3,=0x0203f080
    ldr r3,[r3]
    cmp r3,#0x20
    add r0,r0,#0x02000000
    addeq r0,r0,#0x00010000
    addne r0,r0,#0x00020000
test_read:
    ldr r3,[r0],#4
    str r3,[r1],#4
    subs r2,r2,#4
    bne test_read
    bx lr
    .ltorg
'''

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--core',type=Path,required=True);a=p.parse_args()
    work=ROOT/'work/addon-menu-test';work.mkdir(parents=True,exist_ok=True)
    source=(ROOT/'kernels/omega/source/gba_rts_patch.s').read_bytes().decode('latin1').replace('\r\n','\n')
    marker=(ROOT/'common/slot_return_mark.inc').read_text().replace('ldr r0,=0x0e00ff00','ldr r0,=0x0202ff00').replace('add r0,r0,#8','ldr r0,=0x0e00ff08')
    source=source.replace('.include "slot_return_mark.inc"',marker)
    start=source.index('SetRampage:\n');end=source.index('backup_LCD:\n',start)
    source=source[:start]+SHIMS+'\n'+source[end:]
    asm=work/'addon.s';asm.write_bytes(source.encode('latin1'))
    sdk=Path('C:/devkitPro');gcc=sdk/'devkitARM/bin/arm-none-eabi-gcc.exe';elf=work/'addon-menu.elf';rom=work/'addon-menu.gba'
    command=[str(gcc),'-Os','-mcpu=arm7tdmi','-marm','-mthumb-interwork','-specs=gba.specs','-I'+str(ROOT/'common'),'-I'+str(sdk/'libgba/include'),'-Wa,-I'+str(ROOT/'common'),str(ROOT/'tests/addon_menu_harness.c'),str(asm),'-L'+str(sdk/'libgba/lib'),'-lgba','-o',str(elf)]
    subprocess.run(command,check=True)
    subprocess.run([str(sdk/'devkitARM/bin/arm-none-eabi-objcopy.exe'),'-O','binary',str(elf),str(rom)],check=True)
    subprocess.run([str(sdk/'tools/bin/gbafix.exe'),str(rom),'-tSLOT MENU TEST'],check=True,stdout=subprocess.DEVNULL)
    e=Emulator(a.core);e.load(rom);e.run(180)
    def result(i):return C.c_uint.from_address(e.ewram[0]+0x3f000+i*4).value
    def command(n):C.c_uint.from_address(e.ewram[0]+0x3f040).value=n;e.run(3)
    def press(key):e.keys={key};e.run(2);e.keys=set();e.run(3)
    def open_menu():e.keys={2,3};e.run(3);e.keys=set();e.run(3);assert result(2)==1
    def shot(name):e.frame.save(work/name)
    assert result(0)==0x534d3131 and result(5)==0,(result(0),result(4),result(5))
    checks=result(4);open_menu();shot('menu-dark.png')
    font=(ROOT/'assets/slot/ds-font.bin').read_bytes()
    def text_mask(text,x,y):
        nonlocal checks
        image=e.frame.convert('RGB')
        for i,char in enumerate(text):
            for row in range(12):
                for col in range(6):
                    ink=bool(font[ord(char)*12+row]&(128>>col))
                    pixel=image.getpixel((x+i*6+col,y+row))
                    assert (min(pixel)>240)==ink,(char,row,col,pixel,ink)
                    checks+=1
    text_mask('Resume',31,52)
    # Pixels outside the two changed rows must not be touched on a move.
    # Sentinels catch full-screen/panel clears even if final screenshots match.
    assert e.vram,'mGBA did not expose VRAM'
    sentinels=[(10,10),(220,25),(210,90)]
    for x,y in sentinels:C.c_ushort.from_address(e.vram[0]+2*(y*240+x)).value=0x7c1f
    press(5);press(8);assert result(2)==1;checks+=1  # disabled Save state
    for x,y in sentinels:
        assert C.c_ushort.from_address(e.vram[0]+2*(y*240+x)).value==0x7c1f,(x,y,'unrelated menu pixels repainted')
        checks+=1
    press(5);press(8);assert result(2)==1;checks+=1  # disabled Load state
    press(0);e.run(20);assert result(2)==0 and result(1)==result(3) and result(6)==0x40,tuple(result(i) for i in range(9));checks+=3
    command(2);open_menu();shot('menu-light.png');text_mask('Resume',31,52)
    press(8);e.run(20);assert result(2)==0 and result(1)==result(3);checks+=2
    command(1);command(3);open_menu();press(5);press(5);press(5);press(8);shot('menu-cheats.png')
    assert result(2)==1;press(0);e.run(20);assert result(7)==1 and result(1)==result(3);checks+=3
    report={'status':'pass','checks':checks,'failed':0,'scope':'Actual addon menu/return-marker assembly in mGBA with emulated SRAM banks: native glyph pixels, dirty-row-only navigation with unrelated VRAM sentinels, selection, disabled states, resume VRAM/blend restoration, cheat toggle, warm cookie and cold-start rejection. Real cartridge mapper/reset/SD timing and save/load-state IO still require hardware.'}
    (ROOT/'validation/addon-menu-validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
