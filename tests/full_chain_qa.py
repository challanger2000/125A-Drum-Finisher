#!/usr/bin/env python3
"""Full-chain stereo combination regression: no musical quality claim."""
import array,math,pathlib,subprocess,sys
FS=48000
ROOT=pathlib.Path("audio-qa-output")
ROOT.mkdir(exist_ok=True)
RENDERER=sys.argv[1]
FRAMES=FS*3
def render(left,right,mode,amount):
    src=ROOT/"combined_in.f32"
    out=ROOT/"combined_out.f32"
    values=array.array("f")
    for l,r in zip(left,right):values.extend((l,r))
    src.write_bytes(values.tobytes())
    subprocess.run([RENDERER,str(src),str(out),str(FS),mode,str(amount)],check=True)
    result=array.array("f");result.frombytes(out.read_bytes())
    src.unlink();out.unlink()
    return result[::2],result[1::2]
def rms(v):return math.sqrt(sum(float(x)*x for x in v)/len(v))
def db(r):return 20*math.log10(max(r,1e-14))
# Independent asymmetric stereo with repeated kick, snare and high-frequency
# transients, not the old duplicated-mono test fixture.
L=[];R=[]
for i in range(FRAMES):
    t=i/FS
    kick=(t%0.50);snare=(t-0.25)%0.50;hats=t%0.125
    L.append(0.28*math.sin(2*math.pi*76*t)*math.exp(-kick*22)
             +0.18*math.sin(2*math.pi*193*t)*math.exp(-snare*45)
             +0.055*math.sin(2*math.pi*7100*t)*math.exp(-hats*220))
    R.append(0.16*math.sin(2*math.pi*91*t)*math.exp(-kick*20)
             -0.20*math.sin(2*math.pi*231*t)*math.exp(-snare*40)
             +0.08*math.sin(2*math.pi*5700*t)*math.exp(-hats*180))
dry=rms(L+R)
for character in ("ALL","ALL_TIGHT","ALL_DENSE"):
    neutralL,neutralR=render(L,R,character,0.0)
    if neutralL!=array.array('f',L) or neutralR!=array.array('f',R):
        # Input raw is quantized to float; allow rounding to float32.
        if max(abs(a-b) for a,b in zip(neutralL,L))>3.e-8:
            raise AssertionError("full-chain 0% is not neutral")
    previous=None
    for amount in (0.25,0.5,0.75,1.0):
        wetL,wetR=render(L,R,character,amount)
        if any(not math.isfinite(v) for v in wetL+wetR):
            raise AssertionError("combined non-finite output")
        value=db(rms(wetL+wetR)/dry)
        peak=max(max(abs(x) for x in wetL),max(abs(x) for x in wetR))
        if peak>=1.0: raise AssertionError("combined chain clipped an in-range stereo drum test")
        if previous is not None and abs(value-previous)>6.0:
            raise AssertionError("combined-chain gain step too large")
        previous=value
        print("COMBINED",character,amount,"rms_delta_dB",round(value,4),
              "peak",round(peak,6),flush=True)
print("COMBINED_STEREO_CHAIN REGRESSION PASS; DAW and listening verification separate",flush=True)
