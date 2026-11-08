"""Regenerate the compact, one-third-scale assets used by both kernels."""
from pathlib import Path
import sys,re
from PIL import Image,ImageDraw,ImageFont
sys.path.insert(0,str(Path(__file__).parent))
from convert_labels import ASSETS,ROOT,SPECS,Shell,compose,DEFAULTS

def pixels(im):
    vals=[]
    for r,g,b,a in im.convert('RGBA').getdata():
        v=0x8000 if a<128 else (r>>3)|((g>>3)<<5)|((b>>3)<<10)
        vals.append(v)
    return vals

def array(name,im):
    vals=pixels(im)
    rows=['static const unsigned short '+name+'['+str(len(vals))+'] = {']
    rows+=['    '+','.join(f'0x{v:04x}' for v in vals[i:i+16])+',' for i in range(0,len(vals),16)]
    rows+=['};']
    return '\n'.join(rows)

out=['/* Generated from Slot artwork by Markus. GPL-3.0. Run tools/generate_assets.py. */','#ifndef SLOT_ASSETS_H','#define SLOT_ASSETS_H']
background=Image.open(ASSETS/'background_default.png').convert('RGB').resize((240,160),Image.Resampling.LANCZOS)
out.append(array('slot_background',background))
for tone in ('dark','light'):
    for layer in ('back','front'):
        im=Image.open(ASSETS/f'slot_{layer}_{tone}.png').resize((240,19),Image.Resampling.LANCZOS)
        if tone=='dark':
            # Lift the dark plastic by roughly one GBA colour step. Preserve
            # the downscaled metal screw exactly, including its dark rim.
            im=im.convert('RGBA');px=im.load()
            for y in range(im.height):
                for x in range(im.width):
                    if layer=='back' and 115<=x<=125 and y<=10:continue
                    r,g,b,a=px[x,y];level=max(r,g,b)
                    if a and 8<=level<112:
                        lift=round(8*(1-level/112))
                        px[x,y]=(r+lift,g+lift,b+lift,a)
        out.append(array(f'slot_{layer}_{tone}',im))
    # Slot's untextured slot_chrome.rs bands, at exactly one-third geometry.
    colours=('#242429','#1a1a1d','#050508','#4d4d57') if tone=='dark' else ('#e0e0dc','#c5c5c2','#424247','#aaaaad')
    housing,recess,opening,edge=colours
    back=Image.new('RGBA',(240,19));d=ImageDraw.Draw(back)
    d.rectangle((75,0,165,14),fill=recess);d.rectangle((78,0,162,0),fill=edge)
    d.rectangle((78,2,162,5),fill=opening)
    front=Image.new('RGBA',(240,19));d=ImageDraw.Draw(front)
    d.rectangle((0,0,74,18),fill=housing);d.rectangle((166,0,239,18),fill=housing)
    d.rectangle((0,15,239,18),fill=housing)
    d.rectangle((75,5,82,15),fill=housing);d.rectangle((158,5,165,15),fill=housing)
    for x in range(83,158):
        depth=9*(1-(abs(x-120)/38)**4);y=round(5+depth)
        d.line((x,y,x,15),fill=housing);d.point((x,y),fill=edge)
    out.append(array(f'slot_back_flat_{tone}',back));out.append(array(f'slot_front_flat_{tone}',front))
for platform in SPECS:
    colour,finish=DEFAULTS[platform]
    face,_=compose(platform,'',None,Shell('auto',colour,finish))
    out.append(array('slot_fallback_'+platform.lower(),face))
    face,_=compose(platform,'',None,Shell('auto',colour,finish),skeuomorphic=False)
    out.append(array('slot_flat_'+platform.lower(),face))
out.append(array('slot_board',Image.open(ASSETS/'board.png').resize((120,71),Image.Resampling.LANCZOS)))
# Preserve DS Style's original 6x12 bitmap glyphs verbatim.
font_source=(ROOT/'kernels/omega/source/asc126_new.h').read_bytes().decode('latin1')
font_data=[int(x,16) for x in re.findall(r'0x([0-9a-fA-F]{2})',font_source.split('= {',1)[1])]
assert len(font_data)==1536
out.append('static const unsigned char slot_font[128][12] = {')
out += ['    {'+','.join(str(v) for v in font_data[c*12:(c+1)*12])+'},' for c in range(128)]
out.append('};')
splash=Image.open(ASSETS/'bootlogo.bmp').convert('RGB')
# Slot supplies a wordmark or full-screen bitmap. Fit, preserve aspect ratio,
# and always place it on black at GBA resolution.
if splash.width==720 and splash.height==480:
    splash=splash.resize((240,160),Image.Resampling.LANCZOS)
else:
    from PIL import ImageOps
    splash=splash.resize((max(1,round(splash.width/3)),max(1,round(splash.height/3))),Image.Resampling.LANCZOS)
    bg=Image.new('RGB',(240,160),'black');bg.paste(splash,((240-splash.width)//2,(160-splash.height)//2));splash=bg
splash.putdata([(0,0,0) if max(rgb)<32 else rgb for rgb in splash.getdata()])
out.append(array('slot_splash',splash))
out+=['#endif']
(ROOT/'common/slot_assets.h').write_text('\n'.join(out)+'\n',encoding='utf-8')
print('Generated',ROOT/'common/slot_assets.h')
