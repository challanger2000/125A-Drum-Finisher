# 125A Drum Finisher V1 — current technical and programme-audio acceptance (2026-10-10)

**Status:** all available *automated* DSP, stereo and VST3 checks passed. **Studio One UI click/recall and human listening acceptance remain explicit external gates.** Do not claim the plugin is already user-accepted merely because validators pass.

## Immutable verified binary

- Exact Windows bundle from Full Release QA #9, run `38010699811`, commit `26d0b88435c588b26f8cc7101ec6a6f7ee26ab54`, SUCCESS: Steinberg both validation modes; 125A Plugin Tester 47 PASS / 0 WARNING / 0 FAIL; editor lifecycle checks.
- The later Audio QA #35 from commit `6ef9708d24a18f340b13a448774b4cf68bd5c703` adds **test-only** files and a renderer feature; source blobs for `src/DrumCore.h`, `src/dsp/AdaptiveResonanceSuppressor.cpp`, `src/vst/DrumPlugin.cpp`, and `resource/DrumFinisher.uidesc` have been verified identical to QA #9. Hence no additional Windows compilation is needed for that unchanged plugin binary.
- Full-chain Audio QA #35, run `38011717161`: PASS, including 0%, 25%, 50%, 75%, 100% with CHARACTER = PUNCH/TIGHT/DENSE on asymmetric stereo input.

## User-provided true stereo reference (not redistributed or committed)

Original: `ohne(6).flac`, 16 seconds, 44100 Hz, two *independently varying* channels, 24-bit PCM FLAC. All single-module offline renders use the current DSP at CHARACTER PUNCH, OUTPUT 0 dB, other modules 0%. Only measurable comparisons are reported.

| Module at 100% | RMS delta vs original | Crest delta | Side/Mid energy width delta |
|---|---:|---:|---:|
| PUNCH | +2.331 dB | +1.725 dB | -2.003 dB |
| MASS | +2.501 dB | -2.602 dB | -1.526 dB |
| TIGHT | -0.888 dB | +0.249 dB | -2.027 dB |
| FINISH | -0.067 dB | +0.067 dB | +0.032 dB |
| GLUE | -2.864 dB | +0.666 dB | -0.232 dB |

- MASS low-band 35–200 Hz *after matched global RMS* +0.683 dB; this is distinct from raw +2.501 dB broadband loudness change.
- GLUE stereo-linked gain reduction distribution (sample estimates avoiding near-zero division): p10 2.29 dB, median 3.30 dB, p90 4.03 dB. Changes in relative crest are **normal and permissible** for an attack-preserving programme compressor; they alone are not a reason to redesign the algorithm. The relative attack reduction is smaller than body reduction. 100% has legitimate strong but bounded compression and preserved stereo-linked gain.
- Dynamic-modules PUNCH/TIGHT/GLUE share a single instantaneous gain L/R; `wetL*dryR - wetR*dryL` remains at float-rounding scale (max approx 2e-8 for this reference). The average side/mid change is temporal weighting of *pre-existing* stereo information, **not independent left/right pan gain drift**.
- FINISH is intentionally near-neutral on this largely nonresonant programme: separate controlled resonance injections at 550/1200/2700/4100 Hz measured approx -3.3 to -3.72 dB tail reductions with protected early attack at 100%. Preserve this selectivity, do not simply turn it up on clean material.

All five modules rendered at {0%,25%,50%,75%,100%} from this identical stereo reference. Each gave finite output and controlled, smoothly progressing dynamics; no observed individual output sample clipping on this reference.

## Full-chain same-reference 100% verification

All five modules simultaneously at 100%, OUTPUT = 0 dB. Unlike adding independent processed files, this uses the **actual single Core** with all modules active and shared detector state, via the offline `ALL` renderer.

| CHARACTER | Integrated RMS delta | 4x-oversampled true-peak dBTP | Invalid samples |
|---|---:|---:|---:|
| PUNCH | +0.634 dB | -2.864 dBTP | 0 |
| TIGHT | -0.029 dB | -3.748 dBTP | 0 |
| DENSE | +0.694 dB | -3.654 dBTP | 0 |

At 0%, all modes remain exactly neutral; all amount steps 0/25/50/75/100 rendered without invalid samples or >0 dBFS clips for this source. True peaks use 4× polyphase oversampling as an *estimate*, not hardware-measured ISP.

## Pending non-automated product gate

- **Real Studio One mouse interaction:** click CHARACTER's each segment, click ON/BYPASS both directions, verify that state persists on project save/reopen, automation updates GUI, and all knob controls respect CTRL+click default. Tests validate XML tags and VST3Editor listener wiring but did not inject actual user clicks in Studio One.
- **Musical preference:** user listening acceptance of full plugin on desired content at level matched settings; signal processing metrics prove effect behaviour, not that every engineer prefers the coloration.
- No unnecessary additional DSP gain boosts merely because a module does not reduce crest factor. GLUE/FINISH are technically consistent with their design intent; further changes require a specific reproducible defect.

**Decision:** The existing QA #9 Windows binary is the correct single candidate for the user's final Studio One acceptance. Do not request repeated partial test builds. Until real-host UI/recall and listening acceptance are received, do not publish as fully product-approved.
