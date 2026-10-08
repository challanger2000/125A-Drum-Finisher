# VST3 host-path audit (engineering)

Known open items before 125A Release QA:
- Processor consumes only the **last** automation point per parameter per block. This is not sample-offset-accurate. Fix with a bounded, no-allocation segment schedule and smoothing; test collisions and varying block sizes.
- Only 32-bit floating point buffers are supported. Add an independently verified double-precision processing path before claiming 64-bit host support.
- Current bypass is an immediate hard switch. Verify tail continuity and click-safe automation before release.
- Controller has no custom editor; lifecycle and 100/150% zoom are not tested.
- Silence flags were previously hardcoded to zero. This revision derives flags from actual per-channel output after processing; still verify in the Steinberg Validator and host.
- Core still uses provisional envelope coefficients and does not have validated auto-level.
- No realistic programme A/B, independent oversampling/alias proof, or p99 deadline evidence yet.

The build/test workflow's existing core tests are **not** a substitute for Steinberg Validator or the 125A Plugin Tester.
