# TIGHT preserves drum size while shortening decay — 2026-10-10

**Status:** offline DSP QA #40 SUCCESS. This report documents the exact committed DSP code, commit `815c65aa4d80976e0ba21ecb33c46906fb57f4c9`. A Windows VST3 and user's Studio One acceptance are separate.

## Problem

User: TIGHT sounds much smaller than PUNCH and DENSE with all five processing amounts 50%, OUTPUT 0dB. The user's four aligned, independent stereo FLAC files (16s, 44.1 kHz, 24-bit) confirmed:

| 50% CHARACTER export | Attack 0–20ms | Body 20–85ms | Tail 85–180ms | Global RMS |
|---|---:|---:|---:|---:|
| PUNCH previous | +1.42 dB | -0.17 dB | -0.07 dB | +0.84 dB |
| TIGHT previous | **-1.13 dB** | **-3.02 dB** | **-3.09 dB** | **-1.95 dB** |
| DENSE previous | +1.57 dB | +0.78 dB | +0.33 dB | +1.07 dB |

dB values reference **unaltered DRY**; 59 input transient onsets selected with a fixed 4ms RMS/65–9000Hz detector, 100ms minimum event spacing and consistent prominence.

Cause is not only TIGHT's *intentional* linked envelope shortening. Its broad -3.5dB low shelf at maximum character intensity attenuated the whole bus, including early kick foundation. Simply removing that low shelf initially restored level but failed the established **CHARACTER contrast** control test (Audio QA #39, 0.033678 matched residual versus 0.075 threshold).

## Correction and method

TIGHT now retains the low-band foundation (0dB lower shelf) and expresses its original **relative spectral tilt** by the reciprocal **+3.5dB high shelf** at maximum color amount. On the steady linear frequency response, `(low -3.5 dB, high 0 dB)` and `(low 0 dB, high +3.5 dB)` differ only in absolute gain but preserve the intended low/high tonal contrast. At 50% this shifts the overall level by about 1.75dB; stronger onset (TIGHT punch coefficient 0.65 → 1.00) and preserved MASS coefficient (TIGHT 0.50 → 0.65, equal to PUNCH) retain attack/body rather than simply applying a blind output gain. The linked TIGHT decay contour and max reduction remain unchanged. Existing smoothing of tonal parameters remains intact. PUNCH and DENSE algorithm coefficients are unchanged.

## Render and metrics

Use the **actual Linux offline DrumCore** compiled by Audio QA #40, SHA256 of renderer `734f9c6e36818c9b289190a1644877eea144da3ec5897f0e066f6b7b17b9eff8` and `ALL_TIGHT 0.5` applied to the original `ohne(7).flac`, interleaved float32 at 44100Hz, with all five FX controls 50%, OUTPUT 0dB.

| Source | Global RMS vs DRY | Attack vs DRY | Body vs DRY | Tail vs DRY | True peak estimated 4x |
|---|---:|---:|---:|---:|---:|
| DRY | 0dB | 0dB | 0dB | 0dB | -4.50 dBTP |
| PUNCH previous Studio One | +0.837 | +1.419 | -0.172 | -0.072 | -2.78 |
| TIGHT previous Studio One | -1.946 | -1.130 | -3.019 | -3.085 | -4.53 |
| **TIGHT corrected offline** | **+0.353** | **+1.248** | **-0.884** | **-0.872** | **-2.06** |
| DENSE previous Studio One | +1.067 | +1.568 | +0.777 | +0.334 | -4.34 |

Corrected TIGHT is ~0.49dB quieter overall than PUNCH vs previous ~2.78dB; it retains a ~2.12dB differential between the early attack and late tail, i.e. it is **still transient-tightening, not simply a global level boost**. No 0dBFS samples or inferred 4x intersample peaks on this programme. The PUNCH and DENSE offline variants reproduce the earlier user exports within approximately 0.02dB of their global and phase-segment RMS values.

Raw band comparison TIGHT old → new vs DRY:
- 35–90Hz: -1.80 → +0.55dB
- 90–180Hz: -1.27 → +1.21dB
- 180–400Hz: -1.75 → +0.64dB
- 400–1200Hz: -2.34 → -0.05dB
- 1200–5000Hz: -2.83 → -0.74dB
- 5000–12000Hz: -3.64 → -1.67dB

## Regression and limitations

- Audio QA #40 (`38020311605`): complete PASS for precision, function, stereo, 0–100% combined chain, audible CHARACTER contrast on controlled independent stereo, GLUE/MASS diagnostics and licensed MusicDelta drum fixtures.
- Audio QA #39 failed CHARACTER contrast because the initial variant *removed* TIGHT tilt entirely. This correction intentionally retains spectral distinction.
- These figures establish that this single user reference no longer has the broad *shrinking* effect. They do **not** establish universal quality across every drum recording; no subjective Studio One listening approval yet.
- Do not commit or redistribute any source `ohne(7).flac`, `Punch(1).flac`, `Dense.flac`, `Tight.flac` to GitHub. Preserve existing host parameter IDs and state version.
