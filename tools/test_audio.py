"""Validate the insertion waveform and compare its body level to an older release.
Usage: python tools/test_audio.py --previous-project-zip path/to/0.4-source.zip
"""
from pathlib import Path
import argparse,io,json,math,re,wave,zipfile
ROOT=Path(__file__).resolve().parents[1]
def decode(source):
    with wave.open(source,'rb') as audio:
        assert audio.getframerate()==22050 and audio.getnchannels()==audio.getsampwidth()==1
        return [v-128 for v in audio.readframes(audio.getnframes())]
def rms(values):return math.sqrt(sum(v*v for v in values)/len(values))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--previous-project-zip',type=Path,required=True);a=p.parse_args()
    with zipfile.ZipFile(a.previous_project_zip) as z:previous=decode(io.BytesIO(z.read('slot-omega/preview/slot-insert.wav')))
    current=decode(str(ROOT/'preview/slot-insert.wav'))
    assert len(current)==len(previous)==5292 and current[0]==current[-1]==0
    assert max(map(abs,current))==127 and min(current)>=-127
    gain=20*math.log10(rms(current)/rms(previous));assert gain>4,gain
    header=(ROOT/'common/slot_sounds.h').read_text()
    samples=[int(v) for v in re.findall(r'-?\d+',header.split('{',1)[1].split('}',1)[0])]
    assert samples[:len(current)]==current and all(v==0 for v in samples[len(current):])
    report={'status':'pass','samples':len(current),'seconds':len(current)/22050,'signed8_peak':max(map(abs,current)),'rms_gain_db_vs_0_4':gain,'source_duration_preserved':True,'zero_ended_fades':True,'embedded_pcm_matches_wave':True,'scope':'Offline waveform level and range; actual speaker loudness depends on hardware. Smooth limiting raises the quieter body while keeping peaks within signed-8 range.'}
    (ROOT/'validation/audio-validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
