# MASS V1 — Programme-level FFT evaluation (2026-10-09)

Status: **MEASURED — sonic acceptance NOT YET VERIFIED**. Baseline: branch commit `2d3821084a0cb5e08aff7492f25d1bb627d630f3`. Source: successful [Audio QA #21](https://github.com/challanger2000/125A-Drum-Finisher/actions/runs/37954082010), artifact `125A-Drum-Finisher-Audio-QA`, `expanded_metrics.csv`. Evaluated from the actual uploaded CI artifact, not from the green job badge.

The latest QA replaces the obsolete nine-frequency sparse estimate with four Hann-windowed 4096-point FFT windows and integrates **all** bins in the named frequency bands. Results below are **RMS-level matched wet versus dry**. They do not prove subjective improvement, and the source material in this regression has a mono drum channel duplicated into L/R rather than independently recorded stereo buses.

| Dry material | MASS 100%: 35–200 Hz | 200–1200 Hz | 1200–5000 Hz | 5000–14000 Hz | 10-ms envelope displacement | Level-matched crest change |
|---|---:|---:|---:|---:|---:|---:|
| SpeedMetal | +0.108 dB | +0.257 dB | −0.327 dB | −0.357 dB | 0.427 dB | −0.089 dB |
| Grunge | +0.178 dB | −0.186 dB | −0.379 dB | −0.404 dB | 0.391 dB | +0.261 dB |
| Disco | +1.016 dB | +0.283 dB | −0.228 dB | −0.280 dB | 0.309 dB | −0.215 dB |
| Country | +0.075 dB | −0.164 dB | −0.259 dB | −0.275 dB | 0.252 dB | −0.200 dB |

## Interpretation and engineering decision

1. No generalized **stronger punch / mass** claim is justified. MASS causes material-dependent changes, with comparatively weak full-band low-frequency gains on three of four fixtures.
2. The high-frequency attenuation in the **level-matched** analysis is partly an unavoidable consequence of the louder raw wet output being normalized to match overall dry RMS. It is not evidence for direct high-band damping by MASS.
3. Spectral change is not synonymous with improvement. Measure kick fundamental and harmonics, low-mid masking, active transient/sustain windows, full-band level-matched deltas and clipping against frozen dry at every amount.
4. Do not arbitrarily increase MASS scaling to satisfy a guessed decibel target. Derive a useful definition of perceptual body from real use cases; validate at 0/25/50/75/100% with level-matched A/B and real stereo drum buses.
5. Only amend the DSP after identifying a concrete, repeatable shortcoming and a defensible correction. Do not re-run full Windows CI for this research-only report.

## Outstanding release gates

- **NOT YET VERIFIED:** musical value at normal 20–50% settings and at maximum, against actual full stereo drum buses.
- **NOT YET VERIFIED:** level-matched kick-band / sustain / transient before/after on independent stereo test cases; FFT analysis above is a useful partial result only.
- **NOT YET VERIFIED:** realtime p95/p99/max and interactive Studio One control and automation exercise on the changed branch.
- 125A Full Release QA #4 is PASS, but predates the latest low-kick MASS DSP change; a final release validation must use the exact shipping SHA.
