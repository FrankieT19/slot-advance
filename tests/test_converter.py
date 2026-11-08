"""Integration tests for the real Slot/Cart Studio export layout and BMP contract."""
from pathlib import Path
import hashlib,io,json,os,struct,sys,tempfile,unittest,zipfile
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import convert_labels as c

class ConverterTests(unittest.TestCase):
    def setUp(self):
        temp=Path(os.environ.get('SLOT_TEST_TMP',ROOT/'work'));temp.mkdir(parents=True,exist_ok=True)
        self.tmp=tempfile.TemporaryDirectory(dir=temp);self.base=Path(self.tmp.name)
    def tearDown(self):self.tmp.cleanup()
    def png(self,colour='red',size=(128,66)):
        b=io.BytesIO();Image.new('RGBA',size,colour).save(b,format='PNG');return b.getvalue()
    def export(self,entries):
        p=self.base/'export.zip'
        with zipfile.ZipFile(p,'w',zipfile.ZIP_STORED) as z:
            for name,data in entries.items():z.writestr(name,data)
        return p
    def rom(self,platform,name,code='',flag=0,title=b''):
        p=self.base/'card/Games'/platform/name;p.parent.mkdir(parents=True,exist_ok=True)
        data=bytearray(0x150)
        if platform=='GBA':data[0xac:0xb0]=code.ljust(4).encode()
        else:
            data[0x134:0x134+len(title)]=title;data[0x13f:0x143]=code.ljust(4).encode();data[0x143]=flag;data[0x14a]=1
        p.write_bytes(data);return p
    def test_export_layout_matches_cart_studio(self):
        # web/studio.js writes these exact paths in a store-only UTF-8 ZIP.
        z=self.export({'Labels/GBA/Orbit.png':self.png(),'Labels/GB/Moon.png':self.png(),
                       'Labels/GBC/Prism.png':self.png(),'Labels/cart_shell.txt':'Orbit = auto 8844cc clear\nMoon = notched aabbcc solid\nPrism = rounded 123456 glitter\n'})
        before=hashlib.sha256(z.read_bytes()).digest()
        report=c.convert(z,self.base/'out',previews=True)
        self.assertEqual(len(report['items']),3)
        by_platform={x['platform']:x for x in report['items']}
        self.assertEqual(by_platform['GBA']['cart_size'],(120,71))
        self.assertEqual(by_platform['GBA']['label_size'],(88,45))
        self.assertEqual(by_platform['GB']['cart_size'],(104,112))
        self.assertEqual(by_platform['GB']['label_size'],(74,66))
        self.assertEqual(by_platform['GBC']['label_size'],(74,66))
        self.assertEqual(by_platform['GBA']['shell']['colour'],'8844cc')
        self.assertEqual(by_platform['GBC']['shell']['finish'],'glitter')
        self.assertEqual(hashlib.sha256(z.read_bytes()).digest(),before)
        self.assertTrue((self.base/'out/SYSTEM/SLOT/conversion.json').is_file())
    def test_folder_labels_and_case_insensitive_extensions(self):
        d=self.base/'Labels/GBA';d.mkdir(parents=True)
        (d/'Orbit.PNG').write_bytes(self.png())
        self.assertEqual(c.convert(d.parent,self.base/'out')['items'][0]['stem'],'Orbit')
    def test_system_colour_overridden_by_label_choice(self):
        system=b'A = auto ff0000 solid\nB = rounded 223344 clear\n'
        labels=b'A = auto 00ff00 glitter\nB = auto\n'
        result=c.layered_shells(system,labels)
        self.assertEqual(result['A'],c.Shell('auto','00ff00','glitter'));self.assertNotIn('B',result)
    def test_unicode_nfc_shell_keys(self):
        result=c.layered_shells(b'', 'Cafe\u0301 = auto 223344 solid\n'.encode())
        self.assertIn('Café',result)
    def test_malformed_shell_values_ignored(self):
        for value in ('auto zzzzzz solid','rounded ff0000 chrome','auto ff0000 solid extra','nonsense'):
            self.assertIsNone(c.parse_shell(value))
    def test_comments_and_bom(self):
        self.assertEqual(c.parse_ini(b'\xef\xbb\xbf# foo\n[A]\n; comment\nA = auto\n'),{'A':'auto'})
    def test_zip_traversal_rejected(self):
        z=self.export({'../escape.png':self.png()})
        with self.assertRaises(ValueError):c.Input(z)
        self.assertFalse((self.base.parent/'escape.png').exists())
    def test_absolute_zip_path_rejected(self):
        z=self.export({'C:/escape.png':self.png()})
        with self.assertRaises(ValueError):c.Input(z)
    def test_case_collision_rejected(self):
        z=self.export({'Labels/GBA/A.png':self.png(),'Labels/GBA/a.PNG':self.png()})
        with self.assertRaises(ValueError):c.Input(z)
    def test_same_stem_two_image_formats_rejected(self):
        z=self.export({'Labels/GBA/A.png':self.png(),'Labels/GBA/A.bmp':self.png()})
        with self.assertRaises(ValueError):c.convert(z,self.base/'out')
    def test_empty_export_rejected(self):
        z=self.export({'README.txt':'No labels'})
        with self.assertRaises(ValueError):c.convert(z,self.base/'out')
    def test_corrupt_image_rejected(self):
        z=self.export({'Labels/GBA/A.png':b'not a PNG'})
        with self.assertRaises(OSError):c.convert(z,self.base/'out')
    def test_source_cannot_be_overwritten(self):
        d=self.base/'Labels/GBA';d.mkdir(parents=True);(d/'A.png').write_bytes(self.png())
        with self.assertRaises(ValueError):c.convert(d.parent,d.parent)
        with self.assertRaises(ValueError):c.convert(d.parent,d.parent/'converted')
    def test_fat_name_validation(self):
        for name in ('x'*96,'bad/name','bad:foo','emoji\U0001f600','bad.','bad '):
            with self.assertRaises(ValueError,msg=name):c.validate_name(name)
        c.validate_name('x'*95);c.validate_name('测试');c.validate_name('Orbit (USA)')
    def test_automatic_gba_special_shells(self):
        for code,colour,finish in [('AXVE','c2332e','clear'),('BPEE','399c3c','clear'),('RZWJ','eceee8','solid'),('MXXX','c6c6c9','solid')]:
            self.assertEqual(c.automatic_shell('GBA',self.rom('GBA',code+'.gba',code)).colour,colour)
            self.assertEqual(c.automatic_shell('GBA',self.rom('GBA',code+'.gba',code)).finish,finish)
    def test_automatic_gb_flags_choose_outline(self):
        for flag,shape,colour in [(0,'notched','9a978f'),(0x80,'notched','333031'),(0xc0,'rounded','7c7a8a')]:
            p=self.rom('GBC','Header.gbc',flag=flag)
            s=c.automatic_shell('GBC',p);self.assertEqual((s.outline,s.colour),(shape,colour))
    def test_automatic_gb_special_code_glitter(self):
        p=self.rom('GBC','Crystal.gbc',code='BYTE',flag=0xc0)
        self.assertEqual(c.automatic_shell('GBC',p),c.Shell('rounded','86b9bf','glitter'))
    def test_original_gb_title_shell(self):
        p=self.rom('GB','Red.gb',title=b'POKEMON RED')
        self.assertEqual(c.automatic_shell('GB',p).colour,'c0282c')
    def test_roms_without_labels_get_title_cartridges(self):
        self.rom('GBA','Orbit.gba',code='BPEE')
        z=self.export({'README.txt':'Metadata only'})
        item=c.convert(z,self.base/'out',self.base/'card')['items'][0]
        self.assertFalse(item['scraped_label']);self.assertTrue(item['header_read'])
        self.assertEqual(item['shell']['colour'],'399c3c')
    def test_matching_label_and_rom_convert_once(self):
        self.rom('GBA','Orbit.gba',code='BPRX')
        z=self.export({'Labels/GBA/ORBIT.png':self.png()})
        report=c.convert(z,self.base/'out',self.base/'card')
        self.assertEqual(len(report['items']),1);self.assertEqual(report['items'][0]['stem'],'Orbit')
    def test_gbc_shape_override_keeps_shelf(self):
        z=self.export({'Labels/GBC/Orbit.png':self.png(),'Labels/cart_shell.txt':'Orbit = notched 223344 solid\n'})
        item=c.convert(z,self.base/'out')['items'][0]
        self.assertEqual(item['platform'],'GBC');self.assertEqual(item['shell']['outline'],'notched')
        self.assertEqual(item['cart_size'],(104,112))
    def test_bmp555_red_and_blue_channels(self):
        im=Image.new('RGBA',(2,1));im.putdata([(255,0,0,255),(0,0,255,255)])
        data=c.bmp555(im)
        self.assertEqual(struct.unpack_from('<2H',data,54),(0x7c00,0x001f))
        decoded=Image.open(io.BytesIO(data)).convert('RGB')
        self.assertEqual(list(decoded.getdata()),[(255,0,0),(0,0,255)])
    def test_bmp_padding_bottom_up_and_size(self):
        im=Image.new('RGBA',(3,2));im.putdata([(255,0,0,255)]*3+[(0,0,255,255)]*3)
        data=c.bmp555(im)
        self.assertEqual(len(data),54+8*2)
        self.assertEqual(struct.unpack_from('<H',data,54)[0],31)
        self.assertEqual(struct.unpack_from('<H',data,62)[0],0x7c00)
        self.assertEqual(struct.unpack_from('<I',data,2)[0],len(data))
        self.assertEqual(struct.unpack_from('<H',data,28)[0],16)
    def test_transparency_key_and_magenta_collision(self):
        im=Image.new('RGBA',(2,1));im.putdata([(0,0,0,0),(255,0,255,255)])
        a,b=struct.unpack_from('<2H',c.bmp555(im),54)
        self.assertEqual(a,c.TRANSPARENT);self.assertNotEqual(b,c.TRANSPARENT)
    def test_cover_and_contain(self):
        label=Image.new('RGB',(100,10),'red')
        a,la=c.compose('GB','A',label,c.Shell('auto','ffffff','solid'),'cover')
        b,lb=c.compose('GB','A',label,c.Shell('auto','ffffff','solid'),'contain')
        self.assertNotEqual(la.tobytes(),lb.tobytes());self.assertEqual(a.size,b.size)
    def test_transparent_logo_gets_paper(self):
        im=Image.new('RGBA',(100,100),(0,0,0,0))
        _,label=c.compose('GBA','A',im,c.Shell('auto','333333','solid'))
        self.assertEqual(label.getpixel((label.width//2,label.height//2))[3],255)
        self.assertEqual(label.getpixel((0,0))[3],0)
    def test_cached_shells_do_not_leak_previous_labels(self):
        shell=c.Shell('auto','4444cc','clear')
        first,_=c.compose('GBA','A',Image.new('RGB',(128,66),'red'),shell)
        second,_=c.compose('GBA','B',Image.new('RGB',(128,66),'blue'),shell)
        again,_=c.compose('GBA','A',Image.new('RGB',(128,66),'red'),shell)
        self.assertEqual(first.tobytes(),again.tobytes());self.assertNotEqual(first.tobytes(),second.tobytes())
    def test_default_fit_is_centre_crop(self):
        im=Image.new('RGB',(1000,100),'red');im.paste('blue',(400,0,600,100))
        _,label=c.compose('GB','A',im,c.Shell('auto','333333','solid'))
        self.assertGreater(label.getpixel((label.width//2,label.height//2))[2],200)
    def test_repeat_conversion_is_deterministic(self):
        z=self.export({'Labels/GBA/A.png':self.png()})
        c.convert(z,self.base/'out');file=self.base/'out/SYSTEM/SLOT/CARTS/GBA/A.bmp';before=file.read_bytes()
        c.convert(z,self.base/'out');self.assertEqual(file.read_bytes(),before)
    def test_shared_gb_gbc_save_stem_warning(self):
        self.rom('GB','Same.gb');self.rom('GBC','Same.gbc',flag=0xc0)
        z=self.export({'README.txt':'No art'})
        report=c.convert(z,self.base/'out',self.base/'card')
        self.assertEqual(len(report['items']),2)
        self.assertTrue(any('.esv' in warning for warning in report['warnings']))
    def test_malformed_shell_does_not_override_valid_config(self):
        z=self.export({'Labels/GBA/A.png':self.png(),'Config/cart_shell.txt':'A = auto 112233 solid\n','Labels/cart_shell.txt':'A = bad\n'})
        self.assertEqual(c.convert(z,self.base/'out')['items'][0]['shell']['colour'],'112233')

    def test_round_label_corners_reveal_shell_well(self):
        face,label=c.compose('GBA','A',Image.new('RGBA',(1280,661),'red'),c.Shell('auto','444444','solid'))
        self.assertEqual(label.getpixel((0,0))[3],0)
        self.assertEqual(label.getpixel((label.width//2,label.height//2)),(255,0,0,255))
        self.assertNotEqual(face.getpixel((16,17))[:3],(255,0,0))
        for platform in c.PLATFORMS:
            _,paper=c.compose(platform,'A',None,c.Shell('auto','444444','solid'))
            w,h=paper.size
            missing={(x,y) for y in range(h) for x in range(w) if paper.getpixel((x,y))[3]==0}
            self.assertEqual(missing,{(0,0),(w-1,0),(0,h-1),(w-1,h-1)})
    def test_scan_border_is_removed(self):
        source=Image.new('RGBA',(880,450),'red')
        source.paste('blue',(10,10,870,440))
        _,label=c.compose('GBA','A',source,c.Shell('auto','444444','solid'))
        self.assertGreater(label.getpixel((label.width//2,2))[2],200)
    def test_sd_card_app_converts_in_place_without_touching_inputs(self):
        from converter_app import convert_card
        rom=self.rom('GBA','Orbit.gba',code='BPEE')
        label=self.base/'card/Labels/GBA/Orbit.png';label.parent.mkdir(parents=True);label.write_bytes(self.png('red'))
        save=self.base/'card/SYSTEM/SAVER/Orbit.sav';save.parent.mkdir(parents=True);save.write_bytes(b'KEEP THIS SAVE')
        originals={p:p.read_bytes() for p in (rom,label,save)}
        progress=[];result=convert_card(self.base/'card',lambda *args:progress.append(args))
        self.assertEqual(len(result['items']),1);self.assertEqual(progress[-1][:2],(1,1))
        for p,data in originals.items():self.assertEqual(p.read_bytes(),data)
        result=convert_card(self.base/'card')
        self.assertEqual(len(result['items']),1)
    def test_card_app_preserves_existing_labels_when_png_missing(self):
        from converter_app import convert_card
        self.rom('GBA','Orbit.gba',code='BPEE')
        cart=self.base/'card/SYSTEM/SLOT/CARTS/GBA/Orbit.bmp';cart.parent.mkdir(parents=True)
        face,_=c.compose('GBA','Orbit',Image.new('RGBA',(880,450),'red'),c.Shell('auto','44aa44','solid'))
        cart.write_bytes(c.bmp555(face));result=convert_card(self.base/'card')
        self.assertTrue(result['items'][0]['recovered_existing_cart'])
        center=Image.open(cart).getpixel((60,40));self.assertGreater(center[0],200);self.assertLess(center[2],30)
        once=cart.read_bytes();convert_card(self.base/'card');self.assertEqual(cart.read_bytes(),once)
    def test_card_app_rejects_unrelated_folder(self):
        from converter_app import convert_card
        with self.assertRaises(ValueError):convert_card(self.base)

    def test_generated_text_has_only_native_ink_and_paper(self):
        _,label=c.compose('GBA','Native Pixel Name',None,c.Shell('auto','8855cc','solid'))
        colours={px[:3] for px in label.getdata() if px[3]}
        self.assertEqual(colours,{(40,40,47),(233,228,216)})

    def test_slot_text_metadata_is_redrawn_in_native_font(self):
        source=Image.new('RGBA',(1280,661),'white');source.info['slot-label']='text aabbcc'
        _,tagged=c.compose('GBA','Native Pixel Name',source,c.Shell('auto','8855cc','solid'))
        _,native=c.compose('GBA','Native Pixel Name',None,c.Shell('auto','8855cc','solid'))
        self.assertEqual(tagged.tobytes(),native.tobytes())
        source.info['slot-label']='logo aabbcc'
        _,logo=c.compose('GBA','Native Pixel Name',source,c.Shell('auto','8855cc','solid'))
        self.assertNotEqual(logo.tobytes(),native.tobytes())

    def test_transparent_label_corners_show_coloured_plastic(self):
        a,_=c.compose('GBA','A',Image.new('RGB',(880,450),'white'),c.Shell('auto','cc2222','solid'))
        b,_=c.compose('GBA','A',Image.new('RGB',(880,450),'white'),c.Shell('auto','2222cc','solid'))
        red=a.getpixel((16,17));blue=b.getpixel((16,17))
        self.assertGreater(red[0],red[2]);self.assertGreater(blue[2],blue[0])

    def test_simple_style_uses_slot_mould_details_and_preserves_label(self):
        shell=c.Shell('auto','cc2222','solid');photo=Image.new('RGB',(880,450),'blue')
        real,label=c.compose('GBA','A',photo,shell)
        flat,flat_label=c.compose('GBA','A',photo,shell,skeuomorphic=False)
        self.assertEqual(label.tobytes(),flat_label.tobytes());self.assertNotEqual(real.tobytes(),flat.tobytes())
        # Slot's embossed lettering/ridge now shades the coloured plastic.
        shoulder=[flat.getpixel((x,y)) for y in range(3,16) for x in range(12,108)]
        self.assertGreater(len(set(shoulder)),10)
        self.assertTrue(all(r>g and r>b for r,g,b,a in shoulder if a))
        export=self.export({'Labels/GBA/A.png':self.png()});result=c.convert(export,self.base/'out')
        self.assertTrue((self.base/'out'/result['items'][0]['flat_cart']).is_file())

    def test_simple_clear_plastic_keeps_slot_board_and_rim(self):
        solid,_=c.compose('GBC','A',None,c.Shell('rounded','8855cc','solid'),skeuomorphic=False)
        clear,_=c.compose('GBC','A',None,c.Shell('rounded','8855cc','clear'),skeuomorphic=False)
        glitter,_=c.compose('GBC','A',None,c.Shell('rounded','8855cc','glitter'),skeuomorphic=False)
        self.assertNotEqual(solid.tobytes(),clear.tobytes());self.assertNotEqual(clear.tobytes(),glitter.tobytes())
        self.assertEqual(solid.getpixel((0,0))[3],0);self.assertEqual(clear.getpixel((0,0))[3],0)

if __name__=='__main__':
    runner=unittest.main(verbosity=2,exit=False)
    result=runner.result
    report={'run':result.testsRun,'failed':len(result.failures)+len(result.errors)}
    (ROOT/'validation').mkdir(exist_ok=True)
    (ROOT/'validation/converter-validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    raise SystemExit(0 if result.wasSuccessful() else 1)
