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
  # Controlled 2.7 kHz snare-like ringing injected into actual drum programme.
  # Compare wet versions with and without injected ringing; no musical-quality
  # claims from this diagnostic alone.
  if label=="SpeedMetal":
   freq=2700.0; length=frames
   altered=array.array("f",inp)
   onset_indices=list(range(sr,length-sr//2,sr))
   for onset in onset_indices:
    for j in range(min(int(sr*0.32),length-onset)):
     envelope=math.exp(-j/(sr*0.11))
     tone=0.11*envelope*math.sin(2*math.pi*freq*j/sr)
     altered[2*(onset+j)]+=tone
     altered[2*(onset+j)+1]+=tone
   injected=ROOT/"injected_ring.f32";injected.write_bytes(altered.tobytes())
   fixed=ROOT/"baseline_FINISH100.f32"
   attacked=ROOT/"injected_FINISH100.f32"
   subprocess.run([sys.argv[1],str(raw),str(fixed),str(sr),"FINISH","1"],check=True)
   subprocess.run([sys.argv[1],str(injected),str(attacked),str(sr),"FINISH","1"],check=True)
   baseline=asfloat(fixed);treated=asfloat(attacked)
   def window_energy(data,start,stop):
    return math.sqrt(sum(data[2*i]**2 for i in range(start,stop))/max(1,stop-start))
   diagnostics=[]
   for onset in onset_indices:
    def delta_range(start_s,end_s):
     a=onset+int(sr*start_s);b=min(length,onset+int(sr*end_s))
     dry=array.array("f",(altered[2*i]-inp[2*i] for i in range(a,b)))
     wet=array.array("f",(treated[2*i]-baseline[2*i] for i in range(a,b)))
     return round(db(rms(wet))-db(rms(dry)),4)
    diagnostics.append({"attack_delta_db":delta_range(0.0,0.015),
                        "ring_tail_delta_db":delta_range(0.04,0.22)})
   (ROOT/"injected_resonance_report.json").write_text(json.dumps({
    "source":"SpeedMetal mono duplicated to stereo",
    "injection_hz":freq,"injection_count":len(onset_indices),
    "method":"wet injection-minus-wet baseline relative to dry injection",
    "events":diagnostics,
    "average_attack_delta_db":round(sum(x["attack_delta_db"] for x in diagnostics)/len(diagnostics),4),
    "average_tail_delta_db":round(sum(x["ring_tail_delta_db"] for x in diagnostics)/len(diagnostics),4)},indent=2))
   print("INJECTED_RING",len(diagnostics),"attack",round(sum(x["attack_delta_db"] for x in diagnostics)/len(diagnostics),4),
       "tail",round(sum(x["ring_tail_delta_db"] for x in diagnostics)/len(diagnostics),4),flush=True)
   for item in (injected,fixed,attacked):item.unlink()
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
