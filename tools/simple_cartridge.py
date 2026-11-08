"""Slot's original shell_face/mould_detail/recess_label, adapted from cart.rs.
Pinned source: 57ea55f9763c01eb4e3ac12e8b0240ec682dba3f, GPL-3.0-or-later.
Vector details and lettering are rasterised by generate_simple_moulds.py.
This creates the actual designed shells, including clear internals; no shadows.
"""
from functools import lru_cache
from PIL import Image

@lru_cache(maxsize=128)
def shell_face(asset_dir,shape,colour,finish,panel):
    mould=Image.open(asset_dir/f'{shape.lower()}_simple_mould.png').convert('RGBA')
    w,h=mould.size;plastic=tuple(bytes.fromhex(colour));clear=finish!='solid'
    lerp=lambda a,b,n,d:tuple((x*(d-n)+y*n)//d for x,y in zip(a,b))
    def through(behind):
        return lerp(tuple(a*b//255 for a,b in zip(plastic,behind)),tuple(a*a//255 for a in plastic),150,255)
    base=through(plastic);mean=sum(base)//3
    edge=tuple((v:=(3*c+mean)//4)+(255-v)*2//5 for c in base)
    out=[];rim=(4*w+120)//240 if shape=='GBA' else (7*w+120)//240
    for i,(shadow,light,depth,alpha) in enumerate(mould.getdata()):
        x,y=i%w,i//w;fx,fy=x/w,y/h;c=plastic
        if clear:
            left,right,top,c0,c1,contacts,traces=(.135,.875,50/212,.17,.84,.85,.76) if shape=='GBA' else (.075,.925,.07,.1,.9,.905,.84)
            inside=plastic
            if left<=fx<right and fy>=top:
                pitch=(c1-c0)/32;pin=c0<=fx<c1 and (fx-c0)%pitch<pitch*.6
                inside=(230,180,70) if pin and fy>=contacts else (90,180,116) if pin and (fx-c0)%pitch<pitch*.2 and fy>=traces else (44,150,82)
            c=lerp(edge,through(inside),min(depth,rim),rim)
            if finish=='glitter':
                noise=((x*0x9e3779b9)^(y*0x85ebca6b))&0xffffffff
                noise^=noise>>15;noise=noise*0x2c1b3c6d&0xffffffff;noise^=noise>>12
                if noise%100==0:c=lerp(c,(255,255,255),100,255)
        under=c
        if shadow:c=tuple((int(v*.62)*shadow+old*(255-shadow)+127)//255 for v,old in zip(under,c))
        if light:c=tuple(((v+int((255-v)*.24))*light+old*(255-light)+127)//255 for v,old in zip(under,c))
        out.append((*c,alpha))
    lx,ly,lw,lh=panel;bevel=(3*w+120)//240
    x0,y0=lx-bevel,ly-bevel;x1,y1=lx+lw+bevel,ly+lh+bevel
    for y in range(y0,y1):
        for x in range(x0,x1):
            if lx<=x<lx+lw and ly<=y<ly+lh:continue
            upper=min(y-y0,x-x0)<=min(y1-y-1,x1-x-1)
            old=out[y*w+x]
            c=tuple(min(255,int(v*(.7 if upper else 1.18))) for v in old[:3]) if clear else tuple(min(255,int(v*(.55 if upper else 1.45))) for v in plastic)
            out[y*w+x]=(*c,old[3])
    image=Image.new('RGBA',(w,h));image.putdata(out);return image
