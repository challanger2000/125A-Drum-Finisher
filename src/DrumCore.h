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
        fs_ = std::clamp(sampleRate, 8000.0, 384000.0);
        // Temporal constants are provisional until validated on drum fixtures.
        fastA_ = pole(0.002);
        slowA_ = pole(0.055);
        constexpr double twoPi=6.283185307179586;
        bodyHighA_ = pole(1.0/(twoPi*350.0));
        bodyLowA_ = pole(1.0/(twoPi*110.0));
        glueDetectorAttackA_ = pole(0.012);
        glueDetectorReleaseA_ = pole(0.180);
        glueRmsA_ = pole(0.350);
        glueGainAttackA_ = pole(0.025);
        glueGainReleaseA_ = pole(0.240);
        resonance_.prepare(fs_);
        reset();
    }
    void reset() noexcept {
        for (auto& c : channels_) c = Channel{};
        glueEnv_ = 0.0;
        tightEnvelope_ = 0.0;
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
        // When all macros are 0 and output is 0 dB, return a bit-exact passthrough.
        if (p == 0 && b == 0 && t == 0 && f == 0 && g == 0 && controls_.outputDb == 0) {
            for (std::size_t i=0; i<frames; ++i) {
                // Maintain exact passthrough for valid audio, but never
                // forward non-finite host samples into downstream plug-ins.
                outLeft[i]=finiteSample(left[i]);
                outRight[i]=finiteSample(right[i]);
            }
            return;
        }
        const float characterPunch = controls_.character == Character::Punch ? 1.0f : 0.75f;
        const float characterBody = controls_.character == Character::Dense ? 1.15f : 0.85f;
        const float characterTight = controls_.character == Character::Tight ? 1.0f : 0.75f;
        for (std::size_t i=0; i<frames; ++i) {
            const Sample x[2] = {finiteSample(left[i]), finiteSample(right[i])};
            const float peak = static_cast<float>(std::max(std::abs(x[0]),std::abs(x[1])));
            // Signal-relative stereo-linked bus compression. Threshold follows
            // programme RMS slowly; transfer uses an actual soft-knee ratio.
            const double detectorA=peak>glueEnv_?
                glueDetectorAttackA_:glueDetectorReleaseA_;
            glueEnv_=detectorA*glueEnv_+(1.0-detectorA)*peak;
            glueRmsEnergy_=glueRmsA_*glueRmsEnergy_+
                (1.0-glueRmsA_)*static_cast<double>(peak)*peak;
            tightEnvelope_=slowA_*tightEnvelope_+(1.0-slowA_)*peak;
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
            // Restrained parallel dynamics preserves more of the dry attack.
            const float attenuation=static_cast<float>(1.0-0.80*g*(1.0-autoGain_));
            // Update both channel envelopes before deriving the punch gain.
            // A stereo bus must not shift its pan because one side has a louder attack.
            double linkedTransient = 0.0;
            for (int ch=0; ch<2; ++ch) {
                Channel& s = channels_[ch];
                const float absx = static_cast<float>(std::abs(x[ch]));
                s.attack = fastA_*s.attack+(1.0-fastA_)*absx;
                s.sustain = slowA_*s.sustain+(1.0-slowA_)*absx;
                linkedTransient = std::max(linkedTransient,
                    std::max(0.0, s.attack-s.sustain));
            }
            const float punchDrive = p*characterPunch*
                static_cast<float>(linkedTransient/(0.12+linkedTransient));
            for (int ch=0;ch<2;++ch) {
                Channel& s = channels_[ch];
                s.bodyHigh=bodyHighA_*s.bodyHigh+(1.0-bodyHighA_)*x[ch];
                s.bodyLow=bodyLowA_*s.bodyLow+(1.0-bodyLowA_)*x[ch];
                // PUNCH is stereo linked: both channels receive the same
                // transient gain, preserving the existing interchannel ratio.
                double y=x[ch]*(1.0+0.80*punchDrive);
                // BODY is a separate 110-350 Hz low-mid sustain region;
                // difference of two stable one-pole lowpasses, not sub boost.
                const double bodyBand=s.bodyHigh-s.bodyLow;
                const double sustainWeight=std::clamp(
                    s.sustain/(s.attack+0.02),0.0,1.0);
                // MASS: low-mid density plus controlled parallel harmonic shaping.
                // Keep old BODY parameter ID and state compatibility.
                const double massAmount=b*characterBody;
                // Bass Finisher-derived principle: only the nonlinear odd-harmonic
                // residual is added. Normalize the waveshaper by drive so its
                // small-signal slope is unity; the old tanh(2.5) normalization
                // unintentionally inserted additional linear band gain.
                constexpr double massDrive=2.5;
                const double harmonicResidual=bodyBand-
                    std::tanh(massDrive*bodyBand)/massDrive;
                y+=massAmount*sustainWeight*
                    (0.42*bodyBand+0.28*harmonicResidual);
                // Tail moderation is signal-following and not a hard gate.
                const float tail = std::clamp(static_cast<float>(s.sustain/(s.attack+0.01)),0.0f,1.0f);
                y *= 1.0f-(t*characterTight*0.30f)*tail;
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
            outLeft[i]=finiteSample(static_cast<Sample>(outLeft[i]+(fl-finishPairLeft_)*attenuation*makeup));
            outRight[i]=finiteSample(static_cast<Sample>(outRight[i]+(fr-finishPairRight_)*attenuation*makeup));
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
        double bodyHigh=0.0;
        double bodyLow=0.0;
    };
    static float unit(float x) noexcept { return std::isfinite(x) ? std::clamp(x,0.0f,1.0f) : 0.0f; }
    template<class Sample>
    static Sample finiteSample(Sample x) noexcept { return std::isfinite(x) ? x : Sample(0); }
    double pole(double seconds) const noexcept {
        return std::exp(-1.0/(std::max(1.0e-6,seconds)*fs_));
    }
    double fs_=48000.0;
    double fastA_=0.99, slowA_=0.999;
    double bodyHighA_=0.98, bodyLowA_=0.99;
    double glueDetectorAttackA_=0.99,glueDetectorReleaseA_=0.99;
    double glueRmsA_=0.999,glueGainAttackA_=0.99,glueGainReleaseA_=0.999;
    double glueEnv_=0.0,tightEnvelope_=0.0,autoGain_=1.0;
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
