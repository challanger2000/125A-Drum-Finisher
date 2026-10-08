#!/usr/bin/env python3
import array,csv,hashlib,json,math,os,pathlib,subprocess,sys,urllib.request,wave
ROOT=pathlib.Path("audio-qa-output")
ROOT.mkdir(exist_ok=True)
SOURCES={
 "SpeedMetal":"MusicDelta_SpeedMetal_Drum.wav",
 "Grunge":"MusicDelta_Grunge_Drum.wav",
 "Disco":"MusicDelta_Disco_Drum.wav",
 "FusionJazz":"MusicDelta_FusionJazz_Drum.wav",
}
BASE="https://raw.githubusercontent.com/CarlSouthall/MDBDrums/master/MDB%20Drums/audio/drum_only/"
def rms(a):return math.sqrt(sum(v*v for v in a)/max(1,len(a)))
def db(v):return 20*math.log10(max(v,1e-12))
def wav_to_stereo(path, output):
 with wave.open(str(path),"rb") as w:
  sr=w.getframerate();ch=w.getnchannels();sw=w.getsampwidth();frames=w.getnframes()
  if ch not in (1,2) or sw!=2 or sr<32000 or frames/sr<25:raise ValueError("invalid duration/channels/bit depth")
  samples=array.array('h');samples.frombytes(w.readframes(frames))
  if sys.byteorder!="little":samples.byteswap()
  floats=array.array('f')
  for i in range(0,len(samples),ch):
   l=samples[i]/32768.0;r=samples[i+1]/32768.0 if ch==2 else l
   floats.extend((l,r))
  output.write_bytes(floats.tobytes())
  return sr,ch,frames,floats
def asfloat(path):
 a=array.array("f");a.frombytes(path.read_bytes());return a
results=[]; failures=[]
for label,filename in SOURCES.items():
 try:
  url=BASE+filename
  req=urllib.request.Request(url,headers={"User-Agent":"125A-Drum-Audio-QA/1"})
  with urllib.request.urlopen(req,timeout=45) as response:data=response.read()
  source=ROOT/filename;source.write_bytes(data)
  raw=ROOT/(label+".f32");sr,ch,frames,inp=wav_to_stereo(source,raw)
  if len(inp)<sr*50:print("NOTE: shorter than 50 sec",label,frames/sr)
  for module in ("NEUTRAL","PUNCH","BODY","TIGHT","FINISH","GLUE"):
   out=ROOT/(label+"_"+module+".f32")
   subprocess.run([sys.argv[1],str(raw),str(out),str(sr),module,"0" if module=="NEUTRAL" else "0.5"],check=True)
   audio=asfloat(out)
   if len(audio)!=len(inp) or not all(math.isfinite(x) for x in audio):raise ValueError("invalid DSP output")
   delta=db(rms(audio))-db(rms(inp))
   residue=rms([x-y for x,y in zip(audio,inp)])/max(1e-10,rms(inp))
   results.append(dict(genre=label,module=module,seconds=round(frames/sr,3),
                       source_channels=ch,source_sha256=hashlib.sha256(data).hexdigest(),
                       rms_delta_db=round(delta,4),relative_residual=round(residue,5)))
   out.unlink()
  raw.unlink()
 except Exception as exc:
  failures.append({"fixture":label,"reason":str(exc)})
  print("FIXTURE FAILED:",label,exc,flush=True)
with (ROOT/"metrics.csv").open("w",newline="") as f:
 writer=csv.DictWriter(f,fieldnames=["genre","module","seconds","source_channels","source_sha256","rms_delta_db","relative_residual"])
 writer.writeheader();writer.writerows(results)
(ROOT/"summary.json").write_text(json.dumps({"results":len(results),"failure":failures,"fixtures":list(SOURCES),"license":"CC BY-NC-SA 4.0; internal/non-commercial testing only","note":"MDBDrums sources are mono; duplicated to dual-channel for core render. Not evidence of stereo image preservation."},indent=2))
print("MEASUREMENTS",len(results),"FAILURES",len(failures),flush=True)
if failures or len(results)!=len(SOURCES)*6:sys.exit(1)
