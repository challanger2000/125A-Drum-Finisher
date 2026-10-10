> **Archived experimental DSP iteration.** The attack/release numbers and 'known outstanding' lines below describe an older prototype and must **not** override the current `src/DrumCore.h` or `QA-EVIDENCE.md`. Current calibrated MASS, GLUE and TIGHT results live under `QA/` and Audio QA #43.

# Drum DSP module separation — experimental revision

Based on the user's 44.1kHz Stereo Drum loop and isolated module measurement:
- Previous PUNCH and BODY both largely modified the lowpass-115 Hz signal.
- Previous GLUE reduced RMS by just ~0.14 dB at 50%.
- FINISH is the adapted Bass Final resonance detector, independently validated by the new synthetic resonance test.

This revision:
- PUNCH: transient-dependent broad attack gain, bounded at <= 1.8x at 100%; intentionally distinct from tonal BODY.
- BODY: 110–350 Hz low-mid band difference added mainly into sustain, avoiding direct sub-bass stacking.
- GLUE: real stereo-linked feed-forward, soft-knee transfer; ratio 1:1 to 3.5:1. RMS-relative threshold adapting over 350 ms (0.065–0.35 linear), with detector attack 2 ms/release 150 ms and gain attack 5 ms/release 180 ms. Target coefficients updated every 16 samples or on GLUE parameter change to reduce expensive transcendental calls. No unclaimed Auto-Level.
- FINISH: adaptive Bass-derived resonance reduction remains after PUNCH/BODY/TIGHT with linked stereo analysis.

All coefficients are EMPIRICALLY TUNED in this stage, not calibrated against a physical drum processor. This does not constitute a professional sonic sign-off. New baseline comparisons must be measured and level-matched against user recordings. Do not overwrite the previous baseline without rationale.

Known outstanding: actual DAW editor lifecycle, 125A Plugin Tester full suite, full-programme sonic A/B and p99 CPU monitoring. The design must pass those gates before final.
