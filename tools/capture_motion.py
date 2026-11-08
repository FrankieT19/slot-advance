"""Capture real preview-ROM frames, including black entry and timed motion."""
from pathlib import Path
import argparse
from PIL import Image
from test_preview import Emulator,ROOT

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--core',type=Path,required=True);a=p.parse_args()
    e=Emulator(a.core);e.load(ROOT/'preview/slot-preview.gba');frames=[]
    def run(n,key=None):
        e.keys=set() if key is None else {key}
        for _ in range(n):
            e.run(1)
            if e.frame:frames.append(e.frame.resize((480,320),Image.Resampling.NEAREST))
    run(100);run(2,7);run(35);run(2,6);run(28)
    run(2,8);run(45);run(2,0);run(4)
    run(2,11);run(24);run(2,11);run(24);run(2,8);run(45)
    e.close()
    duration=[int((i+1)*100/59.7275)*10-int(i*100/59.7275)*10 for i in range(len(frames))]
    frames[0].save(ROOT/'preview/motion.gif',save_all=True,append_images=frames[1:],duration=duration,loop=0,optimize=True)
    print('Captured',len(frames),'emulated GBA frames')
if __name__=='__main__':main()
