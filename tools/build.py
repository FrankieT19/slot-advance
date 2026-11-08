"""Native devkitARM build; no MSYS shell needed. Python 3.10+, devkitPro gba-dev.
Run: python tools/build.py --target all
"""
from pathlib import Path
import argparse, concurrent.futures, hashlib, json, os, re, shutil, subprocess, sys
ROOT=Path(__file__).resolve().parents[1]

def tool_path(root: Path,name: str):
    p=root/(name+('.exe' if os.name=='nt' else ''))
    if not p.is_file():raise SystemExit(f'Missing devkitPro tool: {p}')
    return str(p)

def run(command,log):
    p=subprocess.run([str(x) for x in command],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    log.write_text(p.stdout, encoding='utf-8')
    if p.returncode:
        print(p.stdout)
        raise RuntimeError(f'Command failed ({p.returncode}); see {log}')
    return p.stdout

def build(target: str,dk: Path,build_root: Path):
    source=ROOT/'kernels'/target
    out=build_root/target;out.mkdir(parents=True,exist_ok=True)
    gcc=tool_path(dk/'devkitARM/bin','arm-none-eabi-gcc')
    include=['-I'+str(source/'source'),'-I'+str(source/'source/ff15'),'-I'+str(source/'images'),'-I'+str(ROOT/'common'),'-I'+str(dk/'libgba/include')]
    flags=['-g','-Wall','-Os','-mcpu=arm7tdmi','-mtune=arm7tdmi','-mthumb','-mthumb-interwork','-fomit-frame-pointer','-ffast-math','-ffunction-sections','-fdata-sections','-Wno-unused-function','-Wno-unused-variable']
    files=sorted((source/'source').glob('*.c'))+sorted((source/'source/ff15').glob('*.c'))+[f for f in sorted((source/'source').glob('*.s')) if f.name!='slot_blit_kernel.s']+[ROOT/'common/slot_blit.s']
    objects=[]
    def compile(f):
        obj=out/(f.stem+'.o')
        run([gcc,*flags,*include,'-Wa,-I'+str(ROOT/'common'),'-c',f,'-o',obj],obj.with_suffix('.log'))
        return obj
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        objects=list(pool.map(compile,files))
    elf=out/'slot.elf';rom=out/'slot.gba';map_file=out/'slot.map'
    run([gcc,'-g','-mthumb','-mthumb-interwork','-mcpu=arm7tdmi','-specs=gba.specs','-Wl,--gc-sections','-Wl,-Map,'+str(map_file),*objects,'-L'+str(dk/'libgba/lib'),'-lmm','-lgba','-o',elf],out/'link.log')
    run([tool_path(dk/'devkitARM/bin','arm-none-eabi-objcopy'),'-O','binary',elf,rom],out/'objcopy.log')
    run([tool_path(dk/'tools/bin','gbafix'),rom,'-tSLOT OMEGA','-cSLOT','-m00'],out/'gbafix.log')
    from boot_entry import install_black_entry
    install_black_entry(rom,dk,out)
    size=rom.stat().st_size
    # Conservative accepted envelope, at or below upstream kernel budgets.
    limit=2*1024*1024  # Both builds stay below the smallest Omega-family budget.
    if size>limit:raise RuntimeError(f'{target}: kernel {size} bytes exceeds {limit}-byte budget')
    report=run([tool_path(dk/'devkitARM/bin','arm-none-eabi-size'),'-A',elf],out/'sections.txt')
    sections=[]
    for line in report.splitlines():
        match=re.match(r'^([.][A-Za-z0-9_]+)\s+(\d+)\s+(\d+)',line)
        if match: sections.append((match[1],int(match[2]),int(match[3])))
    ewram_end=max([0x02000000]+[addr+n for name,n,addr in sections if 0x02000000<=addr<0x02040000])
    iwram_end=max([0x03000000]+[addr+n for name,n,addr in sections if 0x03000000<=addr<0x03008000])
    if ewram_end>0x0203fff0:raise RuntimeError('EWRAM overlaps the warm-return mailbox')
    if iwram_end>0x03007000:raise RuntimeError('IWRAM static data leaves less than the reserved stack budget')
    symbols=run([tool_path(dk/'devkitARM/bin','arm-none-eabi-nm'),'-n',elf],out/'symbols.txt')
    addresses={line.split()[-1]:int(line.split()[0],16) for line in symbols.splitlines() if len(line.split())==3}
    addon_bytes=addresses['RTS_ReplaceIRQ_end']-addresses['RTS_ReplaceIRQ_start']
    if addon_bytes>0x2000:raise RuntimeError('Addon exceeds its reserved 8 KiB ROM patch window')
    ram={'ewram_static_bytes':ewram_end-0x02000000,'ewram_free_bytes':0x02040000-ewram_end,
         'iwram_static_bytes':iwram_end-0x03000000,'iwram_headroom_to_user_stack_bytes':0x03007f00-iwram_end}
    validation=ROOT/'validation';validation.mkdir(exist_ok=True)
    (validation/(target+'-sections.txt')).write_text(target+'/slot.elf :\n'+'\n'.join(report.splitlines()[1:])+'\n', encoding='utf-8')
    release=ROOT/'release'/target
    release.mkdir(parents=True,exist_ok=True)
    name='ezkernel.bin' if target=='omega' else 'ezkernelnew.bin'
    shutil.copy2(rom,release/name)
    data={'target':target,'file':name,'bytes':size,'budget_bytes':limit,'headroom_bytes':limit-size,'sha256':hashlib.sha256(rom.read_bytes()).hexdigest(),'ram':ram,'addon_patch_bytes':addon_bytes,'compiler':subprocess.check_output([gcc,'--version'],text=True).splitlines()[0]}
    (release/'build.json').write_text(json.dumps(data,indent=2)+'\n', encoding='utf-8')
    print(json.dumps(data,indent=2))
    print(report)
    return data

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--target',choices=['omega','omega-de','all'],default='all')
    default='C:/devkitPro' if os.name=='nt' else '/opt/devkitpro'
    p.add_argument('--devkitpro',type=Path,default=Path(default))
    p.add_argument('--build-root',type=Path,default=ROOT/'work/build')
    a=p.parse_args()
    targets=['omega','omega-de'] if a.target=='all' else [a.target]
    data=[build(t,a.devkitpro.resolve(),a.build_root.resolve()) for t in targets]
    (ROOT/'build-report.json').write_text(json.dumps(data,indent=2)+'\n', encoding='utf-8')
if __name__=='__main__':main()
