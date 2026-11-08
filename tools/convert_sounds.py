"""Convert Slot's 48 kHz signed-16 mono insert PCM to GBA signed-8 mono.
The original duration is preserved with a low-pass windowed-sinc resampler.
The original eject clip uses the same conversion. Run after changing either asset.
"""
from pathlib import Path
import math,struct,wave,json,hashlib
ROOT=Path(__file__).resolve().parents[1]
def resample(pcm,source_hz=48000,target_hz=22050):
    samples=struct.unpack('<'+'h'*(len(pcm)//2),pcm)
    count=round(len(samples)*target_hz/source_hz);cutoff=min(1,target_hz/source_hz)*.94
    result=[];radius=24
    for i in range(count):
        position=i*source_hz/target_hz;centre=math.floor(position);acc=total=0.
        for k in range(centre-radius+1,centre+radius+1):
            if not 0<=k<len(samples):continue
            x=position-k;t=x*cutoff
            sinc=1 if abs(t)<1e-10 else math.sin(math.pi*t)/(math.pi*t)
            window=.5+.5*math.cos(math.pi*x/radius) if abs(x)<=radius else 0
            weight=sinc*window*cutoff;acc+=samples[k]*weight;total+=weight
        sample=acc/total if total else 0
        result.append(sample)
    # Hardware FIFO A is already at 100%. Raise the quieter body of the sound
    # with a smooth limiter instead of clipping peaks or adding a second FIFO.
    peak=max((abs(v) for v in result),default=0) or 1
    drive=2.0;normalizer=127/math.tanh(drive)
    output=[]
    for i,v in enumerate(result):
        edge=min(1,i/(target_hz*.001),(count-1-i)/(target_hz*.001))
        output.append(round(normalizer*math.tanh(drive*v/peak)*edge)&255)
    return bytes(output)
def main():
    rows=['/* Original Slot insert and eject sounds, GPL-3.0-or-later. */']
    metadata={}
    for name in ['insert','eject']:
        source=ROOT/('assets/slot/audio/'+name+'-48000-s16.pcm')
        pcm=resample(source.read_bytes());pad=pcm+b'\0'*((-len(pcm))%16)
        rows+=['static const signed char slot_'+name+'_pcm[] __attribute__((aligned(4))) = {']
        rows+=[' '+','.join(str(v if v<128 else v-256) for v in pad[i:i+24])+',' for i in range(0,len(pad),24)]
        rows+=['};','#define SLOT_'+name.upper()+'_SAMPLES '+str(len(pcm))]
        metadata[name]={'source_rate':48000,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'samples':len(pcm),'seconds':len(pcm)/22050}
        preview=ROOT/('preview/slot-'+name+'.wav');preview.parent.mkdir(exist_ok=True)
        with wave.open(str(preview),'wb') as f:
            f.setnchannels(1);f.setsampwidth(1);f.setframerate(22050);f.writeframes(bytes(v^128 for v in pcm))
    rows+=['#define SLOT_SOUND_HZ 22050']
    (ROOT/'common/slot_sounds.h').write_text('\n'.join(rows)+'\n',encoding='utf-8')
    (ROOT/'assets/slot/audio/conversion.json').write_text(json.dumps({'output_rate':22050,'output_format':'signed 8-bit mono, 16-byte padded','smooth_limiter_drive':2.0,'eject_used':True,'clips':metadata},indent=2)+'\n',encoding='utf-8')
    print(json.dumps(metadata,indent=2))
if __name__=='__main__':main()
