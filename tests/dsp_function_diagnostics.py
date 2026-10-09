#!/usr/bin/env python3
"""Deterministic DSP function diagnostics. Measurements, not music-quality verdicts."""
import array, csv, math, pathlib, subprocess, sys
fs=48000; duration=3; n=duration*fs; root=pathlib.Path("audio-qa-output");root.mkdir(exist_ok=True)
renderer=sys.argv[1]
def rms(x):return math.sqrt(sum(v*v for v in x)/max(1,len(x)))
def db(v):return 20*math.log10(max(v,1e-14))
def render(source,module,amount):
 raw=root/"function_input.f32";out=root/"function_output.f32"
 raw.write_bytes(array.array("f",(a for v in source for a in (v,v))).tobytes())
 subprocess.run([renderer,str(raw),str(out),str(fs),module,str(amount)],check=True)
 a=array.array("f");a.frombytes(out.read_bytes())
 if len(a)!=2*len(source) or not all(math.isfinite(v) for v in a):raise RuntimeError("invalid render")
 raw.unlink();out.unlink()
 return a[0::2]
def component(x,f,start=1.0,end=2.5):
 a=int(fs*start);b=min(int(fs*end),len(x))
 c=s=0.
 for i in range(a,b):
  ang=2*math.pi*f*i/fs;c+=x[i]*math.cos(ang);s+=x[i]*math.sin(ang)
 return math.hypot(c,s)*2/max(1,b-a)
# Two-tone band-limited programme and test signal. Measure the nonlinear
# harmonics outside the input partials; harmonics are not automatically desirable.
tones=[(125.0,.14),(230.0,.12)]
source=[sum(amp*math.sin(2*math.pi*f*i/fs) for f,amp in tones) for i in range(n)]
rows=[]
for amount in (.25,.5,1.):
 wet=render(source,"MASS",amount)
 fundamental=sum(component(wet,f)**2 for f,_ in tones)
 extra=sum(component(wet,f)**2 for f in (250.,375.,460.,690.,920.))
 original_extra=sum(component(source,f)**2 for f in (250.,375.,460.,690.,920.))
 rows.append(dict(test="mass_harmonics",setting=amount,
   metric="extra_components_over_fundamental_db",
   measured=round(10*math.log10(max(extra,1e-20)/max(fundamental,1e-20)),3),
   dry_reference=round(10*math.log10(max(original_extra,1e-20)/
      max(sum(component(source,f)**2 for f,_ in tones),1e-20)),3)))
# A drum-like stepped crest stimulus with independent repeated decays.
# Diagnose compression trajectories at attack, 50ms and 200ms in each hit,
# and check a sustained passage for stable reduction.
stim=[]
for i in range(n):
 phase=i%int(.5*fs)
 env=.018+.45*math.exp(-phase/(.035*fs))
 stim.append(env*math.sin(2*math.pi*105*i/fs))
for amount in (.25,.5,1.):
 wet=render(stim,"GLUE",amount)
 for millis in (12,50,200,350):
  values=[]
  for hit in range(2,6):
   a=int((hit*.5+millis/1000.)*fs)
   b=a+int(.008*fs)
   values.append(db(rms(wet[a:b]))-db(rms(stim[a:b])))
  rows.append(dict(test="glue_hit_envelope",setting=amount,
    metric="delta_at_%d_ms_db"%millis,
    measured=round(sum(values)/len(values),4),dry_reference=0.))
with (root/"dsp_function_diagnostics.csv").open("w",newline="") as f:
 w=csv.DictWriter(f,fieldnames=("test","setting","metric","measured","dry_reference"));w.writeheader();w.writerows(rows)
print("DSP_FUNCTION_DIAGNOSTICS",len(rows),"complete",flush=True)
