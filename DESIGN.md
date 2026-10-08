# Drum Finisher V1 — Technical design / test contracts

## Sources and positioning

Guitar Finisher V3 supplies the zero-neutral amount discipline and robust lifecycle/automation expectations, **not its guitar-frequency corrections**.
Bass Finisher V1 supplies source-adaptive low-end control concepts, **not its bass-only processing**.
Ultimate Finisher V1 supplies controlled levels, deterministic measurements and regression discipline, **not its master-bus loudness/limiter defaults**.

## Core layout (provisional)

- **PUNCH:** envelope-based transient emphasis; should increase attack-to-sustain contrast in a bounded band without adding uncontrolled high-frequency cymbal spikes.
- **BODY:** controlled mid/low-frequency energy enhancement; verify phase and spectral impact and prevent low-end accumulation.
- **TIGHT:** envelope-tail management and low-frequency resonance control; must preserve kick fundamental and double-kick articulation.
- **FINISH:** gentle programme-adaptive tonal refinement; don't automate guessed harshness frequencies into a fixed heavy notch.
- **GLUE:** stereo-linked bus dynamics without crushing kick/snare peaks or image shift.
- **OUTPUT:** calibrated final trim; should not alter tone by itself.
- **CHARACTER:** TIGHT / PUNCH / DENSE profiles only change coefficients/thresholds of active processing.

## Engineering acceptance measurements before V1 release

1. Neutral settings: sample-equivalent input/output for both channels (excluding explicitly intentional gain trim).
2. Attack/sustain: independent transient ratio fixtures for kick, snare, tom and hats.
3. Low-frequency stability: sub/low resonance decay, fundamental preservation and repeated double-kick bursts.
4. Stereo: linked detectors, dual-mono symmetry, asymmetrical sources, no phantom narrowing.
5. Output: level-matched A/B; integrated loudness and true peak; no unexpected increases.
6. Real programme: acoustic kit, sampled rock/metal kit, fast double-kick, electronic/industrial kit, dry and roomy kits.
7. DSP: oversampling/alias impact only where nonlinear stages justify it.
8. Hosts: Steinberg Validator 47/47 where applicable, 125A Plugin Tester, editor lifecycle, I/O probe, offline, multirate, variable block, reset/defaults, state and automation.
9. Realtime: allocations 0; no locks/file work in audio callback; profile p95/p99/max and deadline overruns at supported rates/blocks.

## Status / known limitations

The initial pure-DSP kernel is a **research prototype**, not a signed-off production architecture.
Its fixed filter poles and detector constants require measured fixture calibration.
No standalone VST3 implementation, GUI, state migration or verified SDK dependency is included in this first baseline.
Do not label the result PASS or final before corresponding evidence.
