"""Bundle the SD-card GUI as one Windows executable (no Python installation).
Build dependency: PyInstaller. All resources are embedded in the executable.
"""
from pathlib import Path
import argparse,json,os,subprocess,sys,shutil,hashlib
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bundler-path',type=Path,help='Optional directory containing an isolated PyInstaller installation')
    p.add_argument('--work-root',type=Path,default=ROOT/'work/app-build')
    p.add_argument('--tcl-library',type=Path,help='Optional Tcl script library containing init.tcl (matching this Python runtime)')
    a=p.parse_args();work=a.work_root.resolve();work.mkdir(parents=True,exist_ok=True)
    destination=ROOT/'app';destination.mkdir(exist_ok=True)
    env=os.environ.copy()
    if a.tcl_library:
        # A relative path also works with relocatable Python distributions.
        env['TCL_LIBRARY']=os.path.relpath(a.tcl_library.resolve())
        env['TK_LIBRARY']=os.path.relpath(a.tcl_library.resolve().parent/'tk8.6')
    check=subprocess.run([sys.executable,'-c',"import tkinter; t=tkinter.Tcl(); print(t.eval('info library'))"],env=env,capture_output=True,text=True)
    if check.returncode:raise SystemExit('Tk is incomplete in this Python installation. Provide --tcl-library or use a full Python installation.\n'+check.stderr)
    tcl=Path(check.stdout.strip()).resolve()
    licenses=destination/'licenses';licenses.mkdir(exist_ok=True)
    shutil.copy2(Path(sys.base_prefix)/'LICENSE.txt',licenses/'Python.txt')
    shutil.copy2(ROOT/'kernels/omega/LICENSE',licenses/'DS-Style-Apache-2.0.txt')
    shutil.copy2(tcl.parent/'tk8.6/license.terms',licenses/'Tcl-Tk.txt')
    import importlib.metadata
    pillow=importlib.metadata.distribution('pillow')
    for file in pillow.files:
        if 'licenses' in file.parts:
            target=licenses/'Pillow'/Path(*file.parts[file.parts.index('licenses')+1:])
            target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(pillow.locate_file(file),target)
    pyinstaller=(a.bundler_path.resolve() if a.bundler_path else Path(importlib.metadata.distribution('pyinstaller').locate_file('')))/'PyInstaller'
    bootloader_license=next(pyinstaller.parent.glob('pyinstaller-*.dist-info/licenses/COPYING.txt'))
    shutil.copy2(bootloader_license,licenses/'PyInstaller.txt')
    args=[sys.executable,'-m','PyInstaller','--noconfirm','--clean','--onefile','--windowed','--name','slot-advance-label-converter-v1.0','--distpath',str(destination),'--workpath',str(work/'build'),'--specpath',str(work/'spec'),'--paths',str(ROOT/'tools')]
    for module in ['numpy','scipy','matplotlib','pandas','cv2','pytest','IPython']:
        args+=['--exclude-module',module]
    for name in ['gba_neutral.png','gb_neutral.png','gbc_neutral.png','gba_clear.png','gbc_clear.png','gba_interior.png','gb_interior.png','gbc_interior.png','board.png','gb_board.png','ds-font.bin','gba_simple_mould.png','gb_simple_mould.png','gbc_simple_mould.png']:
        args+=['--add-data',str(ROOT/'assets/slot'/name)+os.pathsep+'assets/slot']
    args+=['--add-data',str(licenses)+os.pathsep+'licenses','--add-data',str(ROOT/'LICENSE')+os.pathsep+'.','--add-data',str(ROOT/'NOTICE.md')+os.pathsep+'.',str(ROOT/'tools/converter_app.py')]
    if a.bundler_path:env['PYTHONPATH']=str(a.bundler_path.resolve())+os.pathsep+env.get('PYTHONPATH','')
    with (work/'build.log').open('w',encoding='utf-8') as log:
        result=subprocess.run(args,env=env,stdout=log,stderr=subprocess.STDOUT)
    if result.returncode:raise SystemExit('Application build failed; see '+str(work/'build.log'))
    exe=destination/'slot-advance-label-converter-v1.0.exe'
    dist=ROOT/'dist';dist.mkdir(exist_ok=True)
    shutil.copy2(exe,dist/exe.name)
    report={'file':exe.name,'bytes':exe.stat().st_size,'sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'platform':'Windows x64','one_file':True,'python_required':False}
    (destination/'build.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
