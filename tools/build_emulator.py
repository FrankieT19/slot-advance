"""Rebuild the embedded Jagoomba v0.5 with slot. advance controls and return hook.

Pass a checkout of EvilJagaGenius/jagoombacolor at UPSTREAM_COMMIT. The normal
kernel build uses the supplied generated headers and needs no network access.
"""
from pathlib import Path
import argparse,concurrent.futures,hashlib,json,shutil,subprocess
ROOT=Path(__file__).resolve().parents[1]
UPSTREAM_COMMIT='2a77825e04b7fcef968c2e5f97cb238296a1ebb5'

def integrate(src):
    def edit(name,before,after):
        p=src/name;s=p.read_text(encoding='latin1')
        if s.count(before)!=1:raise RuntimeError('Unrecognised upstream hook: '+name)
        p.write_text(s.replace(before,after),encoding='latin1',newline='\n')
    edit('config.h','#define EZFLASH_DE_BUILD 0','#ifndef EZFLASH_DE_BUILD\n#define EZFLASH_DE_BUILD 0\n#endif')
    declarations='''
void Slot_JagoombaInit(void);
unsigned Slot_JagoombaInput(unsigned held);
unsigned Slot_JagoombaTakeExit(void);
void Slot_JagoombaResume(void);
'''
    edit('includes.h','#include "config.h"','#include "config.h"\n'+declarations)
    edit('main.c','\tewram_canary_2 = 0xDEADBEEF;','\tewram_canary_2 = 0xDEADBEEF;\n\tSlot_JagoombaInit();')
    edit('timeout.s','''\tadr lr,line0x\t\t@return here after doing L/R + SEL/START

\ttst r1,#0x300\t\t@if L or R was pressed
\ttstne r0,#0x100
\ttstne r0,#0x200\t\t@and both L+R are held..
\tldrne r1,=ui
\tbxne r1\t\t\t@do menu''','''\tstmfd sp!,{r0-r1}
\tbl_long Slot_JagoombaInput
\tmov r2,r0
\tldmfd sp!,{r0-r1}
\tadr lr,line0x
\tcmp r2,#0
\tldrne r2,=ui
\tbxne r2''')
    # Count only the VBlank being dispatched, after serial IRQ priority. If
    # both are pending, counting at IRQ entry would count the same VBlank twice.
    edit('lcd.s','\t\tands r0,r1,#0x01\n\t\tldrne r12,vblankfptr','''\t\tands r0,r1,#0x01
\tldrne r3,=Slot_JagoombaVBlanks
\tldrne r12,[r3]
\taddne r12,r12,#1
\tstrne r12,[r3]
\t\tldrne r12,vblankfptr''')
    # Hold-exit pauses at the same frame boundary as the menu and uses the
    # existing SRAM backup. A failed backup leaves the emulator menu usable.
    edit('ui.c','\tselected=0;\n\tmain_ui_selection = selected;','''\tif(Slot_JagoombaTakeExit()) {
\t\tif(backup_gb_sram(1))exit_();
\t}
\tselected=0;
\tmain_ui_selection = selected;''')
    edit('ui.c','\tmain_ui_selection = -1;','\tmain_ui_selection = -1;\n\tSlot_JagoombaResume();')
    edit('ui.c','''\tfadetowhite();
\tREG_DISPCNT=FORCE_BLANK;\t\t//screen OFF
\tREG_BG0HOFS=0;
\tREG_BG0VOFS=0;
\tREG_BLDMOD=0;\t\t\t\t\t//no blending
\tdoReset();''','''\t/* Manual Exit and Select-hold both finish normal GB save handling. */
\tif(!backup_gb_sram(1))return;
\tdoReset();''')
    edit('io.s','doReset:\n','''doReset:
#ifdef SLOT_EMULATOR_TEST
\tldr pc,=Slot_JagoombaTestReset
#else
\tldr pc,=Slot_EmulatorReset
#endif
''')

def build(model,source,sdk,out,test=False):
    folder=out/model;folder.mkdir(parents=True,exist_ok=True)
    src=folder/'src';shutil.copytree(source/'src',src,dirs_exist_ok=True);integrate(src)
    binary=sdk/'devkitARM/bin';gcc=binary/'arm-none-eabi-gcc.exe';copy=binary/'arm-none-eabi-objcopy.exe'
    def run(args,name):
        p=subprocess.run([str(x) for x in args],cwd=src,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
        (folder/(name+'.log')).write_text(p.stdout,encoding='utf-8')
        if p.returncode:raise RuntimeError(name+': '+p.stdout[-6000:])
    flags=['-Os','-g','-mcpu=arm7tdmi','-mtune=arm7tdmi','-mthumb','-mthumb-interwork','-fomit-frame-pointer','-ffast-math','-fcommon','-std=gnu99','-Wno-error=implicit-function-declaration','-Wno-error=incompatible-pointer-types','-DEZFLASH_OMEGA_BUILD','-DEZFLASH_DE_BUILD='+str(int(model=='omega-de')),'-I'+str(src),'-I'+str(ROOT/'common'),'-I'+str(sdk/'libgba/include')]
    if test:flags+=['-DSLOT_EMULATOR_TEST']
    files=sorted(src.glob('*.c'))+[src/'all.s',src/'gba_crt0_my.s',ROOT/'common/slot_jagoomba.c',ROOT/'common/slot_emulator_reset.s']
    def compile(f):
        obj=folder/(f.stem+'.o');extra=['-x','assembler-with-cpp'] if f.suffix=='.s' else []
        if f.name=='slot_jagoomba.c':extra+=['-marm']
        run([gcc,*flags,*extra,'-c',f,'-o',obj],f.stem);return obj
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:objects=list(pool.map(compile,files))
    for name,symbol in [('font.lz77','font_lz77'),('fontpal.bin','fontpal_bin')]:
        obj=folder/(symbol+'.o');asm=folder/(symbol+'.s')
        asm.write_text('.section .rodata\n.align 2\n.global '+symbol+'\n'+symbol+':\n.incbin "'+(src/name).as_posix()+'"\n')
        run([gcc,'-c',asm,'-o',obj],symbol);objects.append(obj)
    crt=objects.pop(objects.index(folder/'gba_crt0_my.o'))
    def crtfile(name):return subprocess.check_output([str(gcc),'-mthumb','-mcpu=arm7tdmi','-print-file-name='+name],text=True).strip()
    elf=folder/'jagoomba.elf';rom=folder/'jagoomba.gba'
    run([gcc,'-g','-mcpu=arm7tdmi','-mthumb','-mthumb-interwork','-nostartfiles','-specs=sync-none.specs','-T',src/'gba_cart_my.ld','-Wl,-Map,'+str(folder/'jagoomba.map'),crt,crtfile('crti.o'),*objects,'-L'+str(sdk/'libgba/lib'),'-lgba','-lc','-lgcc',crtfile('crtn.o'),'-o',elf],'link')
    run([copy,'-O','binary',elf,rom],'objcopy');run([sdk/'tools/bin/gbafix.exe',rom,'-tGOOMBA COLOR','-cGMBC'],'gbafix')
    symbols=subprocess.check_output([str(binary/'arm-none-eabi-nm.exe'),'-n',str(elf)],text=True)
    (folder/'symbols.txt').write_text(symbols)
    addresses={l.split()[-1]:int(l.split()[0],16) for l in symbols.splitlines() if len(l.split())==3}
    # Keep explicit headroom in the emulator's tightly packed IWRAM layout.
    headroom=addresses['__sp_usr']-addresses['__bss_end__']
    if headroom<256:raise RuntimeError('Emulator IWRAM stack headroom below 256 bytes')
    data=rom.read_bytes()
    return {'target':model,'file':str(rom),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'iwram_headroom_bytes':headroom,'upstream_commit':UPSTREAM_COMMIT,'test_shims':test}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,required=True)
    p.add_argument('--target',choices=['all','omega','omega-de'],default='all');p.add_argument('--devkitpro',type=Path,default=Path('C:/devkitPro'))
    p.add_argument('--build-root',type=Path,default=ROOT/'work/emulator-build');p.add_argument('--test',action='store_true');p.add_argument('--no-install',action='store_true');a=p.parse_args()
    source=a.source.resolve();commit=subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip()
    if commit!=UPSTREAM_COMMIT:raise SystemExit('Use upstream Jagoomba v0.5 at '+UPSTREAM_COMMIT)
    if subprocess.run(['git','-C',str(source),'diff','--quiet','HEAD','--','src']).returncode:
        raise SystemExit('Use an unmodified upstream src directory; integration is applied to a build-directory copy.')
    models=['omega','omega-de'] if a.target=='all' else [a.target]
    reports=[]
    for model in models:
        report=build(model,source,a.devkitpro.resolve(),a.build_root.resolve(),a.test);reports.append(report)
        if not a.test and not a.no_install:
            data=Path(report['file']).read_bytes()
            header='/* Generated by tools/build_emulator.py from pinned Jagoomba v0.5. */\nconst long int goomba_gba_size = '+str(len(data))+';\nconst unsigned char goomba_gba['+str(len(data))+'] = {\n'
            header+=''.join('    '+', '.join(f'0x{x:02X}' for x in data[i:i+16])+',\n' for i in range(0,len(data),16))+'};\n'
            (ROOT/'kernels'/model/'source/goomba.h').write_text(header)
    target=a.build_root.resolve()/'build.json';target.write_text(json.dumps(reports,indent=2)+'\n')
    if not a.test and not a.no_install:
        clean=[{k:v for k,v in r.items() if k!='file'} for r in reports]
        (ROOT/'validation/emulator-build.json').write_text(json.dumps(clean,indent=2)+'\n')
    print(json.dumps(reports,indent=2))
if __name__=='__main__':main()
