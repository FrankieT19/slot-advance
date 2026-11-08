"""Convert Slot Cart Studio labels/shells to slot. advance's RGB555 BMPs.

No network calls. Accepts a Slot card, Labels directory, or Cart Studio ZIP.
Copyright 2026. GPL-3.0-or-later; artwork and rendering model from Slot.
"""
from __future__ import annotations
import argparse
import sys
import textwrap
from functools import lru_cache
from dataclasses import dataclass
import io
import json
import os
from pathlib import Path, PurePosixPath
import re
import struct
import unicodedata
import zipfile
from PIL import Image, ImageDraw, ImageFont, ImageOps
from simple_cartridge import shell_face

ROOT = Path(getattr(sys,'_MEIPASS',Path(__file__).resolve().parents[1]))
ASSETS = ROOT / 'assets' / 'slot'
PLATFORMS = ('GBA', 'GB', 'GBC')
TRANSPARENT = 0x7c1f  # Standard BMP magenta, identical after RGB555 channel swap.
SPECS = {
    'GBA': ((360, 212), (48, 50, 264, 136), (120, 71)),
    'GB': ((312, 337), (44, 95, 223, 199), (104, 112)),
    'GBC': ((312, 337), (45, 110, 223, 199), (104, 112)),
}
DEFAULTS = {'GBA': ('35353a', 'solid'), 'GB': ('9a978f', 'solid'), 'GBC': ('7c7a8a', 'clear')}
# Slot shell.rs: longer code overrides are tested before region-independent prefixes.
GBA_CODES = {'AXV': ('c2332e', 'clear'), 'AXP': ('2f5cc0', 'clear'), 'BPE': ('399c3c', 'clear'),
             'BPR': ('d85224', 'solid'), 'BPG': ('63b044', 'solid'), 'U3I': ('d9dbd8', 'clear'),
             'U32': ('d9dbd8', 'clear'), 'U33': ('d9dbd8', 'clear'), 'V49': ('a9513b', 'solid'),
             'KYGE': ('2f8f4e', 'solid'), 'KYGP': ('2f8f4e', 'solid'),
             'RZW': ('5f6264', 'clear'), 'RZWJ': ('eceee8', 'solid')}
GB_CODES = {'AAU': ('b38b3a', 'solid'), 'AAUJ': ('333031', 'solid'), 'AAX': ('a9aaa7', 'solid'),
            'AAXJ': ('333031', 'solid'), 'BYT': ('86b9bf', 'glitter'), 'BXT': ('86b9bf', 'glitter'),
            'KTN': ('ec94b4', 'clear'), 'KKK': ('ec94b4', 'clear'), 'KCE': ('1f9fb6', 'clear'),
            'VCA': ('f0a95e', 'clear'), 'VPHJ': ('e2b413', 'solid'), 'BMG': ('333031', 'solid')}

@dataclass(frozen=True)
class Shell:
    outline: str
    colour: str
    finish: str


def key(s: str) -> str:
    return unicodedata.normalize('NFC', s)


def parse_ini(data: bytes) -> dict[str, str]:
    text = data.decode('utf-8-sig')
    result = {}
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith(('#', ';', '[')) or '=' not in line:
            continue
        name, value = line.split('=', 1)
        result[key(name.strip())] = value.strip()
    return result


def parse_shell(value: str) -> Shell | None:
    words = value.split()
    if (len(words) == 3 and words[0] in ('auto', 'notched', 'rounded')
            and re.fullmatch('[0-9a-fA-F]{6}', words[1]) and words[2] in ('solid', 'clear', 'glitter')):
        return Shell(words[0], words[1].lower(), words[2])
    return None


def layered_shells(system: bytes, labels: bytes) -> dict[str, Shell]:
    out = {k: s for k, v in parse_ini(system).items() if (s := parse_shell(v))}
    for k, v in parse_ini(labels).items():
        if v == 'auto':
            out.pop(k, None)
        elif (s := parse_shell(v)):
            out[k] = s
    return out


class Input:
    """Read a directory or ZIP without ever extracting archive paths."""
    def __init__(self, path: Path):
        self.path = path.resolve()
        self.archive = zipfile.ZipFile(path) if path.is_file() else None
        self.files: dict[str, object] = {}
        entries = self.archive.infolist() if self.archive else sorted(path.rglob('*'))
        for entry in entries:
            if self.archive:
                if entry.is_dir():
                    continue
                name = entry.filename.replace('\\', '/')
                if entry.file_size > 64 * 1024 * 1024:
                    raise ValueError(f'Archive entry exceeds 64 MiB: {name}')
            else:
                if not entry.is_file() or entry.is_symlink():
                    continue
                name = entry.relative_to(path).as_posix()
            if any(p == '..' for p in PurePosixPath(name).parts) or name.startswith('/') or ':' in name:
                raise ValueError(f'Unsafe input path: {name}')
            n = key(name).casefold()
            if n in self.files:
                raise ValueError(f'Duplicate name after FAT/Unicode normalisation: {name}')
            self.files[n] = entry

    def read(self, name: str) -> bytes:
        entry = self.files[key(name).casefold()]
        return self.archive.read(entry) if self.archive else entry.read_bytes()

    def optional(self, *names: str) -> bytes:
        for name in names:
            if key(name).casefold() in self.files:
                return self.read(name)
        return b''

    def close(self):
        if self.archive:
            self.archive.close()

    def labels(self) -> dict[tuple[str, str], str]:
        out = {}
        for name in self.files:
            parts = PurePosixPath(name).parts
            if 'system' in parts[:-2]:
                continue  # Never re-import this application's generated BMPs.
            if len(parts) < 2 or Path(name).suffix.lower() not in ('.png', '.jpg', '.jpeg', '.bmp', '.webp'):
                continue
            platform = parts[-2].upper()
            if platform not in PLATFORMS:
                continue
            # Avoid treating arbitrary game/save files as art on a full card.
            if len(parts) > 2 and parts[-3] not in ('labels',):
                continue
            entry = self.files[name]
            raw = entry.filename.replace('\\', '/') if self.archive else entry.relative_to(self.path).as_posix()
            stem = key(PurePosixPath(raw).stem)
            k = (platform, stem.casefold())
            if k in out:
                raise ValueError(f'Multiple labels for {platform}/{stem}')
            out[k] = raw
        return out


def lookup(code: str, table: dict) -> tuple | None:
    if code in table:
        return table[code]
    return table.get(code[:3])


def automatic_shell(platform: str, rom: Path | None) -> Shell:
    colour, finish = DEFAULTS[platform]
    outline = 'auto'
    if rom:
        with rom.open('rb') as stream:
            header = stream.read(0x150)
        if platform == 'GBA' and len(header) >= 0xb0:
            code = header[0xac:0xb0].decode('ascii', 'replace')
            choice = lookup(code, GBA_CODES)
            if choice:
                colour, finish = choice
            elif code.startswith('M'):
                colour, finish = 'c6c6c9', 'solid'
        elif platform in ('GB', 'GBC') and len(header) >= 0x14b:
            flag = header[0x143]
            outline = 'rounded' if flag == 0xc0 else 'notched'
            colour, finish = ('7c7a8a', 'clear') if flag == 0xc0 else (('333031', 'solid') if flag == 0x80 else ('9a978f', 'solid'))
            code = header[0x13f:0x143].decode('ascii', 'replace')
            if not re.fullmatch('[A-Z0-9]{4}',code): code='' 
            choice = lookup(code, GB_CODES)
            if choice:
                colour, finish = choice
            elif header[0x14a] != 0:
                title = header[0x134:0x13f].split(b'\0', 1)[0].decode('ascii', 'replace')
                for prefix, ink in [('POKEMON RED', 'c0282c'), ('POKEMON BLU', '2b3a88'), ('POKEMON YEL', 'e9a826')]:
                    if title == prefix:
                        colour, finish = ink, 'solid'
    return Shell(outline, colour, finish)


def shape_for(platform: str, shell: Shell) -> str:
    if platform == 'GBA':
        return platform
    return {'notched': 'GB', 'rounded': 'GBC'}.get(shell.outline, platform)


def overlay(base: int, colour: int) -> int:
    if base >= 128:
        return min(255, colour + ((255 - colour) * (base - 128) + 63) // 127)
    return max(0, colour - (colour * (128 - base) + 64) // 128)


@lru_cache(maxsize=128)
def tinted(name: str, colour: str, panel=None, clear=False, glitter=False) -> Image.Image:
    im = Image.open(ASSETS / name).convert('RGBA')
    rgb = bytes.fromhex(colour)
    src = list(im.getdata())
    out = []
    for i, (r, g, b, a) in enumerate(src):
        x, y = i % im.width, i // im.width
        on_label = panel and panel[0] <= x < panel[0]+panel[2] and panel[1] <= y < panel[1]+panel[3]
        if not on_label:
            r, g, b = (overlay(c, ink) for c, ink in zip((r, g, b), rgb))
        if on_label:
            a = 255
        elif clear:
            a = (a * 204 + 127) // 255
        if glitter and a:
            h = ((x * 0x9e3779b9) ^ (y * 0x85ebca6b)) & 0xffffffff
            h ^= h >> 15
            h = h * 0x2c1b3c6d & 0xffffffff
            h ^= h >> 12
            if h % 100 == 0:
                r, g, b = ((c * 155 + 255 * 100) // 255 for c in (r, g, b))
                a = 255
        out.append((r, g, b, a))
    im.putdata(out)
    return im


def generated_label(title: str, size: tuple[int, int]) -> Image.Image:
    # Final-resolution pixels: this image must never pass through a downscale.
    w,h=size
    im=Image.new('RGBA',(w,h),'#e9e4d8');px=im.load()
    data=(ASSETS/'ds-font.bin').read_bytes()
    clean=re.sub(r'\s*\([^)]*\)|\s*\[[^]]*\]', '', title).strip() or title
    lines=textwrap.wrap(clean,width=max(1,(w-8)//6),break_long_words=True)[:max(1,(h-4)//12)]
    y=(h-len(lines)*12)//2
    for line in lines:
        x=(w-len(line)*6)//2
        for char in line:
            code=ord(char) if 32<=ord(char)<127 else ord('?')
            for row in range(12):
                bits=data[code*12+row]
                for col in range(8):
                    xx,yy=x+col,y+row
                    if bits & (128>>col) and 0<=xx<w and 0<=yy<h:px[xx,yy]=(40,40,47,255)
            x+=6
        y+=12
    return im


def label_mask(size):
    # One missing pixel at each corner, with the rest of every edge intact.
    mask=Image.new('L',size,255);w,h=size
    for x,y in ((0,0),(w-1,0),(0,h-1),(w-1,h-1)):mask.putpixel((x,y),0)
    return mask


def compose(platform: str, title: str, label: Image.Image | None, shell: Shell, fit='cover', *, skeuomorphic=True) -> tuple[Image.Image, Image.Image]:
    shape = shape_for(platform, shell)
    native, panel, small = SPECS[shape]
    clear = shell.finish != 'solid'
    # Colour the plastic beneath the entire label too. Slot's rectangular grey
    # well was exposed when our sticker mask rounded its corners.
    if skeuomorphic:
        face = tinted(f'{shape.lower()}_{"clear" if clear and shape != "GB" else "neutral"}.png',
                      shell.colour, None, clear, shell.finish == 'glitter').copy()
    else:
        face=shell_face(ASSETS,shape,shell.colour,shell.finish,panel).copy()
    if clear and skeuomorphic:
        # Slot's three composited layers, flattened before GBA quantisation.
        back = tinted(f'{shape.lower()}_interior.png', shell.colour).copy()
        board = Image.open(ASSETS / ('board.png' if shape == 'GBA' else 'gb_board.png')).convert('RGBA')
        back.alpha_composite(board)
        back.alpha_composite(face)
        face = back
    face = face.resize(small, Image.Resampling.LANCZOS)
    final_size=(round(panel[2]/3),round(panel[3]/3))
    if label is None or str(label.info.get('slot-label','')).startswith('text '):
        label = generated_label(title, final_size)
    else:
        label = ImageOps.exif_transpose(label).convert('RGBA')
        # Discard about one final GBA pixel of scan edge before fitting.
        border_x=max(1,round(label.width/(panel[2]/3)))
        border_y=max(1,round(label.height/(panel[3]/3)))
        if label.width>2*border_x and label.height>2*border_y:
            label=label.crop((border_x,border_y,label.width-border_x,label.height-border_y))
        if fit == 'contain':
            contained = ImageOps.contain(label, final_size, Image.Resampling.LANCZOS)
            label = Image.new('RGBA', final_size, '#e9e4d8')
            label.alpha_composite(contained, ((final_size[0]-contained.width)//2, (final_size[1]-contained.height)//2))
        else:
            label = ImageOps.fit(label, final_size, method=Image.Resampling.LANCZOS, centering=(.5, .5))
    # Transparent logos get a paper backing, not transparent holes in the cartridge.
    paper = Image.new('RGBA', final_size, '#e9e4d8')
    paper.alpha_composite(label)
    paper.putalpha(label_mask(final_size))
    face.alpha_composite(paper, (round(panel[0]/3),round(panel[1]/3)))
    return face, paper


def round_existing_cart(image,platform,outline='auto'):
    shape=shape_for(platform,Shell(outline,'808080','solid'))
    _,(lx,ly,lw,lh),size=SPECS[shape]
    if image.size!=size:raise ValueError('Existing cartridge BMP has incompatible dimensions; restore its original PNG label.')
    image=image.convert('RGBA');x,y=round(lx/3),round(ly/3);w,h=round(lw/3),round(lh/3)
    # Decode the BMP colour key without altering any opaque source pixels.
    pixels=list(image.getdata());image.putdata([(r,g,b,0 if (r,g,b)==(255,0,255) else a) for r,g,b,a in pixels])
    paper=image.crop((x,y,x+w,y+h));paper.putalpha(label_mask((w,h)))
    fill=image.getpixel((max(0,x-1),min(image.height-1,y+4)))
    image.paste(fill,(x,y,x+w,y+h));image.alpha_composite(paper,(x,y))
    return image,paper


def bmp555(im: Image.Image, dither=False) -> bytes:
    im = im.convert('RGBA')
    w, h = im.size
    stride = (w * 2 + 3) & ~3
    data = bytearray(stride*h)
    bayer = ((0,8,2,10),(12,4,14,6),(3,11,1,9),(15,7,13,5))
    pixels = im.load()
    for y in range(h):
        for x in range(w):
            r,g,b,a = pixels[x,y]
            if a < 128:
                v = TRANSPARENT
            else:
                bias = (bayer[y%4][x%4]-7.5)/2 if dither else 0
                r,g,b = (max(0,min(31,int((c+bias)*31/255+.5))) for c in (r,g,b))
                v = b | g<<5 | r<<10
                if v == TRANSPARENT:
                    v ^= 1
            struct.pack_into('<H',data,(h-1-y)*stride+x*2,v)
    header = struct.pack('<2sIHHI', b'BM', 54+len(data),0,0,54)
    dib = struct.pack('<IiiHHIIiiII',40,w,h,1,16,0,len(data),2835,2835,0,0)
    return header+dib+data


def atomic(path: Path, data: bytes):
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_name(path.name+'.tmp')
    tmp.write_bytes(data)
    os.replace(tmp, path)


def validate_name(stem: str, suffix='.gba'):
    if not stem or stem in ('.','..') or any(c in stem for c in '/\\<>:"|?*') or stem.endswith((' ','.')):
        raise ValueError(f'Filename cannot be used on FAT: {stem!r}')
    try:
        encoded = (stem+suffix).encode('cp936')
    except UnicodeEncodeError as e:
        raise ValueError(f'Filename is outside the kernel CP936 character set: {stem!r}') from e
    if len(encoded) >= 100:
        raise ValueError(f'Filename exceeds DS Style\'s 99-byte record: {stem+suffix}')


def find_roms(root: Path | None) -> dict[tuple[str,str], Path]:
    result = {}
    if root is None:
        return result
    root = root.resolve()
    for platform in PLATFORMS:
        dirs = [root/'Games'/platform, root/platform, root/'ROMS'/platform]
        directory = next((d for d in dirs if d.is_dir()), None)
        if directory is None:
            continue
        for p in sorted(directory.iterdir()):
            if not p.is_file() or p.name.startswith('.') or p.suffix.lower() not in ({'.gba','.agb'} if platform=='GBA' else {'.'+platform.lower()}):
                continue
            k = (platform, key(p.stem).casefold())
            if k in result:
                raise ValueError(f'Two ROMs share the same artwork stem: {p}')
            validate_name(p.stem, p.suffix)
            result[k] = p
    return result


def convert(source: Path, output: Path, rom_root: Path | None=None, fit='cover', dither=False, previews=False, *, allow_card_output=False, progress=None):
    source, output = source.resolve(), output.resolve()
    # Output can be the SD root but must not replace a source artwork directory.
    if (output == source and not allow_card_output) or source in output.parents:
        raise ValueError('Choose a separate output folder, then copy SYSTEM to the SD card.')
    incoming = Input(source)
    try:
        labels = incoming.labels()
        shells = layered_shells(incoming.optional('Config/cart_shell.txt','Config/cart_shell.ini','System/cart_shell.txt','System/cart_shell.ini'),
                                incoming.optional('Labels/cart_shell.txt','cart_shell.txt','Labels/cart_shell.ini','cart_shell.ini'))
        roms = find_roms(rom_root)
        if rom_root is None and source.is_dir():
            roms = find_roms(source)
        items = sorted(labels.keys() | roms.keys())
        if not items:
            raise ValueError('No labels or ROMs found. Expected Labels/GBA/*.png, GB and GBC, or GBA/*.png in a Labels folder.')
        try: old_manifest=json.loads(incoming.optional('SYSTEM/SLOT/conversion.json') or b'{}')
        except (ValueError,UnicodeError):old_manifest={}
        old_items={(item.get('platform'),key(item.get('stem','')).casefold()):item for item in old_manifest.get('items',[]) if isinstance(item,dict)}
        manifest = {'format':3,'cart_scale':'1/3','bmp':'16-bit BI_RGB RGB555; magenta colour key',
                    'source':source.name,'items':[],'warnings':[]}
        shared_saves = sorted({stem for platform,stem in roms if platform=='GB'} & {stem for platform,stem in roms if platform=='GBC'})
        for stem in shared_saves:
            manifest['warnings'].append(f'GB and GBC share the save stem {stem!r}. DS Style stores both in SYSTEM/SAVER as .esv; use distinct ROM names to keep saves separate.')
        counts = {p:0 for p in PLATFORMS}
        for item_number,(platform, normalized) in enumerate(items,1):
            label_path = labels.get((platform,normalized))
            rom = roms.get((platform,normalized))
            stem = key(rom.stem if rom else PurePosixPath(label_path).stem)
            validate_name(stem, rom.suffix if rom else ('.gba' if platform=='GBA' else '.'+platform.lower()))
            shell = shells.get(stem, automatic_shell(platform,rom))
            if shell.outline == 'auto':
                shell = Shell(automatic_shell(platform,rom).outline, shell.colour, shell.finish)
            label = None
            text_label=False
            if label_path:
                with Image.open(io.BytesIO(incoming.read(label_path))) as src:
                    src.load()
                    label = src.copy()
                    text_label=str(src.info.get('slot-label','')).startswith('text ')
            old=old_items.get((platform,normalized),{})
            existing_name=f'SYSTEM/SLOT/CARTS/{platform}/{stem}.bmp'
            existing=incoming.optional(existing_name) if label is None and not (old and not old.get('scraped_label') and not old.get('recovered_existing_cart')) else b''
            recovered=False
            if existing:
                outline=old.get('shell',{}).get('outline','auto')
                with Image.open(io.BytesIO(existing)) as im:
                    _,small_label=round_existing_cart(im,platform,outline)
                if old.get('shell'):shell=Shell(**old['shell'])
                # Rebuild the plastic, then reuse the surviving label pixels.
                face,_=compose(platform,stem,None,shell)
                shape=shape_for(platform,shell);panel=SPECS[shape][1]
                face.alpha_composite(small_label,(round(panel[0]/3),round(panel[1]/3)))
                recovered=True
            else:
                face, small_label = compose(platform,stem,label,shell,fit)
            flat,_=compose(platform,stem,None if recovered else label,shell,fit,skeuomorphic=False)
            if recovered:flat.alpha_composite(small_label,(round(panel[0]/3),round(panel[1]/3)))
            cart_file = Path('SYSTEM/SLOT/CARTS')/platform/(stem+'.bmp')
            flat_file = Path('SYSTEM/SLOT/CARTS_FLAT')/platform/(stem+'.bmp')
            label_file = Path('SYSTEM/SLOT/LABELS')/platform/(stem+'.bmp')
            atomic(output/cart_file,bmp555(face,dither))
            atomic(output/flat_file,bmp555(flat,dither))
            atomic(output/label_file,bmp555(small_label,dither))
            if previews:
                preview = output/'PREVIEWS'/platform/(stem+'.png')
                preview.parent.mkdir(parents=True,exist_ok=True)
                face.save(preview)
                flat.save(preview.with_name(stem+'-flat.png'))
            counts[platform] += 1
            manifest['items'].append({'platform':platform,'stem':stem,'cart':cart_file.as_posix(),
                'label':label_file.as_posix(),'cart_size':face.size,'label_size':small_label.size,
                'flat_cart':flat_file.as_posix(),'shell':shell.__dict__,'scraped_label':bool(label_path),
                'native_text':not label_path or text_label,'recovered_existing_cart':recovered,'header_read':bool(rom)})
            if progress: progress(item_number,len(items),stem)
        for p,n in counts.items():
            if n>512: manifest['warnings'].append(f'{p}: {n} images converted; the kernel displays at most 512 ROMs per shelf.')
        atomic(output/'SYSTEM/SLOT/conversion.json',json.dumps(manifest,ensure_ascii=False,indent=2).encode('utf-8'))
        return manifest
    finally:
        incoming.close()


def gui():
    from converter_app import run_gui
    run_gui()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,help='Slot card root, Labels folder, or Cart Studio ZIP')
    parser.add_argument('--output',type=Path,help='Separate staging folder for SYSTEM/SLOT')
    parser.add_argument('--roms',type=Path,help='Card root or parent of GBA, GB and GBC folders; reads headers for automatic shell choices')
    parser.add_argument('--fit',choices=['cover','contain'],default='cover')
    parser.add_argument('--dither',action='store_true')
    parser.add_argument('--previews',action='store_true')
    parser.add_argument('--gui',action='store_true')
    args=parser.parse_args()
    if args.gui:
        gui(); return
    if not args.input or not args.output:
        parser.error('--input and --output are required (or use --gui)')
    try:
        manifest=convert(args.input,args.output,args.roms,args.fit,args.dither,args.previews)
    except (ValueError,OSError,zipfile.BadZipFile) as e:
        parser.exit(1,f'Error: {e}\n')
    print(f'Converted {len(manifest["items"])} cartridges into {args.output / "SYSTEM/SLOT"}')
    for warning in manifest['warnings']: print('Warning:',warning)

if __name__=='__main__': main()
