"""Regression checks for the update-file header and early startup hook."""
from pathlib import Path
import struct,sys,unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from boot_entry import patch_black_entry,arm_branch_target

class BootEntryTests(unittest.TestCase):
    def fixture(self):
        # The devkitARM header/vector used by the working 0.1 kernels.
        data=bytearray(0x400)
        struct.pack_into('<I',data,0,0xea00002e)
        struct.pack_into('<I',data,0xc0,0xea000006)
        struct.pack_into('<I',data,0xe0,0xe3a00301)
        return data

    def test_header_and_secondary_vector_remain_intact(self):
        original=self.fixture();code=struct.pack('<II',0xe3a00301,0x12345678)
        patched=patch_black_entry(original,code)
        self.assertEqual(patched[:0xe0],original[:0xe0])
        self.assertEqual(patched[0xe4:len(original)],original[0xe4:])
        self.assertEqual(arm_branch_target(patched,0xe0),len(original))
        self.assertEqual(struct.unpack_from('<I',patched,len(patched)-4)[0],0x080000e4)

    def test_rejects_the_02_header_regression(self):
        original=self.fixture();struct.pack_into('<I',original,0,0xea063f7c)
        with self.assertRaisesRegex(RuntimeError,'0xEA00002E'):
            patch_black_entry(original,struct.pack('<I',0x12345678))

    def test_rejects_unrecognised_crt(self):
        original=self.fixture();struct.pack_into('<I',original,0xe0,0xe1a00000)
        with self.assertRaisesRegex(RuntimeError,'Unsupported CRT'):
            patch_black_entry(original,struct.pack('<I',0x12345678))

    def test_release_header_and_checksum(self):
        for model,filename in [('omega','ezkernel.bin'),('omega-de','ezkernelnew.bin')]:
            with self.subTest(model=model):
                data=(ROOT/'release'/model/filename).read_bytes()
                self.assertEqual(struct.unpack_from('<I',data)[0],0xea00002e)
                self.assertEqual(struct.unpack_from('<I',data,0xc0)[0],0xea000006)
                self.assertEqual((sum(data[0xa0:0xbd])+data[0xbd]+0x19)&255,0)
                self.assertEqual(data[0xb2],0x96)
                self.assertLessEqual(len(data),2*1024*1024)
                target=arm_branch_target(data,0xe0)
                # Source-built stub replays MOV r0, IO_BASE, then returns to E4.
                self.assertEqual(len(data)-target,60)
                self.assertEqual(struct.unpack_from('<I',data,target+40)[0],0xe3a00301)
                self.assertEqual(struct.unpack_from('<I',data,target+56)[0],0x080000e4)

if __name__=='__main__':unittest.main(verbosity=2)
