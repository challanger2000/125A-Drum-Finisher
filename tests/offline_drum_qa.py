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
  # Multiple resonant drum-ring frequencies, identical real programme.
  # Render baseline only once; no artificial threshold-based pass claims.
  if label=="SpeedMetal":
   length=frames
   baseline_path=ROOT/"baseline_FINISH100.f32"
   subprocess.run([sys.argv[1],str(raw),str(baseline_path),str(sr),"FINISH","1"],check=True)
   baseline=asfloat(baseline_path)
   # Detect real programme transients from channel-averaged short-time energy.
   # 5ms windows, preceding 60ms median-like mean floor, 120ms refractory.
   hop=max(1,int(sr*0.005))
   energy=[]
   for start in range(0,length-hop,hop):
    energy.append(sum(0.5*(inp[2*i]**2+inp[2*i+1]**2)
                      for i in range(start,start+hop))/hop)
   candidates=[]
   for k in range(12,len(energy)-2):
    previous=sorted(energy[k-12:k])[6]
    if energy[k]>max(0.000015,previous*3.0) and energy[k]>=energy[k-1] and energy[k]>energy[k+1]:
     candidates.append((energy[k]/max(previous,1e-8),k*hop))
   candidates.sort(reverse=True)
   onset_indices=[]
   for _,sample in candidates:
    if sample<sr or sample>length-int(sr*0.5):continue
    if all(abs(sample-chosen)>=int(sr*0.12) for chosen in onset_indices):
     onset_indices.append(sample)
    if len(onset_indices)>=35:break
   onset_indices.sort()
   if len(onset_indices)<8:raise ValueError("Too few natural transients detected")
   print("NATURAL_ONSETS",len(onset_indices),flush=True)
   frequency_reports=[]
   for freq in (550.0,1200.0,2700.0,4100.0):
    altered=array.array("f",inp)
    for onset in onset_indices:
     for j in range(min(int(sr*0.32),length-onset)):
      env=math.exp(-j/(sr*0.11))
      tone=0.11*env*math.sin(2*math.pi*freq*j/sr)
      altered[2*(onset+j)]+=tone
      altered[2*(onset+j)+1]+=tone
    injected=ROOT/"injected_ring.f32"
    injected.write_bytes(altered.tobytes())
    processed=ROOT/"injected_FINISH100.f32"
    subprocess.run([sys.argv[1],str(injected),str(processed),str(sr),"FINISH","1"],check=True)
    wet=asfloat(processed)
    diagnostics=[]
    for onset in onset_indices:
     def delta_range(start_s,end_s):
      a=onset+int(sr*start_s);b=min(length,onset+int(sr*end_s))
      dry_sq=0.;wet_sq=0.
      for i in range(a,b):
       d=altered[2*i]-inp[2*i]
       w=wet[2*i]-baseline[2*i]
       dry_sq+=d*d;wet_sq+=w*w
      return round(10*math.log10(max(wet_sq,1e-20)/max(dry_sq,1e-20)),4)
     diagnostics.append({"attack_delta_db":delta_range(0.,0.015),
                         "ring_tail_delta_db":delta_range(0.04,0.22)})
    attack=sum(z["attack_delta_db"] for z in diagnostics)/len(diagnostics)
    tail=sum(z["ring_tail_delta_db"] for z in diagnostics)/len(diagnostics)
    frequency_reports.append({"frequency_hz":freq,"events":len(diagnostics),
      "average_attack_delta_db":round(attack,4),"average_tail_delta_db":round(tail,4),
      "per_event":diagnostics})
    print("INJECTED_RING",freq,"events",len(diagnostics),"attack",round(attack,4),
          "tail",round(tail,4),flush=True)
    injected.unlink();processed.unlink()
   baseline_path.unlink()
   (ROOT/"injected_resonance_multiband_report.json").write_text(json.dumps({
    "source":"SpeedMetal mono duplicated to stereo",
    "test_frequencies_hz":[550,1200,2700,4100],
    "method":"wet injection-minus-wet baseline relative to dry injection",
    "caveat":"Broadband and transient distortion may influence these metrics. This is not a psychoacoustic discrimination test.",
    "reports":frequency_reports},indent=2))
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
