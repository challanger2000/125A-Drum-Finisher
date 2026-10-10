# Studio One CHARACTER A/B — 2026-10-10

**Evidence:** Four user-supplied 16.000s FLAC exports (`ohne(7).flac`, `Punch(1).flac`, `Tight.flac`, `Dense.flac`) from Studio One, 44.1kHz, 24-bit stereo; companion screenshot shows PUNCH/MASS/TIGHT/FINISH/GLUE each 50%, OUTPUT 0.0 dB and DENSE selected. Audio remains user-private and is NOT committed.

## GUI visual acceptance
New passive engraved faceplate is visible: six separate module bays, screw details, centered CHARACTER group, header BYPASS beneath zoom. The GUI now shows *dynamic* `50 %` and `0.0 dB` below knobs with clean name-only headers. DENSE is visibly lit on the screenshot, proving at least that its selected-state rendering works; click/automation/project-restore functionality are not fully evidenced by a screenshot alone.

## Audio results
All four sources share the same duration/sample rate and are time-aligned (no offset).
A/B signal metrics:

| Mode | Full-band RMS dBFS | Relative to DRY | Sample peak dBFS | Crest dB | Side/Mid width delta vs DRY |
|---|---:|---:|---:|---:|---:|
| DRY | -26.704 | 0.000 | -4.762 | 21.942 | 0.000 |
| PUNCH | -25.868 | +0.837 | -3.031 | 22.837 | -2.302 |
| TIGHT | -28.650 | -1.946 | -4.827 | 23.823 | -2.063 |
| DENSE | -25.637 | +1.067 | -4.341 | 21.296 | -2.510 |

4x polyphase interpolated peak estimate: DRY -4.50 dBTP, PUNCH -2.78 dBTP, TIGHT -4.53 dBTP, DENSE -4.34 dBTP. No clipping on these files.

All spectral comparisons below **match global RMS first** to avoid mistaking level difference for musical timbre; integration uses Welch 8192-sample stereo-channel PSD at 44.1kHz:

| Comparison (first versus second) | 25–60 Hz | 1–5 kHz | 5–12 kHz | 12–20 kHz |
|---|---:|---:|---:|---:|
| PUNCH minus DENSE | -0.57 dB | +0.76 dB | +0.63 dB | +1.27 dB |
| TIGHT minus DENSE | -0.70 dB | +1.47 dB | +1.15 dB | +2.50 dB |
| PUNCH minus TIGHT | +0.13 dB | -0.70 dB | -0.52 dB | -1.23 dB |

Level-matched normalized correlations: PUNCH–TIGHT 0.99595, PUNCH–DENSE 0.99613, TIGHT–DENSE 0.98800. Thus DENSE vs PUNCH is still **relatively subtle** at the user's chosen 50% settings; the previously reported ~4.4dB presence contrast on a separate 100% test must NOT be ascribed to these new files. TIGHT differs more from DENSE but is also ~3.01 dB quieter in raw playback, severely confounding ungain-matched subjective A/B.

Each mode narrows *average* side/mid level by ~2.1–2.5 dB on this programme despite linked instantaneous stereo dynamics; that is a temporal/frequency weighting consequence, not proof of independently drifting channel gain.

## Decision
User prefers PUNCH with all five controls at 50%; keep that as the musical reference, **not 100%-all**. Do not claim all three CHARACTER voicings are conclusively musically distinct or equally loud based on old artificial-signal tests. Do not hard-code a 3dB TIGHT gain boost to fit only this recording. To evaluate any automatic makeup, validate stable loudness on more independent drum fixtures and retention of transient intent. Do not modify DSP until a reproducible quality issue and a suitable corrective design are confirmed.

**Release status:** GUI appearance improved materially and numerical unit labels displayed properly; audible/level-matched CHARACTER difference is present, with notable inter-mode loudness mismatch requiring consideration. Screenshot alone does not close DAW click/recall acceptance.
