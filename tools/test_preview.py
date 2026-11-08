"""Render the compiled preview and regression-test it through an mGBA libretro core.
Usage: python tools/test_preview.py --core /path/to/mgba_libretro.dll
The external emulator core is not redistributed with this project.
"""
from pathlib import Path
import argparse,ctypes as C,json,struct,sys
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
ENV=C.CFUNCTYPE(C.c_bool,C.c_uint,C.c_void_p)
VIDEO=C.CFUNCTYPE(None,C.c_void_p,C.c_uint,C.c_uint,C.c_size_t)
AUDIO=C.CFUNCTYPE(None,C.c_int16,C.c_int16)
BATCH=C.CFUNCTYPE(C.c_size_t,C.POINTER(C.c_int16),C.c_size_t)
POLL=C.CFUNCTYPE(None)
INPUT=C.CFUNCTYPE(C.c_int16,C.c_uint,C.c_uint,C.c_uint,C.c_uint)
class Game(C.Structure):_fields_=[('path',C.c_char_p),('data',C.c_void_p),('size',C.c_size_t),('meta',C.c_char_p)]
class Info(C.Structure):_fields_=[('library_name',C.c_char_p),('library_version',C.c_char_p),('valid_extensions',C.c_char_p),('need_fullpath',C.c_bool),('block_extract',C.c_bool)]
class MemoryDescriptor(C.Structure):
    _fields_=[('flags',C.c_uint64),('ptr',C.c_void_p),('offset',C.c_size_t),('start',C.c_size_t),('select',C.c_size_t),('disconnect',C.c_size_t),('length',C.c_size_t),('addrspace',C.c_char_p)]
class MemoryMap(C.Structure):_fields_=[('descriptors',C.POINTER(MemoryDescriptor)),('num_descriptors',C.c_uint)]
class Emulator:
    def __init__(self,core):
        self.lib=C.CDLL(str(core.resolve()));self.pixel_format=0;self.frame=None;self.keys=set();self.frames=0;self.ewram=None;self.capture_audio=False;self.audio_data=bytearray()
        self.env=ENV(self.environment);self.video=VIDEO(self.on_video)
        self.audio=AUDIO(lambda a,b:None);self.batch=BATCH(self.on_audio_batch)
        self.poll=POLL(lambda:None);self.input=INPUT(self.on_input)
        self.lib.retro_set_environment(self.env);self.lib.retro_set_video_refresh(self.video)
        self.lib.retro_set_audio_sample(self.audio);self.lib.retro_set_audio_sample_batch(self.batch)
        self.lib.retro_set_input_poll(self.poll);self.lib.retro_set_input_state(self.input)
        self.lib.retro_get_system_info.argtypes=[C.POINTER(Info)]
        self.info=Info();self.lib.retro_get_system_info(C.byref(self.info))
        self.lib.retro_init()
        self.lib.retro_load_game.argtypes=[C.POINTER(Game)];self.lib.retro_load_game.restype=C.c_bool
        self.lib.retro_get_memory_data.argtypes=[C.c_uint];self.lib.retro_get_memory_data.restype=C.c_void_p
        self.lib.retro_get_memory_size.argtypes=[C.c_uint];self.lib.retro_get_memory_size.restype=C.c_size_t
    def environment(self,cmd,data):
        cmd=cmd&0xffff
        if cmd==10:
            self.pixel_format=C.cast(data,C.POINTER(C.c_uint))[0];return self.pixel_format in (0,1,2)
        if cmd in (3,51):
            if data: C.cast(data,C.POINTER(C.c_bool))[0]=True
            return True
        if cmd==36:
            mmap=C.cast(data,C.POINTER(MemoryMap))[0]
            for i in range(mmap.num_descriptors):
                desc=mmap.descriptors[i]
                if desc.start==0x02000000 and desc.length>=0x40000:self.ewram=(desc.ptr+desc.offset,desc.length)
            return True
        if cmd==47:
            C.cast(data,C.POINTER(C.c_int))[0]=3;return True
        if cmd in (11,12,16,18,35,36,44,45,52,53,54,55,56,62,67,68,69):return True
        if cmd==17:
            C.cast(data,C.POINTER(C.c_bool))[0]=False;return True
        return False
    def on_audio_batch(self,ptr,frames):
        if self.capture_audio:self.audio_data.extend(C.string_at(ptr,frames*4))
        return frames
    def on_input(self,port,device,index,key):
        if key==256:return sum(1<<k for k in self.keys)
        return int(key in self.keys)
    def on_video(self,ptr,w,h,pitch):
        if not ptr:return
        data=C.string_at(ptr,pitch*h)
        if self.pixel_format==1:
            self.frame=Image.frombytes('RGB',(w,h),data,'raw','BGRX',pitch)
        else:
            self.frame=Image.frombytes('RGB',(w,h),data,'raw','BGR;15' if self.pixel_format==0 else 'BGR;16',pitch)
        self.frames+=1
    def load(self,path):
        self.rom=C.create_string_buffer(path.read_bytes());self.rompath=str(path.resolve()).encode()
        self.game=Game(self.rompath,C.cast(self.rom,C.c_void_p),len(self.rom)-1,None)
        assert self.lib.retro_load_game(C.byref(self.game)),'mGBA rejected preview ROM'
    def run(self,n):
        for _ in range(n):self.lib.retro_run()
    def press(self,key,after=120):
        self.keys={key};self.run(2);self.keys=set();self.run(after)
    def state(self):
        assert self.ewram, "mGBA did not provide its EWRAM memory map"
        ptr,size=self.ewram
        assert size>=0x3f01c and ptr, f'mGBA memory size {size}, ptr {ptr}'
        return struct.unpack('<7I',C.string_at(ptr+0x3f000,28))
    def shot(self,name):
        path=ROOT/'preview'/name
        assert self.frame and self.frame.size==(240,160)
        self.frame.save(path)
        return path
    def close(self):self.lib.retro_unload_game();self.lib.retro_deinit()

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--core',type=Path,required=True);a=p.parse_args()
    e=Emulator(a.core);e.load(ROOT/'preview/slot-preview.gba');e.run(180)
    signature,checks,failed,platform,index,screen,theme=e.state()
    print('GBA regression results:',hex(signature),checks,failed)
    assert signature==0x534c4f54 and checks>1800 and failed==0,e.state()
    interactive=0
    def expect(condition):
        nonlocal interactive
        interactive+=1
        assert condition,e.state()
    e.shot('gba.png')
    e.press(7);expect(e.state()[4]==1);e.shot('gba-clear.png')
    e.press(7);expect(e.state()[4]==2);e.shot('gba-glitter.png')
    e.press(7);expect(e.state()[4]==0)
    e.press(11);expect(e.state()[3]==1);e.shot('gb.png')
    e.press(11);expect(e.state()[3]==2);e.shot('gbc.png')
    previous=e.state();e.press(2);expect(e.state()==previous)
    e.press(3);expect(e.state()[5]==1);e.shot('settings.png')
    e.press(8);expect(e.state()[6]==1);e.shot('settings-dark.png')
    e.press(8);expect(e.state()[6]==2);e.shot('settings-light.png')
    e.press(0);expect(e.state()[5]==0);e.shot('gbc-light.png')
    e.press(0);expect(e.state()[5]==0) # GB/GBC have no cartridge options.
    e.press(11);expect(e.state()[3]==0)
    e.press(0);expect(e.state()[5]==2);e.shot('cartridge-back.png')
    e.press(0);e.capture_audio=True;e.audio_data.clear();e.press(8,120);expect(e.state()[5]==3);e.shot('inserted.png')
    samples=struct.unpack('<'+'h'*(len(e.audio_data)//2),e.audio_data)
    audio_peak=max((abs(x) for x in samples),default=0);expect(audio_peak>20000)
    e.capture_audio=False
    e.press(0);previous=e.state();e.press(2);expect(e.state()==previous)
    e.press(11);expect(e.state()[3]==1);e.press(0);expect(e.state()[5]==0)
    e.press(11);expect(e.state()[3]==2);e.press(0);expect(e.state()[5]==0)
    e.press(11);expect(e.state()[3]==0)
    # Every shelf remembers its independent selection while switching systems.
    expect(e.state()[4]==0)
    e.press(7);expect(e.state()[4]==1)
    e.press(11);expect(e.state()[3]==1 and e.state()[4]==0)
    e.press(7);expect(e.state()[4]==1)
    e.press(11);expect(e.state()[3]==2 and e.state()[4]==0)
    e.press(11);expect(e.state()[3]==0 and e.state()[4]==1)
    e.press(10);expect(e.state()[3]==2 and e.state()[4]==0)
    e.press(10);expect(e.state()[3]==1 and e.state()[4]==1)
    report={'emulator' :e.info.library_name.decode()+' '+e.info.library_version.decode(),
            'compiled_c_checks':checks,'failed':failed,'interactive_checks':interactive,
            'insert_audio_peak':audio_peak,'video':'240x160','scope':'Shared renderer and pure C format/navigation logic; flashcart IO and ROM launch require hardware.'}
    (ROOT/'preview/validation.json').write_text(json.dumps(report,indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report,indent=2));e.close()
if __name__=='__main__':main()


