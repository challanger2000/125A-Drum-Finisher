#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include "dsp/AdaptiveResonanceSuppressor.h"

namespace a125 {
namespace drum {

enum class Character { Tight, Punch, Dense };

struct Controls {
    float punch = 0.0f;
    float body = 0.0f;
    float tight = 0.0f;
    float finish = 0.0f;
    float glue = 0.0f;
    float outputDb = 0.0f;
    Character character = Character::Punch;
};

class Core {
public:
    void prepare(double sampleRate) noexcept {
        // Hosts can supply an invalid rate during unusual lifecycle paths.
        // Never allow NaN/Infinity to poison every detector coefficient.
        fs_ = std::isfinite(sampleRate)
            ? std::clamp(sampleRate, 8000.0, 384000.0) : 48000.0;
        // Temporal constants are provisional until validated on drum fixtures.
        fastA_ = pole(0.002);
        slowA_ = pole(0.055);
        constexpr double twoPi=6.283185307179586;
        kickHighA_ = pole(1.0/(twoPi*135.0));
        kickLowA_ = pole(1.0/(twoPi*42.0));
        bodyHighA_ = pole(1.0/(twoPi*350.0));
        bodyLowA_ = pole(1.0/(twoPi*110.0));
        glueDetectorAttackA_ = pole(0.009);
        glueDetectorReleaseA_ = pole(0.180);
        glueRmsA_ = pole(0.350);
        glueGainAttackA_ = pole(0.012);
        glueGainReleaseA_ = pole(0.240);
        // TIGHT is triggered by a linked transient, then follows a bounded
        // exponential attenuation contour. All times are sample-rate derived.
        characterToneA_ = pole(1.0/(twoPi*900.0));
        characterSmoothA_ = pole(0.020);
        tightDecayA_ = pole(0.060);
        tightGainRecoverA_ = pole(0.0005);
        tightGainReduceA_ = pole(0.008);
        tightHoldSamples_ = static_cast<std::size_t>(0.015 * fs_);
        tightRefractorySamples_ = static_cast<std::size_t>(0.040 * fs_);
        tightMaximumAgeSamples_ = static_cast<std::size_t>(fs_);
        resonance_.prepare(fs_);
        reset();
    }
    void reset() noexcept {
        for (auto& c : channels_) c = Channel{};
        glueEnv_ = 0.0;
        characterLowGain_=characterHighGain_=1.0;
        characterPunchGain_=1.25;
        characterBodyGain_=0.65;
        characterTightGain_=0.40;
        tightContour_ = 0.0;
        tightGainSmoothed_ = 1.0;
        tightSinceOnset_ = 0;
        tightAboveThreshold_ = false;
        autoGain_ = 1.0;
        glueTargetGain_=1.0;
        glueRmsEnergy_=0.0;
        glueTick_=0;
        glueDirty_=true;
        resonance_.reset();
    }
    void setControls(const Controls& c) noexcept {
        glueDirty_ = glueDirty_ || controls_.glue != unit(c.glue);
        controls_ = c;
        controls_.punch = unit(c.punch);
        controls_.body = unit(c.body);
        controls_.tight = unit(c.tight);
        controls_.finish = unit(c.finish);
        controls_.glue = unit(c.glue);
        controls_.outputDb = std::isfinite(c.outputDb) ? std::clamp(c.outputDb, -12.0f, 12.0f) : 0.0f;
        makeup_ = std::pow(10.0f, controls_.outputDb / 20.0f);
    }
    const Controls& controls() const noexcept { return controls_; }

    // Interleaved-independent stereo buffers, in-place supported; no allocation or locks.
    template<class Sample>
    void processTyped(const Sample* left, const Sample* right,
                      Sample* outLeft, Sample* outRight, std::size_t frames) noexcept {
        if (!left || !right || !outLeft || !outRight) return;
        const float p = controls_.punch;
        const float b = controls_.body;
        const float t = controls_.tight;
        const float f = controls_.finish;
        const float g = controls_.glue;
        const float makeup = makeup_;
        // A neutral output must still advance detector state: otherwise
        // automating a module from 0% starts with stale envelopes.
        const bool neutral = p == 0 && b == 0 && t == 0 && f == 0 &&
                             g == 0 && controls_.outputDb == 0;
        // Explicitly separate established processing modes. The old
        // 0.75/1.0/1.15 variations measured nearly identical after level
        // matching (<0.5 dB spectral differences on the user's stereo loop).
        // PUNCH increases only the transient emphasis, TIGHT strengthens
        // the bounded decay reduction, DENSE prioritizes sustained lows.
        // The DENSE mass factor is capped at 1.0 of the calibrated 2x
        // residual, avoiding the previously rejected 3x-4x residual.
        const float characterPunchTarget = controls_.character == Character::Punch ? 1.25f : 0.65f;
        const float characterBodyTarget = controls_.character == Character::Dense ? 1.00f :
                                    (controls_.character == Character::Tight ? 0.50f : 0.65f);
        const float characterTightTarget = controls_.character == Character::Tight ? 1.50f :
                                     (controls_.character == Character::Punch ? 0.40f : 0.30f);
        // Existing macros alone were too similar after matched loudness.
        // One-pole minimum-phase low/high shelves add bounded, distinct
        // voicing: TIGHT lean (-3.5 dB low), PUNCH neutral tonality, DENSE
        // warmer (+1.4 dB low, -1.0 dB high). Amount is musical and
        // approaches zero with the effect controls. TIGHT-only and FINISH-
        // only must retain their established transient/selectivity profile.
        const double characterColorAmount=std::max(p,g);
        const double targetLowDb=characterColorAmount*
            (controls_.character==Character::Tight?-3.5:
             (controls_.character==Character::Dense?1.4:0.0));
        const double targetHighDb=characterColorAmount*
            (controls_.character==Character::Dense?-1.0:0.0);
        const double lowTarget=std::pow(10.0,targetLowDb/20.0);
        const double highTarget=std::pow(10.0,targetHighDb/20.0);
        for (std::size_t i=0; i<frames; ++i) {
            characterLowGain_=characterSmoothA_*characterLowGain_+
                (1.0-characterSmoothA_)*lowTarget;
            characterHighGain_=characterSmoothA_*characterHighGain_+
                (1.0-characterSmoothA_)*highTarget;
            // The three existing DSP character gains also crossfade over
            // 20ms. Without this, live character clicks can pop even when
            // the new shelf filters themselves are smoothed.
            characterPunchGain_=characterSmoothA_*characterPunchGain_+
                (1.0-characterSmoothA_)*characterPunchTarget;
            characterBodyGain_=characterSmoothA_*characterBodyGain_+
                (1.0-characterSmoothA_)*characterBodyTarget;
            characterTightGain_=characterSmoothA_*characterTightGain_+
                (1.0-characterSmoothA_)*characterTightTarget;
            const Sample x[2] = {finiteSample(left[i]), finiteSample(right[i])};
            const float peak = static_cast<float>(std::max(std::abs(x[0]),std::abs(x[1])));
            // Signal-relative stereo-linked bus compression. Threshold follows
            // programme RMS slowly; transfer uses an actual soft-knee ratio.
            const double detectorA=peak>glueEnv_?
                glueDetectorAttackA_:glueDetectorReleaseA_;
            glueEnv_=detectorA*glueEnv_+(1.0-detectorA)*peak;
            // Programme reference is true stereo mean-square energy,
            // NOT the peak of the two channels squared. The latter made
            // an independently panned drum hit raise the reference and
            // weaken GLUE according to channel distribution.
            const double stereoEnergy=0.5*(
                static_cast<double>(x[0])*x[0]+
                static_cast<double>(x[1])*x[1]);
            glueRmsEnergy_=glueRmsA_*glueRmsEnergy_+
                (1.0-glueRmsA_)*stereoEnergy;
            if(++glueTick_>=16||glueDirty_){
                glueTick_=0;
                glueDirty_=false;
                glueTargetGain_=1.0;
                if(g>0.0f){
                    // Relative threshold: absolute clamps caused gain-stage dependence.
                    const double threshold=std::max(
                        1.0e-12,1.35*std::sqrt(std::max(0.0,glueRmsEnergy_)));
                    const double overDb=20.0*std::log10(
                        std::max(1.0e-12,glueEnv_)/threshold);
                    constexpr double halfKnee=3.0;
                    const double above=overDb<=-halfKnee?0.0:
                        (overDb>=halfKnee?overDb:
                        (overDb+halfKnee)*(overDb+halfKnee)/(4.0*halfKnee));
                    const double ratio=1.0+2.0*g;
                    const double reductionDb=std::min(
                        6.0,above*(1.0-1.0/ratio));
                    glueTargetGain_=std::pow(10.0,-reductionDb/20.0);
                }
            }
            const double gainA=glueTargetGain_<autoGain_?
                glueGainAttackA_:glueGainReleaseA_;
            autoGain_=gainA*autoGain_+(1.0-gainA)*glueTargetGain_;
            // A 0..100% parallel blend: 0% is exactly neutral, and
            // 100% reaches the complete bounded compressor gain curve.
            // The former 80% ceiling prevented full-strength operation.
            const float attenuation=static_cast<float>(1.0-g*(1.0-autoGain_));
            // Update both channel envelopes before deriving the punch gain.
            // A stereo bus must not shift its pan because one side has a louder attack.
            double linkedTransient = 0.0;
            for (int ch=0; ch<2; ++ch) {
                Channel& s = channels_[ch];
                const float absx = static_cast<float>(std::abs(x[ch]));
                s.attack = fastA_*s.attack+(1.0-fastA_)*absx;
                s.sustain = slowA_*s.sustain+(1.0-slowA_)*absx;
                linkedTransient = std::max(linkedTransient,
                    std::max(0.0, (s.attack-s.sustain)/(s.attack+s.sustain+1.0e-12)));
            }
            // Differential amplitude ratio: identical drum transients produce
            // the same attack boost at different input gain settings.
            const float punchDrive = p*static_cast<float>(characterPunchGain_)*
                static_cast<float>(linkedTransient);
            // TIGHT: transient-triggered stereo-linked decay control.
            // Hold the first 15 ms of the hit, then approach bounded
            // reduction exponentially (60 ms contour). Fast 0.5-ms recovery
            // on each new attack avoids an abrupt gain step; 8-ms smoothing
            // on attenuation prevents zipper/click artefacts.
            const bool tightAbove = linkedTransient > 0.45;
            if (tightAbove && !tightAboveThreshold_ &&
                tightSinceOnset_ >= tightRefractorySamples_) {
                tightSinceOnset_ = 0;
                tightContour_ = 0.0;
            } else if (tightSinceOnset_ < tightMaximumAgeSamples_) {
                ++tightSinceOnset_;
            }
            tightAboveThreshold_ = tightAbove;
            if (tightSinceOnset_ > tightHoldSamples_)
                tightContour_ = tightDecayA_*tightContour_+
                    (1.0-tightDecayA_);
            const double tightTarget = std::pow(10.0,
                (-6.0*t*characterTightGain_*tightContour_)/20.0);
            const double tightSmoothA =
                tightTarget > tightGainSmoothed_
                    ? tightGainRecoverA_ : tightGainReduceA_;
            tightGainSmoothed_ = tightSmoothA*tightGainSmoothed_+
                (1.0-tightSmoothA)*tightTarget;
            // 0% must be exactly bypassed even during automated release.
            const double tightGain = t > 0.0f ? tightGainSmoothed_ : 1.0;
            // The MASS sustain detector also uses the stereo pair. Separate
            // L/R weights would reshape equally timed hits differently merely
            // because one channel is quieter.
            double linkedSustainWeight = 0.0;
            for (int ch=0; ch<2; ++ch) {
                const Channel& s = channels_[ch];
                linkedSustainWeight = std::max(linkedSustainWeight,
                    std::clamp(s.sustain/(s.attack+1.0e-12),0.0,1.0));
            }
            for (int ch=0;ch<2;++ch) {
                Channel& s = channels_[ch];
                s.kickHigh=kickHighA_*s.kickHigh+(1.0-kickHighA_)*x[ch];
                s.kickLow=kickLowA_*s.kickLow+(1.0-kickLowA_)*x[ch];
                s.bodyHigh=bodyHighA_*s.bodyHigh+(1.0-bodyHighA_)*x[ch];
                s.bodyLow=bodyLowA_*s.bodyLow+(1.0-bodyLowA_)*x[ch];
                // PUNCH is stereo linked: both channels receive the same
                // transient gain, preserving the existing interchannel ratio.
                double y=x[ch]*(1.0+0.80*punchDrive);
                // BODY is a separate 110-350 Hz low-mid sustain region;
                // difference of two stable one-pole lowpasses, not sub boost.
                // Low-frequency kick body: difference of two DC-blocking
                // lowpasses. No uncontrolled sub-bass or permanent shelf.
                const double kickBand=s.kickHigh-s.kickLow;
                const double bodyBand=s.bodyHigh-s.bodyLow;
                const double sustainWeight=linkedSustainWeight;
                // MASS: gain calibrated against the two summed one-pole band
                // responses: ~3 dB on sustained 75 Hz and 196 Hz at 100%
                // with default character. Harmonic residual remains parallel.
                // The sustain detector is gain-relative, not tied to 0.02 FS.
                // MASS: low-mid density plus controlled parallel harmonic shaping.
                // Keep old BODY parameter ID and state compatibility.
                // Additive residual amplification: the established MASS
                // filter and nonlinear shape remain unchanged. On the full
                // knob range b=[0,1], gain of the wet residual is b*(1+b).
                // Thus 0% is exact dry, 50% gives 0.75x of the old 100%
                // residual, and 100% gives 2.0x. This is not a post gain
                // boost and does not modify other modules or parameter IDs.
                // A 2x cap was selected after level-matched 1x/1.5x/2x/
                // 3x/4x comparisons on 5 drum sources; beyond 2x bass
                // benefit diminished while spectral balance/width worsened.
                const double massAmount=b*(1.0+b)*characterBodyGain_;
                // Bass Finisher-derived principle: only the nonlinear odd-harmonic
                // residual is added. Normalize the waveshaper by drive so its
                // small-signal slope is unity; the old tanh(2.5) normalization
                // unintentionally inserted additional linear band gain.
                constexpr double massDrive=2.5;
                const double harmonicResidual=bodyBand-
                    std::tanh(massDrive*bodyBand)/massDrive;
                y+=massAmount*sustainWeight*
                    (0.60*kickBand+0.60*bodyBand+0.28*harmonicResidual);
                // Tail moderation is signal-following and not a hard gate.
                y *= tightGain;
                // The FINISH detector processes both pre-output channels together.
                if(ch==0) finishPairLeft_=y;
                else finishPairRight_=y;
                y*=attenuation*makeup;
                if (ch==0) outLeft[i]=finiteSample(static_cast<Sample>(y));
                else outRight[i]=finiteSample(static_cast<Sample>(y));
            }
            // Stereo-linked adaptive resonance control, after other shaping.
            // Stateful detector is maintained even at 0% to permit smooth automation.
            double fl=finishPairLeft_,fr=finishPairRight_;
            resonance_.processFrame(fl,fr,f);
            // Correction is applied to the already rendered channel outputs.
            // Only the delta is added; unity at FINISH=0 remains bit-exact.
            outLeft[i]=neutral ? x[0] : finiteSample(static_cast<Sample>(outLeft[i]+(fl-finishPairLeft_)*attenuation*makeup));
            outRight[i]=neutral ? x[1] : finiteSample(static_cast<Sample>(outRight[i]+(fr-finishPairRight_)*attenuation*makeup));
            // Stereo pair uses identical first-order response coefficients
            // and smoothed gain. Keep the filter warm even at 0% or while
            // bypassed; in/out signal is unchanged when macro is disabled.
            for(int ch=0;ch<2;++ch){
                Channel& channel=channels_[ch];
                const double raw=ch==0?outLeft[i]:outRight[i];
                channel.toneLow=characterToneA_*channel.toneLow+
                    (1.0-characterToneA_)*raw;
                if(characterColorAmount>0.0 && !neutral){
                    const double colored=characterLowGain_*channel.toneLow+
                        characterHighGain_*(raw-channel.toneLow);
                    if(ch==0)outLeft[i]=finiteSample(static_cast<Sample>(colored));
                    else outRight[i]=finiteSample(static_cast<Sample>(colored));
                }
            }
        }
    }
    void process(const float* l,const float* r,float* ol,float* or_,std::size_t n) noexcept {
        processTyped(l,r,ol,or_,n);
    }
    void process(const double* l,const double* r,double* ol,double* or_,std::size_t n) noexcept {
        processTyped(l,r,ol,or_,n);
    }

private:
    struct Channel {
        double attack=0.0;
        double sustain=0.0;
        double kickHigh=0.0;
        double kickLow=0.0;
        double bodyHigh=0.0;
        double bodyLow=0.0;
        double toneLow=0.0;
    };
    static float unit(float x) noexcept { return std::isfinite(x) ? std::clamp(x,0.0f,1.0f) : 0.0f; }
    template<class Sample>
    static Sample finiteSample(Sample x) noexcept { return std::isfinite(x) ? x : Sample(0); }
    double pole(double seconds) const noexcept {
        return std::exp(-1.0/(std::max(1.0e-6,seconds)*fs_));
    }
    double fs_=48000.0;
    double fastA_=0.99, slowA_=0.999;
    double characterToneA_=0.99,characterSmoothA_=0.99;
    double characterLowGain_=1.0,characterHighGain_=1.0;
    double characterPunchGain_=1.25,characterBodyGain_=0.65,characterTightGain_=0.40;
    double kickHighA_=0.98, kickLowA_=0.99;
    double bodyHighA_=0.98, bodyLowA_=0.99;
    double glueDetectorAttackA_=0.99,glueDetectorReleaseA_=0.99;
    double glueRmsA_=0.999,glueGainAttackA_=0.99,glueGainReleaseA_=0.999;
    double glueEnv_=0.0,autoGain_=1.0;
    double tightDecayA_=0.999,tightGainRecoverA_=0.99,tightGainReduceA_=0.99;
    double tightContour_=0.0,tightGainSmoothed_=1.0;
    std::size_t tightSinceOnset_=0,tightHoldSamples_=0;
    std::size_t tightRefractorySamples_=0,tightMaximumAgeSamples_=0;
    bool tightAboveThreshold_=false;
    double glueTargetGain_=1.0,glueRmsEnergy_=0.0;
    int glueTick_=0;
    bool glueDirty_=true;
    Channel channels_[2]{};
    Controls controls_{};
    float makeup_=1.0f;
    double finishPairLeft_=0.0,finishPairRight_=0.0;
    dsp::AdaptiveResonanceSuppressor resonance_{};
};
} // namespace drum
} // namespace a125
