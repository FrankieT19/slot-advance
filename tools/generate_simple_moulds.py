"""Rasterise Slot's original vector moulds once. Build-only dependency: resvg-py.
The end-user converter uses the resulting PNGs with Pillow alone.
Ported from silhouette.rs at Slot commit 57ea55f9763c01eb4e3ac12e8b0240ec682dba3f.
"""
from pathlib import Path
from io import BytesIO
import json,hashlib
from PIL import Image
import resvg_py
ROOT=Path(__file__).resolve().parents[1]
ASSETS=ROOT/'assets/slot/simple-source'

def raster(name,w,h):
    return Image.open(BytesIO(resvg_py.svg_to_bytes(svg_path=str(ASSETS/name),width=w,height=h,skip_system_fonts=True))).convert('RGBA')

def main():
    for shape,w,h in [('gba',360,212),('gb',312,337),('gbc',312,337)]:
        prefix='cart' if shape=='gba' else shape+'_cart'
        mask=list(raster(prefix+'.svg',w,h).getchannel('A').getdata())
        depth=[255 if a>127 else 0 for a in mask]
        for y in range(h):
            for x in range(w):
                i=y*w+x
                if depth[i]:depth[i]=min(depth[i],(depth[i-w] if y else 0)+1,(depth[i-1] if x else 0)+1)
        for y in range(h-1,-1,-1):
            for x in range(w-1,-1,-1):
                i=y*w+x
                if depth[i]:depth[i]=min(depth[i],(depth[i+w] if y+1<h else 0)+1,(depth[i+1] if x+1<w else 0)+1)
        shadow=[];highlight=[]
        for r,g,b,a in raster(prefix+'_detail.svg',w,h).getdata():
            # tiny-skia exposes premultiplied channels; PNG decodes straight alpha.
            lit=min(a,(r*a*54+g*a*183+b*a*19)//(255*256))
            highlight.append(lit);shadow.append(a-lit)
        lettering=Image.open(ASSETS/f'lettering_{shape}.png').convert('L')
        # Slot decode_mask: centre-sampled bilinear expansion of native lettering.
        sw,sh=lettering.size;src=list(lettering.getdata());lettering=[]
        for y in range(h):
            fy=max(0,(y+.5)*sh/h-.5);y0=int(fy);ty=fy-y0
            for x in range(w):
                fx=max(0,(x+.5)*sw/w-.5);x0=int(fx);tx=fx-x0
                at=lambda xx,yy:src[min(yy,sh-1)*sw+min(xx,sw-1)]
                top=at(x0,y0)*(1-tx)+at(x0+1,y0)*tx
                bottom=at(x0,y0+1)*(1-tx)+at(x0+1,y0+1)*tx
                lettering.append(int(top*(1-ty)+bottom*ty+.5))
        for y in range(h):
            for x in range(w):
                i=y*w+x;here=lettering[i];before=lettering[i-w-1] if x and y else 0
                highlight[i]=min(255,highlight[i]+max(0,here-before))
                shadow[i]=min(255,shadow[i]+max(0,before-here))
        if shape=='gbc':
            at=lambda v,of,to:(v*to+of//2)//of
            y=at(52,259,h)
            for x in range(at(72,240,w),at(168,240,w)):
                shadow[y*w+x]=200;highlight[(y+1)*w+x]=160
        out=Image.new('RGBA',(w,h));out.putdata(list(zip(shadow,highlight,depth,mask)))
        out.save(ROOT/f'assets/slot/{shape}_simple_mould.png')
        print(shape,'mould, details, lettering and depth saved')
if __name__=='__main__':main()
