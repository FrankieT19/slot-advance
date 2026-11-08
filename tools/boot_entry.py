"""Install early black entry without changing the flashcart's kernel header.

Keep both devkitARM entry branches intact. Hook the CRT's first instruction,
which the trampoline replays before returning to the second instruction.
"""
import struct
from pathlib import Path
from build import run,tool_path

def arm_branch_target(data,offset):
    instruction=struct.unpack_from('<I',data,offset)[0]
    if instruction>>24!=0xea:raise RuntimeError('Expected an unconditional ARM entry branch')
    displacement=instruction&0xffffff
    if displacement&0x800000:displacement-=1<<24
    return offset+8+displacement*4

def thumb_bl_target(data,offset):
    high,low=struct.unpack_from('<HH',data,offset)
    if high&0xf800!=0xf000 or low&0xf800!=0xf800:
        raise RuntimeError('Expected Thumb BL at CRT EWRAM clear')
    displacement=((high&0x7ff)<<12)|((low&0x7ff)<<1)
    if displacement&0x400000:displacement-=0x800000
    return offset+4+displacement

def thumb_bl(offset,target):
    displacement=target-offset-4
    if displacement&1 or not -0x400000<=displacement<0x400000:
        raise RuntimeError('EWRAM-clear hook is outside Thumb BL range')
    return struct.pack('<HH',0xf000|((displacement>>12)&0x7ff),0xf800|((displacement>>1)&0x7ff))

def patch_black_entry(original,code):
    data=bytearray(original)
    # Preserve the entry/header structure used by known-working kernels.
    if len(data)<0xe4 or struct.unpack_from('<I',data)[0]!=0xea00002e:
        raise RuntimeError('Kernel must retain the standard 0xEA00002E header entry')
    vector=arm_branch_target(data,0)
    entry=arm_branch_target(data,vector)
    if entry!=0xe0 or struct.unpack_from('<I',data,entry)[0]!=0xe3a00301:
        raise RuntimeError('Unsupported CRT startup: expected MOV r0, #0x04000000 at 0xE0')
    if data[0x126:0x12c]!=bytes.fromhex('40210903c801') or thumb_bl_target(data,0x12c)!=0x188:
        raise RuntimeError('Unsupported CRT EWRAM clear: refusing an unsafe warm-return hook')
    marker=struct.pack('<I',0x12345678)
    if code.count(marker)!=1:raise RuntimeError('Invalid boot trampoline relocation')
    code=code.replace(marker,struct.pack('<I',0x08000000+entry+4))
    clear_marker=struct.pack('<I',0x23456789)
    if code.count(clear_marker)!=1:raise RuntimeError('Invalid EWRAM-clear relocation')
    code=code.replace(clear_marker,struct.pack('<I',0x08000189))
    data+=b'\0'*((-len(data))%4);offset=len(data);data+=code
    struct.pack_into('<I',data,entry,0xea000000|((offset-entry-8)//4))
    # The first 60 bytes are the ARM black-entry stub; Thumb clear wrapper follows.
    data[0x12c:0x130]=thumb_bl(0x12c,offset+60)
    if data[:entry]!=original[:entry]:raise RuntimeError('Kernel header was modified')
    return data

def install_black_entry(rom,dk,out):
    source=Path(__file__).resolve().parents[1]/'common/slot_boot_entry.s'
    obj=out/'black-entry.o';raw=out/'black-entry.bin'
    run([tool_path(dk/'devkitARM/bin','arm-none-eabi-as'),'-mcpu=arm7tdmi',source,'-o',obj],out/'black-entry-asm.log')
    run([tool_path(dk/'devkitARM/bin','arm-none-eabi-objcopy'),'-O','binary','-j','.text',obj,raw],out/'black-entry-bin.log')
    rom.write_bytes(patch_black_entry(rom.read_bytes(),raw.read_bytes()))
