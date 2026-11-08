"""Run the full modified Jagoomba with self-authored GB/GBC battery-save ROMs.

The test build substitutes only mapper-bank access and the final reboot. The
emulator's CPU, physical VBlank clock, menus and SRAM save routines are real.
"""
from pathlib import Path
import argparse,ctypes as C,json,struct
from test_preview import Emulator
ROOT=Path(__file__).resolve().parents[1]
def gb_rom(color):
    data=bytearray(32768);data[0x100:0x104]=bytes.fromhex('00c35001')
    data[0x104:0x134]=bytes.fromhex('CE ED 66 66 CC 0D 00 0B 03 73 00 83 00 0C 00 0D 00 08 11 1F 88 89 00 0E DC CC 6E E6 DD DD D9 99 BB BB 67 63 6E 0E EC CC DD DC 99 9F BB B9 33 3E')
    data[0x134:0x13f]=b'SLOT SAVE 1';data[0x143]=0x80 if color else 0
    data[0x146]=3;data[0x14b]=0x33  # Both fixtures advertise SGB support.
    data[0x147]=3;data[0x148]=0;data[0x149]=2
    # Enable MBC1 RAM; increment a persisted byte once per game boot.
    # Mirror it to C000 so the host can inspect it without bypassing the GB CPU.
    code=bytearray.fromhex('f33e00e0403e0aea0000fa00a0fe5a')
    branch=len(code);code+=b'\x28\0'
    code+=bytes.fromhex('3e5aea00a03e01')
    jump=len(code);code+=b'\xc3\0\0'
    already=len(code);code+=bytes.fromhex('fa01a03c')
    store=len(code);code+=bytes.fromhex('ea01a0ea00c0')
    # Send a real SGB PCT_TRN packet. SGB-only GB games must not draw a border
    # either, and dual SGB/CGB games must skip the automatic SGB dry-run.
    def pulse(value):code.extend(bytes([0x3e,value,0xe0,0]))
    pulse(0);pulse(0x30)
    for value in bytes([0xa1])+bytes(15):
        for bit in range(8):pulse(0x10 if value&(1<<bit) else 0x20);pulse(0x30)
    pulse(0x20);pulse(0x30)
    loop=0x150+len(code);code+=b'\x00\xc3'+struct.pack('<H',loop)
    code[branch+1]=already-branch-2;struct.pack_into('<H',code,jump+1,0x150+store)
    data[0x150:0x150+len(code)]=code
    checksum=0
    for x in data[0x134:0x14d]:checksum=(checksum-x-1)&255
    data[0x14d]=checksum;struct.pack_into('>H',data,0x14e,sum(data[:0x14e]+data[0x150:])&65535)
    return data
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--core',type=Path,required=True)
    p.add_argument('--build-root',type=Path,default=ROOT/'work/emulator-test');p.add_argument('--target',choices=['omega','omega-de'],default='omega-de');a=p.parse_args()
    folder=a.build_root.resolve()/a.target;work=ROOT/'work/emulator-validation'/a.target;work.mkdir(parents=True,exist_ok=True)
    addresses={l.split()[-1]:int(l.split()[0],16) for l in (folder/'symbols.txt').read_text().splitlines() if len(l.split())==3}
    total=0;results=[]
    for color in (False,True):
        path=work/('test-gbc.gba' if color else 'test-gb.gba');path.write_bytes((folder/'jagoomba.gba').read_bytes()+gb_rom(color))
        e=Emulator(a.core);e.load(path);e.run(180)
        def pointer(address):
            if 0x02000000<=address<0x02040000:return e.ewram[0]+address-0x02000000
            if 0x03000000<=address<0x03008000:return e.iwram[0]+address-0x03000000
            raise AssertionError(hex(address))
        def value(name):return C.c_uint.from_address(pointer(addresses[name])).value
        def set_value(name,n):C.c_uint.from_address(pointer(addresses[name])).value=n
        def check(condition,description):
            nonlocal total
            total+=1
            if not condition:raise AssertionError(('GBC' if color else 'GB',description,{n:value(n) for n in ['Slot_JagoombaVBlanks','Slot_JagoombaTestMenus','Slot_JagoombaTestExit','main_ui_selection','ui_x']}))
        def press(keys,frames=2,after=20):e.keys=set(keys);e.run(frames);e.keys=set();e.run(after)
        def counter():return C.c_ubyte.from_address(pointer(addresses['XGB_RAM'])).value
        def arm():
            name=b'Save test.gbc' if color else b'Save test.gb';name=name.ljust(100,b'\0');platform=2 if color else 1
            h=2166136261
            for x in [platform,1,*name]:h=((h^x)*16777619)&0xffffffff
            record=struct.pack('<5I100s2I',0x52544c53,h,0,0,platform,name,1,0)
            C.memmove(pointer(addresses['Slot_JagoombaTestRecord']),record,len(record))
            return h
        check(counter()==1,'GB ROM runs and initialises battery RAM')
        def byte(name):return C.c_ubyte.from_address(pointer(addresses[name])).value
        check(byte('auto_border')==0,'SGB automatic border loading defaults off')
        check(not(byte('ui_border_visible')&2),'SGB border packet does not display a border')
        if not color:
            packet=addresses['GLOBAL_PTR_BASE']+addresses['borderpartsadded']
            check(C.c_ubyte.from_address(pointer(packet)).value&4,'GB CPU sent a real SGB border packet')
        # Simulate border bits restored by an older emulator save state.
        set_value('ui_border_request',value('ui_border_request')|2);e.run(8)
        check(not(value('ui_border_screen')&2),'old border requests are masked at display')
        check(not(value('ui_border_buffer')&2),'old border requests are masked in scanline buffers')
        canaries=[C.c_uint.from_address(pointer(addresses[n])).value for n in ['CANARY1','CANARY2']]
        press([10,11]);check(value('Slot_JagoombaTestMenus')==0,'L+R no longer opens menu')
        press([2],20);check(not value('Slot_JagoombaTestExit'),'short Select does not exit')
        press([2,3],30);check(value('Slot_JagoombaTestMenus')==1,'Start+Select opens once while held')
        check(not value('Slot_JagoombaTestExit'),'menu chord cannot trigger hold exit')
        press([0]);check(value('main_ui_selection')==0xffffffff,'B resumes game')
        check(counter()==1,'resume does not restart ROM')
        arm();e.keys={2};e.run(170);check(not value('Slot_JagoombaTestExit'),'hold does not fire early')
        e.run(40);e.keys=set();e.run(40)
        check(value('Slot_JagoombaTestExit')==1,'Select hold exits through menu/save path')
        check(max(e.frame.tobytes())==0,'Select-hold exit fades every display area to black')
        check(value('Slot_JagoombaTestPage')==0,'return restores save bank zero')
        record=C.string_at(pointer(addresses['Slot_JagoombaTestRecord']),128)
        check(struct.unpack_from('<2I',record,8)==(0x5443454a,0xabbcbab5),'one-shot return request written')
        check(struct.unpack_from('<I',record,16)[0]==(2 if color else 1),'return identifies correct shelf')
        cookie=struct.unpack('<4I',C.string_at(e.ewram[0]+0x3fff0,16))
        check(cookie[0]==0x53574a45 and cookie[2]==struct.unpack_from('<I',record,4)[0],'powered-session cookie written')
        check([C.c_uint.from_address(pointer(addresses[n])).value for n in ['CANARY1','CANARY2']]==canaries,'emulator IWRAM canaries survive save and exit')
        e.lib.retro_reset();e.run(180)
        check(counter()==2,'GB battery save survives full emulator restart')
        check(not value('Slot_JagoombaTestExit'),'cold boot does not request another exit')
        check(byte('auto_border')==0,'borders remain disabled after restart without saving settings')
        # Disabled hold still permits the menu and manual Exit.
        set_value('slot_hold_enabled',0);press([2],220)
        check(not value('Slot_JagoombaTestExit'),'Select hold setting respected')
        press([2,3]);check(value('Slot_JagoombaTestMenus')==1,'menu works with hold disabled')
        arm();press([4]);press([8],after=60)
        check(value('Slot_JagoombaTestExit')==1,'manual menu Exit uses return hook')
        check(max(e.frame.tobytes())==0,'manual Exit also fades to black')
        e.lib.retro_reset();e.run(180)
        check(counter()==3,'manual menu Exit also preserves the battery save')
        press([2,3]);press([4]);press([4]);press([8],after=100)
        check(not value('Slot_JagoombaTestExit'),'Restart does not return or eject')
        check(counter()==4,'Restart restarts the GB ROM')
        results.append({'system':'GBC' if color else 'GB','save_roundtrip':True,'hold_exit':True,'menu_exit':True,'cold_boot':True,'restart_stays_in_emulator':True,'sgb_borders_disabled':True,'black_exit_fade':True})
        e.close()
    report={'status':'pass','target':a.target,'checks':total,'results':results,'scope':'Full modified emulator with homebrew GB/GBC ROMs: actual CPU, VBlank clock, menu and battery SRAM handling. Cartridge SRAM banking and the final kernel/FPGA reboot use test shims and still require physical testing.'}
    (ROOT/('validation/emulator-'+a.target+'-validation.json')).write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
