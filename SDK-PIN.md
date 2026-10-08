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

**Verification status:** exact SDK tag and immutable commit confirmed; plug-in compilation, host compatibility and Validator have NOT yet run. Do not use this preliminary branch for distribution.

**Known VST3 integration gaps:** GUI, 64-bit buffers, sample-offset-accurate automation, safe state-transfer boundary, bypass tail/silence semantics, host stress tests. These must be resolved before release QA.
