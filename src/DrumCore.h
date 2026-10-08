#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>

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
        bodyA_ = pole(1.0 / (2.0 * 3.141592653589793 * 115.0));
        releaseA_ = pole(0.180);
        finishA_ = pole(1.0 / (2.0 * 3.141592653589793 * 3500.0));
        reset();
    }
    void reset() noexcept {
        for (auto& c : channels_) c = Channel{};
        glueEnv_ = 0.0;
        tightEnvelope_ = 0.0;
        autoGain_ = 1.0;
    }
    void setControls(const Controls& c) noexcept {
        controls_ = c;
        controls_.punch = unit(c.punch);
        controls_.body = unit(c.body);
        controls_.tight = unit(c.tight);
        controls_.finish = unit(c.finish);
        controls_.glue = unit(c.glue);
        controls_.outputDb = std::clamp(c.outputDb, -12.0f, 12.0f);
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
        const float makeup = std::pow(10.0f, controls_.outputDb / 20.0f);
        // When all macros are 0 and output is 0 dB, return a bit-exact passthrough.
        if (p == 0 && b == 0 && t == 0 && f == 0 && g == 0 && controls_.outputDb == 0) {
            for (std::size_t i=0; i<frames; ++i) {
                outLeft[i]=left[i]; outRight[i]=right[i];
            }
            return;
        }
        const float characterPunch = controls_.character == Character::Punch ? 1.0f : 0.75f;
        const float characterBody = controls_.character == Character::Dense ? 1.15f : 0.85f;
        const float characterTight = controls_.character == Character::Tight ? 1.0f : 0.75f;
        for (std::size_t i=0; i<frames; ++i) {
            const Sample x[2] = {finiteSample(left[i]), finiteSample(right[i])};
            const float peak = static_cast<float>(std::max(std::abs(x[0]),std::abs(x[1])));
            const double smooth = peak > glueEnv_ ? fastA_ : slowA_;
            glueEnv_ = smooth*glueEnv_+(1.0-smooth)*peak;
            tightEnvelope_ = slowA_*tightEnvelope_+(1.0-slowA_)*peak;
            // Slow, stereo-linked gain control: initial attacks are protected by
            // the fast/slow envelope difference; compression develops on sustain.
            const float onset = std::max(0.0f, static_cast<float>(peak-tightEnvelope_));
            const float protect = onset/(0.08f+onset);
            const float drive = static_cast<float>(tightEnvelope_)/(0.16f+static_cast<float>(tightEnvelope_));
            const float targetGain = 1.0f-g*0.65f*drive*(1.0f-protect);
            autoGain_ = releaseA_*autoGain_+(1.0-releaseA_)*targetGain;
            const float attenuation = static_cast<float>(autoGain_);
            for (int ch=0;ch<2;++ch) {
                Channel& s = channels_[ch];
                const float absx = static_cast<float>(std::abs(x[ch]));
                s.attack = fastA_*s.attack+(1.0-fastA_)*absx;
                s.sustain = slowA_*s.sustain+(1.0-slowA_)*absx;
                s.low = bodyA_*s.low+(1.0-bodyA_)*x[ch];
                const float transient = std::max(0.0f, static_cast<float>(s.attack-s.sustain));
                // Bounded attack-driven enhancement, suppressed on high-passed cymbal component.
                const float lowMid = static_cast<float>(s.low);
                const float punchDrive = p*characterPunch*transient/(0.15f+transient);
                double y = x[ch] + (punchDrive*0.6f)*lowMid;
                // Bounded, low-band-only body gain. Upper snare/cymbal bands
                // retain their direct path, avoiding a global darkening tilt.
                y += (b*characterBody*0.16f)*lowMid;
                // Tail moderation is signal-following and not a hard gate.
                const float tail = std::clamp(static_cast<float>(s.sustain/(s.attack+0.01)),0.0f,1.0f);
                y *= 1.0f-(t*characterTight*0.30f)*tail;
                // FINISH is corrective, not a second saturation stage.
                // A restrained upper-band dynamic shelf attenuates sustained
                // harshness while leaving newly arriving attacks largely intact.
                // No nonlinear waveshaper: the incoming Tape/Tube character
                // should remain the sound source, not be re-generated here.
                s.highLow = finishA_*s.highLow+(1.0-finishA_)*y;
                if (f>0.0f) {
                    const float transientProtect=std::max(0.0f,static_cast<float>(s.attack-s.sustain));
                    const float protect=transientProtect/(0.10f+transientProtect);
                    const float strength=f*0.14f*(1.0f-protect);
                    y -= strength*(y-static_cast<float>(s.highLow));
                }
                y*=attenuation*makeup;
                if (ch==0) outLeft[i]=finiteSample(static_cast<Sample>(y));
                else outRight[i]=finiteSample(static_cast<Sample>(y));
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
        double low=0.0;
        double highLow=0.0;
    };
    static float unit(float x) noexcept { return std::isfinite(x) ? std::clamp(x,0.0f,1.0f) : 0.0f; }
    template<class Sample>
    static Sample finiteSample(Sample x) noexcept { return std::isfinite(x) ? x : Sample(0); }
    double pole(double seconds) const noexcept {
        return std::exp(-1.0/(std::max(1.0e-6,seconds)*fs_));
    }
    double fs_=48000.0;
    double fastA_=0.99, slowA_=0.999, bodyA_=0.98;
    double releaseA_=0.99, finishA_=0.98;
    double glueEnv_=0.0, tightEnvelope_=0.0, autoGain_=1.0;
    Channel channels_[2]{};
    Controls controls_{};
};
} // namespace drum
} // namespace a125
