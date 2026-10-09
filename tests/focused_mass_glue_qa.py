#!/usr/bin/env python3
"""Controlled MASS/GLUE diagnostics; measurement only, not a sonic PASS."""
import array, csv, math, pathlib, subprocess, sys
FS=48000
ROOT=pathlib.Path("audio-qa-output")
ROOT.mkdir(exist_ok=True)
RENDERER=sys.argv[1]
N=FS*3
def db(x): return 20.0*math.log10(max(x,1e-14))
def rms(seq): return math.sqrt(sum(float(x)*x for x in seq)/max(1,len(seq)))
def render(left,right,module,amount,tag):
    src=ROOT/("focused_%s_in.f32"%tag)
    dst=ROOT/("focused_%s_out.f32"%tag)
    samples=array.array("f")
    for a,b in zip(left,right): samples.extend((a,b))
    src.write_bytes(samples.tobytes())
    subprocess.run([RENDERER,str(src),str(dst),str(FS),module,str(amount)],check=True)
    out=array.array("f")
    out.frombytes(dst.read_bytes())
    src.unlink();dst.unlink()
    if len(out)!=2*len(left) or not all(math.isfinite(x) for x in out):
        raise RuntimeError("invalid renderer output")
    return out[::2],out[1::2]
def sine_projection(x,hz,start=1.,stop=2.5):
    a=int(start*FS);b=int(stop*FS);co=si=0.
    for i in range(a,b):
        phase=2*math.pi*hz*i/FS
        co+=x[i]*math.cos(phase);si+=x[i]*math.sin(phase)
    return 2*math.hypot(co,si)/(b-a)
def window_rms(x,start_ms,hit=2,width_ms=8):
    a=int((hit*.5+start_ms/1000.)*FS);b=a+int(width_ms*FS/1000.)
    return rms(x[a:b])
# Input peaks are bounded below full scale even for the +6 dB step.
pulse=[]
for i in range(N):
    t=(i%int(.5*FS))/FS
    env=.006+.18*math.exp(-t/.04)
    pulse.append(env*(.72*math.sin(2*math.pi*110*i/FS)+.28*math.sin(2*math.pi*180*i/FS)))
# Separate coherent two-tone source for harmonic/IMD probes.
tone=[.065*math.sin(2*math.pi*120*i/FS)+.055*math.sin(2*math.pi*230*i/FS) for i in range(N)]
rows=[]
levels=(-18,-12,-6,0,6)
for module in ("MASS","GLUE"):
    for amount in (0.,.25,.5,.75,1.):
        for level in levels:
            scale=10**(level/20.)
            original=tone if module=="MASS" else pulse
            left=[v*scale for v in original]
            right=[v*scale for v in original]
            wl,wr=render(left,right,module,amount,"%s_%d_%d"%(module,int(amount*100),level))
            out_rms=rms(wl);dry_rms=rms(left)
            result=dict(module=module,amount=amount,input_relative_db=level,
                        rms_change_db=round(db(out_rms/dry_rms),5),
                        max_abs_output=round(max(abs(v) for v in wl),6),
                        stereo_max_delta=round(max(abs(a-b) for a,b in zip(wl,wr)),9))
            if module=="MASS":
                fundamental=sum(sine_projection(wl,f)**2 for f in (120,230))
                harmonics=sum(sine_projection(wl,f)**2 for f in (240,360,460,690))
                intermod=sum(sine_projection(wl,f)**2 for f in (110,350,10))
                low_in=sum(sine_projection(left,f)**2 for f in (120,230))
                result.update(harmonics_to_fundamentals_db=round(10*math.log10(max(harmonics,1e-24)/max(fundamental,1e-24)),4),
                              imd_to_fundamentals_db=round(10*math.log10(max(intermod,1e-24)/max(fundamental,1e-24)),4),
                              fundamentals_gain_db=round(10*math.log10(max(fundamental,1e-24)/max(low_in,1e-24)),4))
            else:
                for ms in (12,50,200,350):
                    result["gain_%dms_db"%ms]=round(db(window_rms(wl,ms)/window_rms(left,ms)),5)
                result["crest_change_db"]=round(db(max(abs(v) for v in wl)/out_rms)-db(max(abs(v) for v in left)/dry_rms),5)
            rows.append(result)
keys=list(dict.fromkeys(k for row in rows for k in row))
with (ROOT/"focused_mass_glue_level_matrix.csv").open("w",newline="") as f:
    writer=csv.DictWriter(f,fieldnames=keys);writer.writeheader();writer.writerows(rows)
print("FOCUSED_MASS_GLUE_LEVEL_MATRIX",len(rows),"MEASURED (not sonic PASS)",flush=True)
