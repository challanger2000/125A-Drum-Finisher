> **Archived prototype iteration, not current DSP parameters.** The values and known-open items below were superseded by later measured PUNCH/MASS/TIGHT/FINISH/GLUE revisions. The current verified DSP implementation is `src/DrumCore.h`, with current test evidence in the dated files under `QA/` and Audio QA #43.

# Module-level calibration notes — V1

Build #6 demonstrated at 50%:
- GLUE applied about -2.12 dB RMS on the isolated kick fixture and reduced attack more than tail.
- FINISH reduced kick attack/tail contrast by about 0.57 dB.
- BODY boosted kick level by about 0.72 dB, without changing envelope contrast.

This revision is *experimental*:
- BODY's low-band gain is reduced and its one-pole low-band center moved from 150 Hz to 115 Hz.
- FINISH uses an onset-protected, reduced parallel nonlinearity.
- GLUE replaces the immediate peak-dependent attenuator with a linked, slow envelope-based bounded attenuation, and attack-protection weighting. This is **not** proven auto-level, nor a finalized compressor design.
- Retain the exact pre-revision characterization output as a baseline. Compare the next fixture measurements with #6 rather than claiming improvements from a successful compilation alone.
- Before release: test level-matched real kits, attack/sustain differences, aliasing, steady-signal behavior, automation discontinuities, and CPU distribution. The unused glue detector from prior prototype remains to be removed after architecture review.
