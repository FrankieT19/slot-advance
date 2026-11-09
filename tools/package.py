"""Build the two minimal installation ZIPs from verified binaries."""
from pathlib import Path
import hashlib,json,zipfile,re,argparse
ROOT=Path(__file__).resolve().parents[1]
DIST=ROOT/'dist'
VERSION=re.search(r'SLOT_DISPLAY_VERSION \"([^\"]+)\"',(ROOT/'common/slot_version.h').read_text()).group(1)

def guide(model):
    return f"""slot. advance {VERSION} - {model}

INSTALL
1. Back up your SD card and saves.
2. Copy the contents of SD_CARD to your SD-card root.
3. Put games in Games/GBA, Games/GB and Games/GBC.
4. Hold R while powering on and follow the kernel updater.
Updating? Keep your existing saves and USERNAME.TXT.

ARTWORK
Scrape labels and choose cartridge colours with slot. Cart Studio:
https://studio.slot-cfw.fyi/
You can leave the SD card connected, even when Cart Studio says to eject it.
Then run Label Converter.exe on your PC, select your SD card and convert.

YOUR NAME
Edit USERNAME.TXT on the card. The first line displays up to 10 characters.
Leave it empty for a blank footer.

CONTROLS
Left/Right: choose game. L/R: change system. A: play. Start: settings.
Open Settings > Help for all controls and GBA Addon shortcuts.
In GBA Addon and GB/GBC games, hold Select for 3 seconds to return and eject.
Start+Select opens the in-game menu.
GBA Addon and GB/GBC fade through black when returning.
GB/GBC run without SGB borders.

Source, credits and licences: https://github.com/FrankieT19/slot-advance
"""

def main():
    global DIST
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir',type=Path,default=DIST)
    DIST=parser.parse_args().output_dir.resolve()
    DIST.mkdir(parents=True,exist_ok=True)
    builds=json.loads((ROOT/'build-report.json').read_text(encoding='utf-8'))
    app=json.loads((ROOT/'app/build.json').read_text(encoding='utf-8'))
    converter=(ROOT/'app'/app['file']).read_bytes()
    assert hashlib.sha256(converter).hexdigest()==app['sha256']
    packages=[]
    for build in builds:
        kernel=(ROOT/'release'/build['target']/build['file']).read_bytes()
        assert len(kernel)<2*1024*1024
        assert hashlib.sha256(kernel).hexdigest()==build['sha256']
        model='Original EZ-FLASH Omega' if build['target']=='omega' else 'Original Omega Definitive Edition (Rev A / Rev B)'
        archive=DIST/('slot-advance-'+VERSION+'-'+build['target']+'.zip')
        with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
            readme=guide(model)
            z.writestr('README.txt',readme)
            z.writestr('Label Converter.exe',converter)
            z.writestr('SD_CARD/'+build['file'],kernel)
            z.writestr('SD_CARD/USERNAME.TXT',b'')
            for folder in ['Games/GBA','Games/GB','Games/GBC','SYSTEM/SAVER','SYSTEM/SLOT']:
                z.writestr('SD_CARD/'+folder+'/',b'')
        with zipfile.ZipFile(archive) as z:
            assert z.testzip() is None
            assert z.read('SD_CARD/USERNAME.TXT')==b''
            assert len([n for n in z.namelist() if not n.endswith('/')])==4
            assert z.read('Label Converter.exe')==converter
            assert z.read('SD_CARD/'+build['file'])==kernel
        packages.append({'model':build['target'],'file':archive.name,'converter_included':True,'empty_username_included':True,'files':4,'zip_integrity':'pass'})
        print(archive.name,archive.stat().st_size)
    (ROOT/'validation/release-validation.json').write_text(json.dumps({'status':'pass','packages':packages},indent=2)+'\n',encoding='utf-8')

if __name__=='__main__':main()
