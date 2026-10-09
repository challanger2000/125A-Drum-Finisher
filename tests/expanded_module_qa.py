#!/usr/bin/env python3
"""Programme-level extended diagnostics; metrics are not listening-test PASSes."""
import array,csv,json,math,pathlib,subprocess,sys,wave
root=pathlib.Path("audio-qa-output")
renderer=sys.argv[1]
files={"SpeedMetal":"MusicDelta_SpeedMetal_Drum.wav","Grunge":"MusicDelta_Grunge_Drum.wav","Disco":"MusicDelta_Disco_Drum.wav","Country":"MusicDelta_Country1_Drum.wav"}
modules=("PUNCH","MASS","TIGHT","FINISH","GLUE")
def db(v):return 20*math.log10(max(v,1e-12))
def readwav(path):
 with wave.open(str(path),"rb") as f:
  fs=f.getframerate();ch=f.getnchannels();sw=f.getsampwidth()
  if sw!=2 or ch not in (1,2):raise ValueError("unsupported audio")
  a=array.array("h");a.frombytes(f.readframes(min(f.getnframes(),fs*8)))
  if sys.byteorder!="little":a.byteswap()
  samples=array.array("f")
  for i in range(0,len(a),ch):
   v=a[i]/32768.;w=a[i+1]/32768. if ch==2 else v
   samples.extend((v,w))
 return fs,samples
def mono(a):return [(a[i]+a[i+1])*0.5 for i in range(0,len(a),2)]
def rms(a):return math.sqrt(sum(x*x for x in a)/max(1,len(a)))
def crest(x):return db(max(abs(v) for v in x))-db(rms(x))
def envelope(x,fs):
 hop=max(1,int(fs*.01))
 return [rms(x[i:i+hop]) for i in range(0,len(x)-hop,hop)]
def transient(x,fs):
 e=envelope(x,fs)
 return sorted((e[i]/max(1e-7,sum(e[max(0,i-5):i])/max(1,min(5,i))) for i in range(5,len(e))),reverse=True)[:20]
def autocorr_short(x):
 a=x[::8]
 return sum(a[i]*a[i-1] for i in range(1,len(a)))/max(1e-12,sum(v*v for v in a))
def fft_power_bands(x,fs):
 # Hann-windowed radix-2 FFT with Parseval-consistent relative band powers.
 # Unlike sparse sinusoid projections, this integrates every spectral bin.
 n=4096
 if len(x)<n:return {key:0.0 for key in ("35_200","200_1200","1200_5000","5000_14000")}
 window=[.5-.5*math.cos(2*math.pi*i/(n-1)) for i in range(n)]
 bands=((35,200),(200,1200),(1200,5000),(5000,14000))
 sums={f"{lo}_{hi}":0.0 for lo,hi in bands}
 # Use four nonoverlapping windows spread across the first four seconds.
 for start in [int((len(x)-n)*k/4) for k in range(4)]:
  z=[complex(x[start+i]*window[i],0.) for i in range(n)]
  j=0
  for i in range(1,n):
   bit=n>>1
   while j&bit:
    j^=bit;bit>>=1
   j^=bit
   if i<j:z[i],z[j]=z[j],z[i]
  length=2
  while length<=n:
   phase=-2*math.pi/length
   tw=complex(math.cos(phase),math.sin(phase))
   for a in range(0,n,length):
    w=1+0j
    for k in range(length//2):
     u=z[a+k];v=z[a+k+length//2]*w
     z[a+k]=u+v;z[a+k+length//2]=u-v
     w*=tw
   length*=2
  for k in range(1,n//2):
   hz=k*fs/n
   for lo,hi in bands:
    if lo<=hz<hi:sums[f"{lo}_{hi}"]+=z[k].real*z[k].real+z[k].imag*z[k].imag
 return sums
rows=[]
for label,filename in files.items():
 fs,dry=readwav(root/filename);drym=mono(dry)
 inp=root/"expanded_input.f32";inp.write_bytes(dry.tobytes())
 for module in modules:
  for amount in (.25,.5,1.):
   out=root/"expanded_output.f32"
   subprocess.run([renderer,str(inp),str(out),str(fs),module,str(amount)],check=True)
   wet=array.array("f");wet.frombytes(out.read_bytes())
   if len(wet)!=len(dry) or not all(math.isfinite(x) for x in wet):raise ValueError("invalid output")
   gain=rms(dry)/max(1e-12,rms(wet))
   wetm=mono(array.array("f",(v*gain for v in wet)))
   dryenv=envelope(drym,fs);wetenv=envelope(wetm,fs)
   entdiff=[db(w)-db(d) for d,w in zip(dryenv,wetenv) if d>1e-5]
   rmsenv=math.sqrt(sum(d*d for d in entdiff)/max(1,len(entdiff)))
   peaks=[abs(v) for v in wet]
   bands={}
   pre_bands=fft_power_bands(drym,fs)
   post_bands=fft_power_bands(wetm,fs)
   for key in pre_bands:
    bands["band_"+key+"_db"]=round(10*math.log10(max(post_bands[key],1e-20)/max(pre_bands[key],1e-20)),3)
   rows.append(dict(source=label,module=module,amount=amount,
     rms_change_db=round(db(rms(wet))-db(rms(dry)),3),
     level_matched_crest_delta_db=round(crest(wetm)-crest(drym),3),
     envelope_rms_change_db=round(rmsenv,3),
     transient_p95_ratio_change=round((sorted(transient(wetm,fs))[int(.95*19)]/
             max(1e-9,sorted(transient(drym,fs))[int(.95*19)]))-1,4),
     lag1_corr_change=round(autocorr_short(wetm)-autocorr_short(drym),6),
     clipped_samples=sum(v>=1.0 for v in peaks),**bands))
  out.unlink()
 inp.unlink()
with (root/"expanded_metrics.csv").open("w",newline="") as f:
 w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
(root/"expanded_qa_coverage.json").write_text(json.dumps({
 "metrics":"RMS; RMS-matched crest; 10ms envelope displacement; transient peak ratio; lag-1 correlation; Hann-windowed broadband FFT band integration; clipping",
 "test_count":len(rows),"not_covered":"LUFS/true peak; phase/group delay; THD/IMD/aliasing; true stereo imaging; perceptual listening; realtime CPU; sample-rate sweep",
 "qualification":"FFT band analysis integrates the full defined band; frequency resolution varies with sample rate. Mono source duplicated into stereo. These are diagnostics, not sonic PASS criteria."},indent=2))
print("EXPANDED_METRICS",len(rows),"COMPLETE",flush=True)
