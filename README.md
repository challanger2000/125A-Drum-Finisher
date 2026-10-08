# 125A Drum Finisher V1

Adaptive stereo drum-bus finisher for Windows x64 VST3.

**Status:** engineering prototype; not built, not VST3-validated, not release-ready.

## Product contract

No reference MATCH. Source-specific processing for kick, snare, toms and cymbals on a complete drum bus.

Core processing: `PUNCH -> BODY -> TIGHT -> FINISH -> GLUE -> OUTPUT`.

Character modes: TIGHT, PUNCH, DENSE. All effect amounts are neutral at 0%; default musical region is 20-50%. The modes alter the response of active processors only. Output is gain trim, 0 dB at its default.

No broad-band transient gain that makes cymbals excessively sharp; onset emphasis is biased toward lower and mid bands and limited by envelope tracking. No unnecessary global reverb or reference-file handling.

## Engineering

- `v1.0.0-engineering`: active development branch. Do not develop on main.
- `src/DrumCore.h`: SDK-independent deterministic 32/64-bit-compatible floating-point DSP prototype; currently float-only processing API.
- `tests/DrumCoreTests.cpp`: neutrality, finite-signal and stereo symmetry regression.
- `CMakeLists.txt`: portable core tests only. **This is not yet a VST3 build.**
- VST3 integration must use official **Steinberg VST3 SDK 3.8.1 (2026-08-11)**, pinned to a verified immutable revision, with VST3 processor/controller state, automation, latency and editor tests.

See `DESIGN.md` for processing contracts and unresolved measurements.

All decisions and releases follow `challanger2000/125A-Engineering/START-HERE.md`.
