# VST3 host integration — audited state (2026-10-10)

The former version of this document listed prototype deficiencies (last-point-only automation, 32-bit-only, no editor, hard bypass) as if still current. Those items were corrected in the development branch and must not be reused as release claims.

## Verified technical evidence

- **Sample-offset-accurate automation:** Processor iterates bounded per-parameter VST3 queues and applies events at their sample offsets, including zero-length flush. The exact-current CI has a parameter stress matrix (24 sample-accurate points).
- **32/64-bit float processing:** Both paths exist and have passed Steinberg Validator, 125A Plugin Tester v0.2.9 and CTest in Windows Full Release QA #13, run `38020621476`.
- **Stereo:** Explicit one stereo input/one stereo output; mono buses are intentionally unsupported. Linked PUNCH/TIGHT/GLUE dynamics preserve L/R coherence while each channel retains its independent material.
- **Bypass:** One VST3 parameter (ID 107), 64-sample stereo-linked crossfade in both directions, wet engine runs during bypass; component state recalls parameter.
- **Editor:** CTextButton CHARACTER selection (ID 106), one BYPASS toggle, 100/150 zoom, real VSTGUI controls and passive faceplate. Five full editor lifecycle passes in QA #13.
- **State:** version 1, 68-byte component state, eight parameters IDs 100–107; 125A plugin tester includes roundtrip, fresh-instance verification and controller transfer.
- **Latency:** 0 algorithmic lookahead samples. Tail is **not** zero: MASS and FINISH emit finite decays after the input stops; the release-audit fix reports a conservative ~1-second `getTailSamples` bound at the actual setup sample rate, verified by impulse tests at 44.1/48/96 kHz.
- **Realtime:** allocation-free bounded core, no locks or audio-thread file I/O. Audio QA #41 measures p99 at 512 frames as ~1.43% of available time at 48k and ~5.63% at 192k in a Linux runner using the wrapper-like 1-sample core path. Full VST3 overhead is separately exercised in Windows plugin tester, but its p99 has not been measured in a live DAW.
- **OUTPUT and CHARACTER automation:** current audit adds 8ms gain ramp for abrupt OUTPUT changes and one shared normalized 3-step CHARACTER decoder; both receive explicit regression checks (Audio QA #43).
- **Final Windows build:** QA #14 runs separately after the above changes; earlier QA #13 cannot certify the newer binary.

## Remaining real-host acceptance

Actual **Studio One** user-level automation gestures, bypass click/re-engage, CTRL+left-click to defined defaults, 150%-zoom/preset re-opening, and project-save/reopen audio-equivalence are not proven merely by Validator or source inspection. The user's supplied screenshot confirms live GUI appearance, three selectable CHARACTER outputs and value readouts, and the user has approved the corrected TIGHT voicing.

Official SDK pin: `v3.8.1_build_84`, commit `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`.

**Gate discipline:** statuses in `QA-EVIDENCE.md` and the current dated audit are authoritative over prototype-era notes.
