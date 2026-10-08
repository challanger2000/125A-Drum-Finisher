# 125A Drum Finisher V1 — Release QA evidence matrix

This is **development**, not a release certificate. A check is PASS only with a linked run or reproducible artifact.

| Gate | Status | Evidence |
|---|---|---|
| Windows x64 VST3 compilation, SDK 3.8.1 | PASS | GitHub Actions #14 |
| CTest functional DSP regressions | PASS (6/6) | GitHub Actions #14 |
| Steinberg Validator, standard mode | PASS (47/47) | GitHub Actions #14 |
| Steinberg Validator, fresh instance per test | PENDING | Newly added `validator -l` step, first verification pending |
| 125A Plugin Tester | NOT RUN | A standalone tester execution is still required |
| Editor lifecycle and GUI zoom 100/150 | NOT IMPLEMENTED | Current VST3 Controller has no custom editor |
| I/O Event Probe | NOT RUN | Dedicated external probe required |
| Offline/lifecycle/audio torture | NOT RUN | Dedicated external test required |
| Sample-offset automation end-to-end | NOT VERIFIED | Processing implemented; only source inspected so far |
| Rapid bypass transitions in actual VST3 host | NOT VERIFIED | Pure C++ bypass ramp tests are not host QA |
| Processor+Controller state and project recall | NOT VERIFIED | Needs save/reload audio-equivalence regression |
| Mono bus support | NOT IMPLEMENTED | Current plugin negotiates stereo only |
| 32/64-bit paths | CORE TEST PASS | 64-bit host-level testing still required |
| CPU p95/p99/max and deadline misses | NOT MEASURED | Must profile realistic full drum bus |
| Real drum audio level-matched sonic tests | NOT RUN | Require fixed source and already-saturated fixtures |

A successful Validator result must never be represented as the 125A Plugin Tester.
The developer must install/obtain the official 125A tester and run separate Lifecycle, Event and Audio Torture probes; results need their own evidence.

Source chain:
`Perfect Drums -> MixEngine V3 (or equivalent) -> 125A Drum Finisher`.

Before any release: resolve all required gates in 125A-Engineering/QA/RELEASE-QA.md and log evidence with exact build SHA.
