"""Package corresponding source and per-cartridge hardware-test candidates."""
from pathlib import Path
import datetime,difflib,hashlib,json,os,shutil,zipfile,tempfile
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1]
DIST=ROOT/'dist'

def write_zip(path,base,prefix='',source=False):
    with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
        for file in sorted(base.rglob('*')):
            rel=file.relative_to(base)
            if any(part in ('__pycache__','work','build','.git','dist') for part in rel.parts):continue
            if file.suffix in ('.pyc','.elf','.map','.log'):continue
            if source and rel.parts[0]=='release':continue
            if source and file.suffix=='.exe' and file.name!='slot-advance-label-converter-v1.0.exe':continue
            name=(Path(prefix)/rel).as_posix()
            if file.is_dir():
                if not any(file.iterdir()):archive.writestr(name+'/',b'')
            else:archive.write(file,name)

def main():
    DIST.mkdir(exist_ok=True)
    reports=ROOT/'validation';reports.mkdir(exist_ok=True)
    builds=json.loads((ROOT/'build-report.json').read_text(encoding='utf-8'))
    upstream=json.loads((ROOT/'UPSTREAM.json').read_text(encoding='utf-8-sig'))
    emulator=json.loads((ROOT/'preview/validation.json').read_text(encoding='utf-8'))
    host=json.loads((ROOT/'preview/host-validation.json').read_text(encoding='utf-8'))
    converter=json.loads((reports/'converter-validation.json').read_text(encoding='utf-8'))
    app=json.loads((ROOT/'app/validation.json').read_text(encoding='utf-8'))
    app_build=json.loads((ROOT/'app/build.json').read_text(encoding='utf-8'))
    boot_entry=json.loads((reports/'boot-entry-validation.json').read_text(encoding='utf-8'))
    assert boot_entry['status']=='pass'
    audio=json.loads((reports/'audio-validation.json').read_text(encoding='utf-8'))
    assert audio['status']=='pass'
    navigation=json.loads((reports/'navigation-validation.json').read_text(encoding='utf-8'))
    assert navigation['status']=='pass'
    motion=json.loads((reports/'motion-validation.json').read_text(encoding='utf-8'))
    startup_save=json.loads((reports/'startup-save-validation.json').read_text(encoding='utf-8'))
    assert motion['status']=='pass' and startup_save['failed']==0
    performance=json.loads((reports/'performance.json').read_text(encoding='utf-8'))
    assert converter['failed']==0 and host['failed']==0 and emulator['failed']==0 and app['status']=='pass'
    preview_sha=hashlib.sha256((ROOT/'preview/slot-preview.gba').read_bytes()).hexdigest()
    # Keep the exact backend delta reviewable alongside all corresponding source.
    originals=ROOT.parent.parent/'work'
    patches=ROOT/'patches';patches.mkdir(exist_ok=True)
    for target,baseline in [('omega','omega-ds-style-kernel-master'),('omega-de','omega-de-ds-style-kernel-main')]:
        original=originals/baseline
        if original.is_dir():
            delta=[]
            for name in ['source/ezkernelnew.c','source/launch_indicator_anchor.h','source/launch_indicator.h','source/menu_fade.h','source/GBApatch.c','source/GBApatch.h','source/gba_rts_patch.s','source/Ezcard_OP.c','source/slot_blit_kernel.s','Makefile']:
                before=(original/name).read_bytes().splitlines(keepends=True) if (original/name).exists() else []
                after=(ROOT/'kernels'/target/name).read_bytes().splitlines(keepends=True)
                delta.extend(difflib.diff_bytes(difflib.unified_diff,before,after,fromfile=('a/'+name).encode() if (original/name).exists() else b'/dev/null',tofile=('b/'+name).encode()))
            (patches/(target+'.patch')).write_bytes(b''.join(delta))
    # Build a compact contact sheet directly from emulator captures.
    canvas=Image.new('RGB',(1024,820),'#1b1b20');draw=ImageDraw.Draw(canvas)
    font=ImageFont.truetype(str(ROOT/'assets/slot/DejaVuSansMono.ttf'),18)
    small=ImageFont.truetype(str(ROOT/'assets/slot/DejaVuSansMono.ttf'),13)
    draw.text((16,14),'slot. advance  /  240 x 160',font=font,fill='#eee6d2')
    draw.text((16,42),'Actual GBA emulator captures. Fictional example labels.',font=small,fill='#9f9b91')
    for i,(name,label) in enumerate([('gba.png','GAME BOY ADVANCE'),('gb.png','GAME BOY'),('gbc.png','GAME BOY COLOR'),('settings.png','SETTINGS')]):
        x=16+(i%2)*512;y=76+(i//2)*366
        draw.text((x,y),label,font=small,fill='#d4cbbb')
        frame=Image.open(ROOT/'preview'/name).resize((480,320),Image.Resampling.NEAREST)
        canvas.paste(frame,(x,y+23))
    canvas.save(ROOT/'preview/overview.png')
    # Capture source hashes and checks without making hardware claims.
    report={'date':'2026-11-08','public_version':'v1.0','status':'hardware-test candidate','converter_tests':converter,'converter_application':app,'converter_application_build':app_build,'animation_performance':performance,
            'audio':audio,'navigation':navigation,'motion':motion,'startup_save':startup_save,'boot_entry':boot_entry,'frontend_host':host,'emulator':emulator,'builds':builds,
            'preview_sha256':preview_sha,
            'not_verified':['Boot on physical Omega and Omega DE Rev A / Rev B','Physical SD IO and cartridge PSRAM cache behaviour','Native GBA launch and GB/GBC emulator launch','Save write-back across full power cycles','Native GBA addon reset on 180 VBlanks, Start+Select menu, states/sleep and game compatibility','Physical LCD animation, sound and input timing; startup/reset display before kernel entry']}
    (reports/'results.json').write_text(json.dumps(report,indent=2)+'\n', encoding='utf-8')
    lines=['# Validation — 8 November 2026','','Status: **hardware-test candidate**. No physical cartridge was available to this run.','','## Builds','',
           '| Target | Kernel bytes | 2 MiB envelope headroom | Static EWRAM | Static IWRAM | IWRAM headroom to user stack |',
           '| --- | ---: | ---: | ---: | ---: | ---: |']
    for item in builds:
        ram=item['ram'];lines.append(f'| {item["target"]} | {item["bytes"]:,} | {item["headroom_bytes"]:,} | {ram["ewram_static_bytes"]:,} | {ram["iwram_static_bytes"]:,} | {ram["iwram_headroom_to_user_stack_bytes"]:,} |')
    lines+=['','Both builds use devkitARM GCC 16.1.0, ARM7TDMI/Thumb, -Os and section collection. The build enforces the 2 MiB envelope, EWRAM capacity, and an IWRAM stack reserve. The per-game artwork stays on SD and does not change the kernel size.',
            '', 'Compiler output contains three warnings per target in retained legacy DS Style UI functions (two indentation warnings and one bounded snprintf truncation warning). Those source locations were inherited; no warning originated in the new shared frontend.',
            '', '## Automated checks','',
            f'- **{converter["run"]} converter integration tests pass.** Actual Cart Studio path/metadata layout; BOM/NFC; case matching; header-driven colours and shapes; transparent/clear/glitter assets; RGB555 channels, padding, transparency key; malformed inputs; original-file preservation; shared GB/GBC save-stem warning, inset one-pixel label corners, existing-BMP migration, repeat in-place GUI conversion and preservation of source data.',
            f'- **{host["checks"]} host frontend checks pass.** The actual shared frontend header is compiled against fault-injected hardware/SD mocks and real converted BMPs. Checks include settings snapshots/backup recovery, short writes, failed rename/read, BMP caching, directory sorting, empty/missing/error states, 512-entry bounds, clean/addon launch delegation and menu input. New checks cover skipping empty shelves, immediate targets, release gates, style persistence, bounded username loading/placement and 36 exact resting-endpoint comparisons plus 288 full-frame comparisons between incremental and complete redraws across three platforms, both designs, both directions and all three backgrounds.',
            f'- **{emulator["compiled_c_checks"]:,} compiled GBA checks pass** in {emulator["emulator"]}: navigation boundaries/wrapping, natural-number overflow, strict BMP header validation, rendering bounds guards, and the actual ARM hold-reset macro (threshold, release, Start exclusion, enabled IRQ flags and flag-register preservation), plus the ARM row blitter and exhaustive BGR555 conversion of all 65,536 inputs, odd lengths and destination guards.' ,
            f'- **{emulator["interactive_checks"]} interactive emulator assertions pass** for all three shelves, wrapping, independent remembered selections, themes, settings, cartridge-back, insertion and empty states. Screenshots come from the compiled GBA renderer, not an HTML mock-up.',
            f'- **Standalone Windows app passes.** Frozen executable converts ten labels into thirty BMPs twice with only Windows on PATH. Tagged text has only ink/paper/transparency colours. Originals are verified unchanged. The folder-picker/worker flow was previously exercised through Windows Computer Use; v1.0 converter, one-pixel label corners and original procedural mould assets are verified in the frozen executable.',
            f'- **Insert sound verified in the emulator**, peak {emulator["insert_audio_peak"]} in captured PCM. Only the insertion clip is used. Smooth limiting raises offline waveform RMS by {audio["rms_gain_db_vs_0_4"]:.2f} dB over 0.4; duration remains 240 ms.',
            f'- **Motion/input checks pass:** eight rapid taps, direction reversals, steady 200 ms repeats, release behavior, simple-style toggle, About v1.0, both Help pages (without the unused Select line), slot. advance branding and removal of Rescan. The target changes before settling, without a transition queue. A tap including its subpixel spring tail settles in {motion["single_move"]["emulated_frames"]} emulated GBA frames (~{motion["single_move"]["ms"]:.0f} ms); input remains responsive throughout. The unchanged spring, with the slower hold schedule, matches its independent float reference across {navigation["vblank_comparisons"]:,} VBlanks, maximum position difference {navigation["max_position_error_gba_pixels"]:.3f} GBA pixels. Held navigation shows about 30 changed display images per second on all three shelves in mGBA. Physical SD latency is not represented.',
            f'- **Streaming cache checks pass:** {host["steady_stream_frames"]} steady held-scroll frames across all platforms and both styles have {host["steady_visible_cache_misses"]} visible cache misses in the mock-card simulation. Partial images stay hidden; cancellation/retry, short/failed reads, malformed and missing art are covered. Real SD latency is not simulated. The ARM colour-conversion benchmark is {performance["v0.5"]["bmp_colour_conversion"]["speedup"]:.2f}x faster than the prior Thumb loop. Scrolling never waits on a renderer acknowledgement.',
            '- **Startup save:** five fault-injected checks compile the actual DE save-recovery function and verify success, absent flag, failed write, failed path and retry. Failed writes keep the pending marker. The normal write does not load artwork or draw the old popup.',
            '- **0.2.1 boot-header regression checks pass.** Four unit tests, valid GBA checksum, and the first 224 bytes of both release kernels matching the working 0.1 headers exactly. The early ARM trampoline executes successfully in mGBA and preserves black startup. This addresses the suspected header cause of the reported no-kernel-found error; physical loader acceptance still needs confirmation.',
            '', 'The preview and host tests do not emulate the actual Omega FPGA or physical SD bus. The host launch stub validates handoff parameters, not successful cartridge boot. Memory headroom is a linker measurement, not a runtime stack high-water measurement.',
            '', '## Hardware verification still required','']
    lines+=['- [ ] '+x for x in report['not_verified']]
    lines+=['','Black entry applies from the first instruction in the supplied kernel. A firmware loader displayed before it runs remains outside kernel control. Select-hold is a native GBA addon patch, not a clean-boot or GB/GBC emulator hotkey. Games without the expected VBlank IRQ path require separate compatibility testing.', '', 'Recommended first pass: use a backed-up card and a disposable/homebrew ROM. Test clean boot, a save/reboot round trip, GB and GBC saves, then addon paths. Confirm the same sequence independently on both hardware families. Check empty shelves, an absent or malformed BMP, long filenames, 512+ files, and animation off/on. A failed or interrupted update must be recovered using the correct known-working kernel for that model.',
            '', '## SHA-256','']
    for item in builds:lines+=['`'+item['target']+'/'+item['file']+'`','',item['sha256'],'']
    lines+=['`preview/slot-preview.gba`','',preview_sha,'']
    (reports/'REPORT.md').write_text('\n'.join(lines), encoding='utf-8')
    # Stage complete end-user ZIPs. Only SD_CARD is copied to the card;
    # Tools stays on the computer. A name template never overwrites USERNAME.TXT.
    stage_parent=ROOT/'work/package';stage_parent.mkdir(parents=True,exist_ok=True)
    stage_root=Path(tempfile.mkdtemp(prefix='slot-advance-v1.0-',dir=stage_parent))
    assert app_build['file']=='slot-advance-label-converter-v1.0.exe'
    assert hashlib.sha256((ROOT/'app'/app_build['file']).read_bytes()).hexdigest()==app_build['sha256']
    for item in builds:
        stage=stage_root/item['target'];card=stage/'SD_CARD';card.mkdir(parents=True)
        source_kernel=ROOT/'release'/item['target']/item['file']
        shutil.copy2(source_kernel,card/item['file'])
        for folder in ['SYSTEM/SAVER','SYSTEM/SLOT/CARTS/GBA','SYSTEM/SLOT/CARTS/GB','SYSTEM/SLOT/CARTS/GBC','SYSTEM/SLOT/CARTS_FLAT/GBA','SYSTEM/SLOT/CARTS_FLAT/GB','SYSTEM/SLOT/CARTS_FLAT/GBC','SYSTEM/SLOT/LABELS/GBA','SYSTEM/SLOT/LABELS/GB','SYSTEM/SLOT/LABELS/GBC','Games/GBA','Games/GB','Games/GBC']:
            (card/folder).mkdir(parents=True,exist_ok=True)
        (card/'SYSTEM/SLOT/README.TXT').write_text('slot. advance v1.0.\nUse the bundled Windows converter under Tools to prepare labels and colours.\nGames go in /Games/GBA, /Games/GB and /Games/GBC. Saves stay in /SYSTEM/SAVER.\nOptional /USERNAME.TXT: first line, up to 10 characters. Settings and Help are opened with Start.\n',encoding='utf-8')
        tools=stage/'Tools';tools.mkdir();shutil.copy2(ROOT/'app'/app_build['file'],tools/app_build['file'])
        shutil.copytree(ROOT/'app/licenses',tools/'licenses')
        for name in ['LICENSE','NOTICE.md','CHANGELOG.md','RELEASE_NOTES.md','USERNAME.example.txt']:
            shutil.copy2(ROOT/name,stage/name)
        guide=(ROOT/'README.md').read_text(encoding='utf-8').replace('(docs/BUILD.md)','(https://github.com/FrankieT19/slot-advance/blob/main/docs/BUILD.md)').replace('(validation/REPORT.md)','(VALIDATION.md)')
        (stage/'README.md').write_text(guide,encoding='utf-8')
        (stage/'preview').mkdir();shutil.copy2(ROOT/'preview/overview.png',stage/'preview/overview.png')
        shutil.copy2(reports/'REPORT.md',stage/'VALIDATION.md')
        (stage/'licenses').mkdir();shutil.copy2(ROOT/'kernels'/item['target']/'LICENSE',stage/'licenses/DS-Style-Apache-2.0.txt')
        for notice in ['label.ttf.OFL.txt','DejaVuSansMono.ttf.LICENSE.txt']:shutil.copy2(ROOT/'assets/slot'/notice,stage/'licenses'/notice)
        model='Original EZ-FLASH Omega only' if item['target']=='omega' else 'Original Omega Definitive Edition Rev A / Rev B only; NOT the newer DE B / EZODE-B'
        (stage/'MODEL.txt').write_text(model+'\nCopy the contents of SD_CARD to the card root.\nKernel filename: '+item['file']+'\nRead README.md for installation and artwork setup.\n',encoding='utf-8')
        manifest={'name':'slot. advance','version':'v1.0','target':item['target'],'kernel':{'path':'SD_CARD/'+item['file'],'sha256':item['sha256']},'converter':{'path':'Tools/'+app_build['file'],'sha256':app_build['sha256']}}
        (stage/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
        write_zip(DIST/('slot-advance-v1.0-'+item['target']+'.zip'),stage)
    write_zip(DIST/'slot-advance-v1.0-source-and-tools.zip',ROOT,'slot-advance',source=True)
    archives=sorted(DIST.glob('slot-advance-v1.0-*.zip'))
    sums=[]
    for archive in archives:
        with zipfile.ZipFile(archive) as z:assert z.testzip() is None
        sums.append(hashlib.sha256(archive.read_bytes()).hexdigest()+'  '+archive.name)
        print(archive.name,archive.stat().st_size)
    exe=DIST/app_build['file']
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==app_build['sha256']
    sums.append(app_build['sha256']+'  '+exe.name)
    (DIST/'slot-advance-v1.0-SHA256SUMS.txt').write_text('\n'.join(sums)+'\n', encoding='utf-8')
if __name__=='__main__':main()
