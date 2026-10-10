# 125A Drum Finisher V1 — authoritative release QA evidence

**Updated:** 2026-10-10. This document supersedes the earlier prototype-era matrix. The programme and technical gates are **independent**; a PASS in Steinberg Validator is not equivalent to a DAW user acceptance.

**Exact fully audited Windows VST3:** Full Release QA #14, [run 38022161131](https://github.com/challanger2000/125A-Drum-Finisher/actions/runs/38022161131), commit `8ab575977aa6dfdd04614c957db5f4fd7bd8e6ed`, SUCCESS, exact artifact `125A-Drum-Finisher-V1-Full-Release-QA` (artifact ID 11658343952). Local SHA256 of downloaded artifact ZIP: `0f5984b487b556e432434660b6b9b0151323b72f19bcdf2da51d8a7bcfbd2fa0`.

**Audio DSP regression:** Audio QA #43, [run 38021945180](https://github.com/challanger2000/125A-Drum-Finisher/actions/runs/38021945180), SUCCESS at DSP commit `9d9c8204c94f58ad076594dbe0e094863b091de8`. The QA #14 build includes this same DSP. A GitHub SHA-by-SHA comparison confirms HEAD source blobs for Core, VST3 Processor/Controller, GUI, resource and CMake equal the validated Windows commit; newer changes are documentation only.

## Gate matrix

| Category / requirement | Status | Actual evidence / limitation |
|---|---|---|
| Build Windows x64 VST3 | **PASS** | GitHub QA #14, SDK pinned to `v3.8.1_build_84`. |
| Steinberg Validator normal / local instance | **PASS** | Both exact QA #14 binary checks complete. |
| 125A Plugin Tester | **PASS** | 47 PASS / 0 WARNING / 0 FAIL, v0.2.9, QA #14. |
| Audio engine 32 / 64 bit | **PASS** | QA #14, sample-rate matrix and CTest. |
| Explicit stereo I/O, no undocumented bus | **PASS** | One stereo input/output; **mono bus deliberately unsupported**. |
| State save/restore, controller sync, instance reload | **PASS (automated)** | 68-byte state, 8 ParamIDs 100–107, roundtrip, fresh instance, 5 reload cycles; actual Studio One project save/reload not separately verified. |
| Sample-offset automation | **PASS (automated)** | 24-point host sample automation stress, Processor param-cursor code. |
| Stepped CHARACTER mapping | **PASS** | Shared 3-position decoder on Controller/Processor, including intermediate values; new CTest. |
| OUTPUT automation glide and exact 0dB neutral | **PASS** | New 8ms gain smoothing regression, exact 0dB and ±12dB steady-state tests. |
| Bypass processing and stereo-link ramp | **PASS (automated)** | 64-sample bypass crossfade, state retained, active DSP maintained while bypassed, QA #14 stress; physical Studio One click to bypass and back remains unverified. |
| 0% neutral / 25-100% module scale / stable full chain | **PASS** | Audio QA #43, four licensed real drum fixtures plus synthetic independently varied stereo. |
| PUNCH/MASS/TIGHT/FINISH/GLUE measurable intended behavior | **PASS (functional)** | Existing DSP QA, injected 550/1200/2700/4100Hz resonances, attack/body/tail segmented real user stereo A/B. |
| Real user Studio One character audibility / corrected TIGHT | **PASS for supplied programme** | User confirms strong mode contrasts and accepts improved TIGHT; PUNCH at five FX 50% is user preferred. Not universal proof across genres. |
| Finish/Gain clipping under intended real programme | **PASS for current fixtures** | Finite output and true-peak stress of real user source; OUTPUT +12dB can deliberately drive the signal over 0dBFS, not an automatic limiter. |
| Zero algorithm latency | **PASS** | Host reports zero samples; no lookahead in processing path. |
| DSP tail metadata | **PASS** | MASS/FINISH post-input nonzero proven; host now reports a conservative one-second sample-rate-dependent tail. QA #14 reports 44,100 samples at 44.1k. |
| Realtime allocations / CPU overhead | **PASS for sampled tests** | No allocations or I/O in audio path, Linux one-sample p95/p99 benchmark, Windows plugin-tester sustained path. Windows DAW p99 unmeasured. |
| 100%/150% GUI resources, 4 buttons, value units | **PASS (packaged/visual)** | Verified actual `.uidesc`, user screenshot at 100%, native buttons and resources present; physical 150%-zoom behavior in Studio One not fully verified. |
| Editor lifecycle attach/detach | **PASS** | Five cycles within 125A Plugin Tester #14. |
| Ctrl+left-click actual six-knob reset in Studio One | **NOT YET VERIFIED** | VSTGUI base implements the event and defaults are wired; no physical mouse test. |
| Studio One project save/reload audio identity | **NOT YET VERIFIED** | Automated state roundtrips are not a substitute. |
| Studio One BYPASS/automation and 150%-zoom click and recall | **NOT YET VERIFIED** | Formal practical signoff still required. |
| macOS binary/host verification | **NOT APPLICABLE to Windows-only V1** | No Mac build is advertised or provided. |
| Commercial installation/manual/rights/package documentation | **NOT YET RELEASE-PACKAGED** | Current test artifact contains the VST3 bundle only; Gumroad-style final packaging is separate. |

**Decision:** Technical Windows VST3 QA complete and verified for current binary; **no remaining observed automated test failure**. The signoff for all real DAW interactions and broader user musical benefit cannot be inferred from automated QA and must not be mislabeled PASS. See `QA/FINAL-RELEASE-AUDIT-2026-10-10.md` for exact evidence and outstanding release conditions.
