# GUI field feedback: Studio One, first test build

Observed by user:
- Character presents numeric labels 0.0000 / 0.5000 / 1.0000
- GUI bypass switch not working
- Only TIGHT processing perceptibly compresses
- Controller/parameter values remain when editor is reopened

Corrections in this revision:
- Character uses an explicit VST3 StringListParameter with three user-visible labels, stable ID 106.
- Bypass uses a discrete two-segment VSTGUI control bound to VST3 bypass parameter 107, visibly ON/BYPASS instead of the momentary CTextButton.
- No state reset on reopening: preserving parameter values while reopening editor is correct VST3 host behavior. If fresh instances inherit values unexpectedly, reproduce and fix separately.

Still unverified:
- Studio One actual parameter binding and reset gestures.
- Real audible DSP quality of each module; never infer sonic usefulness from BUILD SUCCESS.
- Visual metal ring alignment, main logo, labels and zoom, editor lifecycle.
- 125A release gates.

Do not claim the GUI fixed until the next host test.
