# MASS / GLUE audio QA decision record — 2026-10-09

## Evidence
- Audio-QA #16 and #17: SUCCESS (measurement and assertions, not sonic approval).
- Windows build #50: SUCCESS including compiler, contract tests, Steinberg validator.
- Evaluated artifact: `125A-Drum-Finisher-Audio-QA`, run #16; CSV `focused_mass_glue_level_matrix.csv`.
- Source: `tests/focused_mass_glue_qa.py` at 48 kHz; 3-second synthetic mono signals duplicated to stereo. Do not label this representative stereo programme.

## Key measurements
At MASS = 100% across -18/-12/-6/0/+6 dB stimulus gain:
- Output RMS delta: +0.63917/+0.99583/+1.38630/+1.72576/+1.93504 dB.
- IMD-to-fundamentals: -54.9293/-46.5323/-40.2357/-36.2822/-35.4980 dB.

At GLUE = 100% across the same levels:
- Output RMS delta: -1.24404 dB at each level.
- 12-ms hit-window gain: -1.16022 dB at each level.

These are empirical measurements of the current processing and synthetic fixture, **not** proof of musical utility.

## Interpretation
- GLUE's programme-relative threshold demonstrates input-level invariance on the tested scaled fixture. Maintain its behavior until programme-level evidence identifies a flaw; do not increase compression purely to make the numbers larger.
- MASS is intentionally nonlinear and frequency selective. Input-level dependence is expected from its saturating density term, but the scale of resulting harmonic/IMD products requires comparison against real drum stems.
- The MASS band in `DrumCore.h` is formed by the difference of approximately 350-Hz and 110-Hz one-pole lowpasses; this is a low-mid treatment and cannot be presented as independently demonstrated sub-bass enhancement.
- Present sparse sine projections as component probes, not full-spectrum IMD/THD or perceptual evaluation.

## Next acceptance gates
1. Freeze source and DRY baseline, obtain fixed **genuine stereo** drum-bus fixtures from multiple styles and document their provenance.
2. Render 0/25/50/75/100% per module at matched integrated loudness, retaining peaks and transient data. Report both absolute and matched-level differences.
3. For MASS, use whole-spectrum FFT/STFT, harmonic/IMD level sweeps and aliasing assessment at 44.1/48/96 kHz. Quantify low-mid energy, spectral balance, and kick/bass masking; do not equate RMS gain with density.
4. For GLUE, assess gain-reduction time trajectories, crest-factor shifts, stereo width/coherence and pumping on true stereo, including cymbals and room tails.
5. Review the 20–50% musical working range and combined PUNCH/MASS/TIGHT/GLUE interactions. Declare sonic PASS only after programme material and level-matched listening review.

## Decision
- **No DSP retune authorized by these measurements alone.**
- Build/automated QA PASS is separate from sonic approval.
- Avoid further CI-only commits until a coherent DSP/test change with measurable acceptance criteria is ready.
