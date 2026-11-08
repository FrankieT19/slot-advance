"""Exercise rapid taps and count visible motion frames in the real GBA preview."""
from pathlib import Path
import argparse,ctypes as C,struct,json
from test_preview import Emulator,ROOT

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--core',type=Path,required=True);a=p.parse_args()
    e=Emulator(a.core);e.load(ROOT/'preview/slot-preview.gba');e.run(180)
    def state():return struct.unpack('<10I',C.string_at(e.ewram[0]+0x3f000,40))
    def tap(key):e.keys={key};e.run(1);e.keys=set();e.run(1)
    def guarded_shot(name):
        before=e.frame.tobytes()
        # Compare the displayed frame against complete redraws, including one
        # with all three sprite caches freshly loaded. Detect stale side carts.
        for command in (2,1):
            C.c_uint.from_address(e.ewram[0]+0x3f040).value=command;e.run(8)
            assert e.frame.tobytes()==before,('stale sprite pixels',name,command)
        e.shot(name)
    frames=[];e.keys={7};e.run(1);frames.append(e.frame.copy());e.keys=set()
    for _ in range(90):
        e.run(1);frames.append(e.frame.copy())
        if state()[7]==1 and not state()[9]:break
    assert state()[4]==1 and state()[7]==1,state()
    changed=sum(frames[i].tobytes()!=frames[i-1].tobytes() for i in range(1,len(frames)))
    movement={'emulated_frames':len(frames),'ms':len(frames)/59.7275*1000,'distinct_frame_changes':changed}
    before=state()[7]
    # Target changes must already be applied while the spring is still moving.
    for _ in range(4):tap(7)
    e.run(4);assert state()[7]==before+4 and state()[4]==2,state()
    e.run(50);assert state()[7]==before+4 and state()[4]==2,state()
    for key in [6,7,6,7]:tap(key)
    e.run(50);assert state()[7]==before+8 and state()[4]==2,state()
    # Repeat cannot build a backlog that keeps scrolling after release.
    e.keys={7};e.run(70);e.keys=set();e.run(20);stopped=state()[7];e.run(40)
    assert state()[7]==stopped,state()
    e.press(3);e.press(5);e.press(8);assert state()[8]==0,state();e.shot('settings-flat.png')
    e.press(0);guarded_shot('gba-flat-wood.png')
    e.press(3);e.press(8);e.press(0);guarded_shot('gba-flat-dark.png')
    e.press(3);e.press(8);e.press(0);guarded_shot('gba-flat-light.png')
    e.press(3);e.press(5);e.press(8);e.press(0);guarded_shot('gba-real-light.png')
    e.press(3);e.press(8);e.press(8);e.press(0);guarded_shot('gba-real-dark.png')
    e.press(3);e.press(5);e.press(8);e.press(0)
    e.press(11);guarded_shot('gb-flat-dark.png');e.press(11);guarded_shot('gbc-flat-dark.png');e.press(11)
    e.press(3);e.press(4);e.keys={8};e.run(30);assert state()[5]==5,state()
    e.keys=set();e.run(4);assert state()[5]==5,state();e.shot('about.png')
    e.press(0);e.press(4);e.keys={8};e.run(30);assert state()[5]==6,state()
    e.keys=set();e.run(4);e.shot('help-menu.png')
    first_help=e.frame.tobytes();e.press(7);assert state()[5]==6 and e.frame.tobytes()!=first_help,state();e.shot('help-ingame.png')
    e.press(6);assert e.frame.tobytes()==first_help,state()
    e.press(0);e.press(0)
    # The empty-shelf preview is a debug command; Rescan was removed from Settings.
    C.c_uint.from_address(e.ewram[0]+0x3f040).value=4;e.run(8)
    assert state()[5]==4,state();e.shot('empty.png');e.press(0)
    e.press(8);e.shot('inserted-flat.png')
    report={'status':'pass','single_move':movement,'rapid_taps':8,'target_retargets_before_settling':True,'direction_reversal_preserved':True,'held_repeat_stops_on_release':True,'skeuomorphic_toggle':True,'about_page':True,'about_survives_entering_key':True,'help_pages':2,'help_survives_entering_key':True,'cache_refresh_preserves_pixels':True,'scope':'Shared GBA renderer and continuously retargeted spring. Settling includes a nearly invisible subpixel tail, not an input lockout. Physical SD reads and LCD cadence require cartridge testing.'}
    (ROOT/'validation/motion-validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2));e.close()
if __name__=='__main__':main()
