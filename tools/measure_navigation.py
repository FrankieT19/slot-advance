"""Measure displayed cadence during held navigation in the compiled GBA preview.
This measures emulator frames, not a physical Omega LCD or SD-card latency.
"""
from pathlib import Path
import argparse,ctypes as C,struct,json
from test_preview import Emulator,ROOT

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--core',type=Path,required=True);a=p.parse_args()
    e=Emulator(a.core);e.load(ROOT/'preview/slot-preview.gba');e.run(180);results={}
    def state():return struct.unpack('<19I',C.string_at(e.ewram[0]+0x3f000,76))
    for platform in ['GBA','GB','GBC']:
        e.keys={7};previous=e.frame.tobytes();changes=0;costs=[]
        for frame in range(240):
            e.run(1);current=e.frame.tobytes()
            if frame>=60:
                changes+=current!=previous;costs.append(state()[11])
            previous=current
        e.keys=set();e.run(90)
        results[platform]={'display_changes_in_180_vblanks':changes,'display_changes_per_second':round(changes/180*59.7275,2),'draw_cpu_cycles_min':min(costs),'draw_cpu_cycles_max':max(costs),'input_model_hz':59.7275}
        e.press(11)
    optimized,legacy=struct.unpack('<2I',C.string_at(e.ewram[0]+0x3f050,8))
    assert optimized<legacy,(optimized,legacy)
    e.close()
    path=ROOT/'validation/performance.json';report=json.loads(path.read_text(encoding='utf-8'))
    report['v0.5']={'held_navigation':results,'bmp_colour_conversion':{'pixels':104*112,'old_thumb_cpu_cycles':legacy,'new_arm_cpu_cycles':optimized,'speedup':round(legacy/optimized,2)},'method':'180 display frames after 60 held warm-up frames per platform. Spring controller ticks each VBlank; steady 200 ms repeat, no loading/presentation gate. Decoder comparison uses prior Thumb conversion versus new ARM conversion on identical staged pixels. No SD IO in preview.'}
    path.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');print(json.dumps(report['v0.5'],indent=2))
if __name__=='__main__':main()
