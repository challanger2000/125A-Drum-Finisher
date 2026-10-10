# Official Steinberg VST3 SDK dependency

Use **v3.8.1_build_84**, verified Git commit:

`3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`

The top-level SDK uses git submodules (base, pluginterfaces, public.sdk, vstgui4); a shallow source ZIP without their contents is insufficient.

Windows checkout (developer / CI environment):

```sh
git clone https://github.com/steinbergmedia/vst3sdk.git --recursive
cd vst3sdk
git checkout 3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96
git submodule update --init --recursive
```

CMake variable `VST3_SDK_ROOT` points to that checkout. SDK integration is optional in the preliminary local CMake configuration.

**Verification status (updated 2026-10-10):** the SDK version and immutable revision above are pinned by CI; Windows VST3 compilation and official Steinberg Validator normal/local checks have passed, most recently QA #13 (2026-10-10). Full Windows QA #14 is being executed for the newer final-audit corrections. This SDK pin alone is not a release certificate.

**Integration coverage:** GUI, 64-bit buffers, sample-offset automation, state transfer, bypass crossfade and host stress tests are now implemented and exercised in CTest / QA. The final audit corrected missing VST3 tail metadata, discrete mode rounding and OUTPUT automation smoothing. Actual Studio One click, automation, zoom and project-reopen behavior remain separate practical acceptance gates; see `HOST-QA.md`.
