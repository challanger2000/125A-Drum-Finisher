# 125A Drum Finisher V1 — Windows x64 VST3

Professional drum-bus effect from **125A Systems**, with five independently scalable musical modules (PUNCH, MASS, TIGHT, FINISH, GLUE), output trim, three selectable characters and bypass.

## Verified product / repository status

- **Development branch:** `v1.0.0-engineering`. Stable `main` is not the development branch.
- **Windows VST3 candidate, QA #13:** completed on 2026-10-10, [run #38020621476](https://github.com/challanger2000/125A-Drum-Finisher/actions/runs/38020621476). Steinberg Validator normal/local, 125A Plugin Tester **47 PASS / 0 WARNING / 0 FAIL**, five Editor Lifecycle cycles and 32/64-bit DSP.
- **Audio QA #40:** [run #38020311605](https://github.com/challanger2000/125A-Drum-Finisher/actions/runs/38020311605), successful stereo/drum fixture regression with corrected TIGHT character.
- **Final release audit:** separately checks host tail metadata, measured p95/p99 processing and exact final Windows package on the newest development commit. Previous QA does **not** automatically prove any later code changes.
- **Release not automatically approved by QA:** direct Studio One click/automation/state recall and musical acceptance are separate gates. There is no published GitHub release or release tag as of this report.

## Processing / host features

`PUNCH → MASS → TIGHT → FINISH → GLUE → OUTPUT` are product module names; actual processing is an integrated stereo-linked, sample-accurate DSP chain. Each module amount 0–100%; 0% effect amounts neutral, 25–50% musical operating range, full range up to 100%. OUTPUT is −12 to +12 dB with 0 dB neutral. CHARACTER = TIGHT/PUNCH/DENSE; one BYPASS toggle; zoom 100/150%.

The processor uses **stereo input and stereo output** intentionally; no mono bus is advertised. 32-bit and 64-bit floating-point processing are implemented. It exposes eight stable ParamIDs (100–107), versioned component state and VST3 automation.

## Engineering, testing and evidence

- `src/DrumCore.h` — allocation-free sample processor and mode controls.
- `src/dsp/` — adaptive resonance suppression and stability primitives.
- `src/vst/DrumPlugin.cpp` — official Steinberg VST3 Processor/Controller, sample-accurate parameter queues, state, editor and bypass.
- `src/gui/` and `resource/` — custom GUI, zoom and VSTGUI controls.
- `tests/` — functional, precision, bypass, resonance, unit readout, GUI contract, realtime and user-programme-adjacent audio QA.
- `QA/` — measurements, original-audio provenance without distributing private recordings, technical limitations and release decisions.
- `.github/workflows/full-release-qa.yml` — deliberate Windows full validation, not every DSP push.
- `.github/workflows/drum-stem-audio-qa.yml` — lower-cost Linux audio regression.

Follow [125A Engineering](https://github.com/challanger2000/125A-Engineering/blob/main/START-HERE.md) as the mandatory source of standards. Do not equate green CI with user-approved sound or real DAW mouse interaction.
