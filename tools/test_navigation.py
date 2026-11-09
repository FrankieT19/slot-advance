"""Compare the compiled fixed-point controller with Slot's float spring equations.
The tap spring stays unchanged. Holds repeat steadily at 200 ms, independent of IO.
"""
from pathlib import Path
import argparse,ctypes as C,math,random,tempfile,json
ROOT=Path(__file__).resolve().parents[1]
DT=280896/16777216
class Nav(C.Structure):
    _fields_=[(x,C.c_uint) for x in ['enabled','count','index','wrap','animated','previous','due','fired','steps']]+[('direction',C.c_int),('offset',C.c_int32),('velocity',C.c_int32)]
class Reference:
    def __init__(self,count,index,wrap,animated):
        self.count=count;self.index=index;self.wrap=wrap;self.animated=animated
        self.previous=0;self.direction=0;self.due=0;self.fired=0;self.steps=0;self.offset=0.;self.velocity=0.
    def step(self,by):
        if self.count<2:return
        target=self.index+by
        if not self.wrap and not 0<=target<self.count:return
        self.index=target%self.count;self.offset-=by;self.steps+=1
    def tick(self,held,now):
        horizontal=held&48
        if horizontal==48:horizontal=0
        if horizontal!=self.previous:
            self.direction=(1 if horizontal&16 else -1) if horizontal else 0
            if self.direction:self.step(self.direction);self.due=now+400;self.fired=0
        self.previous=horizontal
        if self.direction and now>=self.due:
            self.step(self.direction);self.due=now+200
        if not self.animated:self.offset=self.velocity=0.;return
        pull=self.velocity+16*self.offset;decay=math.exp(-16*DT)
        self.offset=(self.offset+pull*DT)*decay
        self.velocity=(self.velocity-16*pull*DT)*decay

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--tcc',type=Path,required=True);a=p.parse_args();base=a.tcc.resolve()
    work=ROOT/'work';work.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(dir=work) as folder:
        folder=Path(folder);source=folder/'nav.c';dll=folder/'nav.dll'
        source.write_text('#include "slot_navigation.h"\n__declspec(dllexport) void reset(SlotNavigation*n,unsigned c,unsigned i,unsigned w,unsigned a){SlotNav_Reset(n,c,i,w,a);}\n__declspec(dllexport) void tick(SlotNavigation*n,unsigned held,unsigned now){SlotNav_Tick(n,held,now);}\n',encoding='utf-8')
        tcc=C.CDLL(str(base/'libtcc.dll'));tcc.tcc_new.restype=C.c_void_p
        for name,args in [('tcc_set_lib_path',[C.c_void_p,C.c_char_p]),('tcc_add_include_path',[C.c_void_p,C.c_char_p]),('tcc_add_library_path',[C.c_void_p,C.c_char_p]),('tcc_set_output_type',[C.c_void_p,C.c_int]),('tcc_add_file',[C.c_void_p,C.c_char_p]),('tcc_output_file',[C.c_void_p,C.c_char_p]),('tcc_delete',[C.c_void_p])]:getattr(tcc,name).argtypes=args
        state=tcc.tcc_new();tcc.tcc_set_lib_path(state,str(base).encode());tcc.tcc_add_include_path(state,str(base/'include').encode());tcc.tcc_add_include_path(state,str(ROOT/'common').encode());tcc.tcc_add_library_path(state,str(base/'lib').encode());tcc.tcc_set_output_type(state,3)
        assert tcc.tcc_add_file(state,str(source).encode())==0;assert tcc.tcc_output_file(state,str(dll).encode())==0;tcc.tcc_delete(state)
        lib=C.CDLL(str(dll));lib.reset.argtypes=[C.POINTER(Nav)]+[C.c_uint]*4;lib.tick.argtypes=[C.POINTER(Nav),C.c_uint,C.c_uint]
        rng=random.Random(4096);checks=0;worst=0;repeat=[]
        left,right=Nav(),Nav()
        for nav in (left,right):lib.reset(C.byref(nav),512,0,1,1)
        for frame in range(180):
            now=int(frame*DT*1000)
            lib.tick(C.byref(left),32 if frame<120 else 0,now)
            lib.tick(C.byref(right),16 if frame<120 else 0,now)
            assert left.offset==-right.offset and left.velocity==-right.velocity,frame
        for count,wrap,animated in [(512,1,1),(3,1,1),(2,1,1),(512,0,1),(0,1,1),(1,1,1),(3,1,0)]:
            n=Nav();lib.reset(C.byref(n),count,0,wrap,animated);r=Reference(count,0,wrap,animated)
            held=0
            for frame in range(12000):
                if frame<180:held=16
                elif frame<300:held=0
                elif frame<420:held=32 if frame%2 else 16
                elif frame<540:held=16 if frame%2 else 0
                elif rng.random()<.08:held=rng.choice([0,16,32,48])
                now=int(frame*DT*1000);before=n.steps
                lib.tick(C.byref(n),held,now);r.tick(held,now)
                assert (n.index,n.steps,n.direction)==(r.index,r.steps,r.direction),(frame,n.index,r.index)
                error=abs(n.offset/65536-r.offset)*120;worst=max(worst,error)
                assert error<.04,('spring differs from Slot',frame,error)
                if count==512 and wrap and animated and frame<180 and n.steps!=before:repeat.append(now)
                checks+=1
        # No renderer acknowledgements exist: held targets keep advancing even
        # if the frontend never draws. A release only stops future repeats.
        n=Nav();lib.reset(C.byref(n),512,0,1,1)
        for frame in range(180):lib.tick(C.byref(n),16,int(frame*DT*1000))
        assert n.steps==14,(n.steps,n.index)
        lib.tick(C.byref(n),0,3014);stopped=n.steps
        for frame in range(180,300):lib.tick(C.byref(n),0,int(frame*DT*1000))
        assert n.steps==stopped
        handle=lib._handle;del lib;C.windll.kernel32.FreeLibrary.argtypes=[C.c_void_p];C.windll.kernel32.FreeLibrary(handle)
    report={'status':'pass','vblank_comparisons':checks,'max_position_error_gba_pixels':worst,'hold_step_times_ms':repeat,'repeat_delay_ms':400,'repeat_intervals_ms':[200],'scrolling_never_waits_for_renderer':True,'spring_omega':16,'input_queue':False,'source':'Slot spring equations; GBA-specific steady 200 ms repeat','scope':'Compiled shared controller versus independent float spring and slower hold schedule, with no render acknowledgements. Does not measure physical SD stalls or LCD cadence.'}
    (ROOT/'validation/navigation-validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
