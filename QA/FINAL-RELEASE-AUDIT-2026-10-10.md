# 125A Drum Finisher V1.0.0 — final technical release audit

**Date:** 2026-10-10  
**Repository:** `challanger2000/125A-Drum-Finisher`  
**Authoritative engineering branch:** `v1.0.0-engineering`; `main` not modified.  
**Examined exact binary:** Full Windows QA **#14**, `8ab575977aa6dfdd04614c957db5f4fd7bd8e6ed`, [GitHub Actions run](https://github.com/challanger2000/125A-Drum-Finisher/actions/runs/38022161131), **SUCCESS**.  
**Windows VST3 artifact:** `125A-Drum-Finisher-V1-Full-Release-QA`, ID 11658343952, ZIP SHA256 `0f5984b487b556e432434660b6b9b0151323b72f19bcdf2da51d8a7bcfbd2fa0`. Archive internally passes ZIP integrity test.  
**Audio QA:** #43, `38021945180`, **SUCCESS**. User audio was **not committed**.  
**Linked standards:** `challanger2000/125A-Engineering/START-HERE.md`, `STANDARDS/DSP-GUIDELINES.md`, `STANDARDS/PROFESSIONAL-PLUGIN-BEHAVIOR.md`, `QA/RELEASE-QA.md`.

## Executive decision

**TECHNICAL WINDOWS RELEASE AUDIT: PASS.** The exact tested build compiles, validators accept it, 125A Plugin Tester records 47 PASS / 0 WARNING / 0 FAIL, all current CTests and full programme-DSP regression pass, and the distributed artifact has been independently opened and checked.

**FULL COMMERCIAL/REAL-HOST ACCEPTANCE: CONDITIONAL.** Do not describe as thoroughly validated *inside Studio One for every gesture* until the four practical items at the bottom are confirmed. An automated editor lifecycle probe does not simulate all real user clicks.

## Independent code and contract inspection

- **Core and DSP:** Core uses separate stereo channel filter states with linked dynamics and shared coefficients, finite host-audio sanitization, explicit unit/default clamping, bounded filters with denormal floor, no audio-thread allocations or filesystem calls. One-pole exponentials and the fixed ten-band adaptive FINISH resonance detector use sample-rate-dependent preparation. FINISH deliberately makes little change on an unproblematic loop, but suppresses injected resonant tails.
- **Host wrapper:** one stereo in/out, 32-bit and 64-bit float audio, no unexpected event/MIDI bus, bounded queues applied at sample offsets, bypass crossfade continuing wet-state processing, output silence flags derived from actual samples, separate Processor and Controller state. Stable 8 ParamIDs (100–107), state schema version 1.
- **GUI:** verified the **exact packaged** VSTGUI `.uidesc`, 3 native CHARACTER CTextButtons, 1 header BYPASS CTextButton, 1 100/150% zoom selector, 6 dynamic readouts with '%' or 'dB' below clean module titles, passive full-canvas metal panel which cannot intercept mouse input, correct logo and knob resource inventory. Controller bindings match parameter IDs.
- **Plugin artifact:** moduleinfo identifies `125A Drum Finisher` v1.0.0; Windows x64 `Contents/x86_64-win/125A_Drum_Finisher.vst3` present. No extra duplicate VST3 bundle. Package represents exact validated run; no subsequent source mutation has changed the binary code. README/host QA history discrepancies were corrected as **documentation-only** changes.
- **Source pin:** Steinberg VST3 SDK `v3.8.1_build_84` at immutable `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`.

## Defects found and fixed during this audit

| ID | Severity | Genuine defect (not a cosmetic guess) | Correction and proof |
|---|---|---|---|
| A-01 | Significant host contract | MASS/FINISH produced output after zero-input transition, yet default `getTailSamples()` returned 0. | Sample-rate-dependent conservative **1.0 sec tail**; measured delayed output on 44.1/48/96k test and 44.1k host reply of 44,100 samples in 125A QA #14. Steinberg SDK IAudioProcessor contract followed. |
| A-02 | Host/automation inconsistency | Processor used `int(3*normalized)` while GUI used nearest of 0/0.5/1, causing disagreement for intermediate automation values. | Single `characterIndex` mapping shared by processor/controller; dedicated threshold and nonfinite CTest; canonical state meanings preserved. |
| A-03 | Automation artefact | OUTPUT could jump ±12dB instantly at arbitrary parameter change sample. | An **8ms** one-pole interpolation for linear output gain, startup/preloaded-state calibration, exact 0dB neutral after transition; sample-discontinuity and steady-state amplitude CTests pass. |
| A-04 | Misleading documentation | README, HOST-QA, SDK-PIN and prototype-era notes incorrectly stated no VST3 GUI/build/64-bit path. | Updated README and live host QA; older GUI/DSP notes clearly marked as historical. |

No unrelated DSP module changes were made; PUNCH/DENSE user preferences and corrected TIGHT musical behavior remain intact.

## Measured audio and CPU

**Audio:** CI #43 ran deterministic 32/64 precision, zero-neutral regression, module isolation, transient response, compressor dynamics, nonlinear residual, adaptive FINISH spectral analysis, four licensed MusicDelta real drum-only fixtures, independent asymmetric stereo matrices across 0/25/50/75/100% and three CHARACTER positions, and extreme/finite-input cases. Prior user-supplied 16s 44.1kHz stereo Studio One exports established at 50% five-FX settings that the initial TIGHT mode shrank both early attack and body; this was fixed and accepted by the user. New QA #43 WAVs and CSV metrics for unaffected fixed-settings renders were byte-identical to QA #40.

**Exact one-sample Core-path p99 measured on Ubuntu CI #41 (512 samples, full active chain):**

| Sample rate | mean processing | p95 | p99 | Max | p99 / deadline |
|---|---:|---:|---:|---:|---:|
| 44.1k | 137us | 152us | 160us | 171us | 1.38% |
| 48k | 136us | 143us | 152us | 186us | 1.43% |
| 96k | 134us | 141us | 142us | 154us | 2.66% |
| 192k | 133us | 140us | 150us | 152us | 5.63% |

This measurement emulates the VST3 wrapper's one-sample Core calls but **excludes** controller/host/GUI overhead and is **not** a Windows Studio One CPU profile. The 125A Plugin Tester #14 on Windows reported sustained full wrapper averages **281us/512 32-bit** and **341us/512 64-bit**; those are averages, **not** p99.

**Tail test:** after injected drum hits, MASS/FINISH emitted measurable samples immediately after source ends (about 0.044 and 0.0072 FS respectively in a controlled fixture). At one second after input, tested signal was silent below 1e-7 FS; host reports 1 sec tail, latency remains **0**. Zero-input tail must not be confused with device latency.

## Technical test evidence from Windows QA #14

- Steinberg Validator normal and local-instance **PASS**, no outstanding findings.
- 125A Plugin Tester v0.2.9: **47 PASS / 0 WARNING / 0 FAIL**, clean release result.
- Editor: five complete create/attach/runtime/detach/destroy cycles.
- Processor/Controller state: 68 bytes, fresh-instance transfer, current-value sync, repeat activation, 32/64 floating and normal/offline processing, NaN/Inf and denormal stress.
- Parameter automation stress with 8 parameters and 24 sample-offset events; bypass ON/OFF transitions accepted without non-finite output.
- Build/package: exact VST3 GUI resource, button count and positions, functional tag associations, value units and bundled visuals checked.

## Remaining practical acceptance, not a hidden FAIL

1. In **Studio One**, physically engage and disengage the *single BYPASS button* during playback; verify the audible dry/processed difference and that the visual state follows host automation.
2. Test **CTRL+left-click default resets** for all six knobs, including OUTPUT default 0dB, and confirm the value display updates.
3. Save/reopen a Studio One project with non-default CHARACTER, levels and BYPASS; verify restored processing and controls. Automated VST3 component state roundtrip is already PASS but does not replace this host test.
4. Confirm the actual **150% GUI** is clickable and remains readable in the user's Studio One display; code/resources are checked but screen-level use is separate.

The user has already provided a real Studio One 100% screenshot with functioning value readouts, selected DENSE state, and four FLAC A/B files; has explicitly affirmed the distinct CHARACTER modes and accepted the newly improved TIGHT sound. These count as genuine **programme-level partial acceptance**, not invented complete DAW signoff.

Windows only: no macOS build/release claim. This QA ZIP is a VST3 test artifact rather than a complete commercial installer, user manual or Gumroad package. Other hosts and exhaustive cross-genre subjective validation are not certified.

## Release gate

**No further DSP modifications are currently indicated by measured defects.** Technical QA is complete for the audited Windows artifact; full product/commercial release approval is **conditional on the four practical host checks and the separate release package documentation**. If those checks pass without defects, preserve this exact SHA and bundle as the Release Candidate instead of performing another speculative DSP iteration.
