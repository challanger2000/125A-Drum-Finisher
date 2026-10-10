# MASS residual gain experiment — 2026-10-10

**Status: implemented on engineering branch; target 2x full wet residual, NOT release signoff.**

## Objective and method

The existing MASS algorithm is two stable low/mid filter differences plus a nonlinear odd-harmonic residual, both multiplied by a gain-relative sustain detector. The effect added to the dry signal is a parallel, signed audio residual; it is **not** the dry output gain. Its gain can be scaled directly without changing the filter topology or stereo routing.

Simulated intensity factors 1x, 1.5x, 2x, 3x, 4x using the same baseline render with `y_k[n] = x[n] + k*(y_1[n] - x[n])`. This is mathematically equivalent to scaling the existing parallel MASS contribution when other modules are 0% and the input stays unchanged. Side effects were measured after RMS matching, not inferred from louder playback.

**Inputs:** one user-supplied 16s 44.1 kHz stereo drum loop `ohne(6).flac` (kept local/private, not committed) and four licensed noncommercial MusicDelta drum-only fixtures in the existing Audio-QA workflow (dual mono). Never publish or redistribute user-provided audio without approval.

## User-recording results (100% MASS, PUNCH character)

| Signed residual factor | 35–200 Hz versus DRY, RMS matched | 1200–5000 Hz versus DRY, RMS matched | 5000–14000 Hz versus DRY, RMS matched | Stereo side/mid width change | Raw total RMS delta |
|---|---:|---:|---:|---:|---:|
| 1x | +0.42 dB | -1.13 dB | -1.17 dB | -0.84 dB | +1.28 dB |
| 1.5x | +0.57 dB | -1.68 dB | -1.74 dB | -1.20 dB | +1.90 dB |
| **2x** | **+0.70 dB** | **-2.19 dB** | **-2.29 dB** | **-1.53 dB** | **+2.50 dB** |
| 3x | +0.90 dB | -3.15 dB | -3.31 dB | -2.07 dB | +3.63 dB |
| 4x | +1.04 dB | -4.00 dB | -4.23 dB | -2.50 dB | +4.67 dB |

The 2x raw waveform peaks at about 0.571 FS (no sample clipping) on this file. Its matched bass gain is *only* +0.70 dB, not +2.50 dB; the latter is the unnormalized full-band RMS increase. This distinction must remain clear when making product claims.

Level-matched low-band gain at 2x on four other sources: SpeedMetal +0.49 dB, Grunge +0.49 dB, Disco +0.45 dB, Country +0.15 dB. A larger coefficient is **not proof of greater perceived mass** on all programme material.

## Engineering choice

At 100% use **2x** the previous signed residual; reject 3x/4x as default because spectral tilt and stereo narrowing grow faster than useful bass change. Preserve established processing, nonlinear residual shaping, preset/state IDs and 0% neutrality. Map amount b (0..1) to amplitude `b*(1+b)` before existing `characterBody`, so 25% retains useful subtlety; 50% produces 0.75x of old full residual and 100% produces 2x old full residual. This is an **EMPIRICALLY TUNED** control curve supported by measurements; listening acceptance remains separate.

## Gates

- Stereo, in-place/out-of-place, finite outputs, sample-rate/partition determinism, 0% bit-exact.
- 75 Hz and 196 Hz 100% coherent sine magnitude expected about +5 dB, with explicit QA bands rather than old +3 dB gate.
- Check 25, 50, 75, 100% monotonicity and input gain robustness.
- Check actual 16s stereo A/B, full DSP audio-QA and exact-current Windows release QA before final release.
