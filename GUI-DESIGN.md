> **Historical static design specification, not the current live layout.** The current authoritative 1200×540 UI geometry, actual button positions and compiled metal faceplate are in `resource/DrumFinisher.uidesc` and `src/gui/FaceplateView.cpp`. The current UI has one header BYPASS toggle, three centered CHARACTER buttons and six readouts with units below plain module titles. The early coordinate proposals below are intentionally retained as design history, **not** as an implementation or release checklist.

# 125A Drum Finisher V1 — Static GUI design gate

Status: **design approved for implementation only after visual review**; no claim that the VSTGUI editor is installed.

## Product identity

Source: exact original `challanger2000/125A-Branding/125A_Logo_Master_FINAL.svg`. Preserve paths and the red A stroke. No recreation from text or font.

## Canvas

Logical resolution: **1200 x 540** at 100%, scale 1.5 for 150%. No DAW border drawn. Dark industrial steel, physically plausible recessed sections, screws and controlled lighting, consistent with 125A finishing tools.

Header: master logo left; centered "125A DRUM FINISHER V1"; right zoom 100/150 and bypass status.

Main row of six illuminated metal-ring controls:
- PUNCH (tag 100), BODY (101), TIGHT (102), FINISH (103), GLUE (104)
- OUTPUT (105) displayed in dB with 0 dB at the defined default; separate visually from amount knobs
- 0–100% amount controls with precise values and Ctrl+left-click reset.
- Three discrete character buttons **TIGHT / PUNCH / DENSE** (tag 106); selection visible and state-recalled.
- BYPASS (tag 107) has a dedicated on/off button and clear state.

No MATCH, Tape, Tube, console, saturation or reverb stage. The input is assumed to have passed through MixEngine V3 or a comparable tone stage.

## Layout coordinates (logical px at 100%)

- Header/logo region: x 30–180, y 20–95
- Title: x 260–940, y 25–63
- Zoom: x 1035–1168, y 25–53
- Main module group: x 38–1162, y 128–350
- Knob centers: x 132 / 318 / 504 / 690 / 876 / 1062, y 245
- Character strip: x 365–835, y 398–450
- Bypass: x 1040–1160, y 445–485
- Footer: version, selected profile and output status; y 504–532

## Metal rings: no placeholders

The ring filmstrip/PNG set must originate from existing, verified 125A master artwork. The Guitar Finisher SteelKnob code documents:
- `125A_FinisherRing_S_64px_100pct.png` and 96px 150%
- `125A_FinisherRing_M_96px_100pct.png` and 144px 150%
- `125A_FinisherRing_H_128px_100pct.png` and 192px 150%

These *names alone are not proof that the assets exist in the requested repository*. Before integration, locate and verify actual binary artwork, check dimensions, transparency, lighting, and physical center; do not synthesize substitutes or assume missing files can render.

## Implementation/release criteria

1. Build a **static visual design** and inspect it visually before VSTGUI coding.
2. Integrate real logo and rings, exact resource paths, proper package resources.
3. All controls bound to current stable VST3 parameter IDs; value display corresponds to actual DSP.
4. Zoom at both 100/150%; Windows HiDPI tested.
5. Ctrl+click reset and editor lifecycle PASS.
6. No editor-open writes to DSP, crash-free reopen, save/restore consistency.
7. Provide actual VST3 binary for Studio One preview before any Gumroad package.
8. User listening test on `Perfect Drums -> MixEngine V3 -> Drum Finisher` precedes final audio QA.
