#pragma once
#include <algorithm>
#include <cmath>
namespace a125::drum {
// A bounded linear bypass ramp: target 1 = dry, target 0 = processed.
// The DSP must run continuously regardless of this ramp.
class BypassRamp {
public:
    void reset(bool bypassed) noexcept { position_=bypassed?1.0:0.0; }
    double advance(bool bypassed) noexcept {
        const double target=bypassed?1.0:0.0;
        constexpr double step=1.0/64.0;
        if(position_<target)position_=std::min(target,position_+step);
        else if(position_>target)position_=std::max(target,position_-step);
        return position_;
    }
    template<class T> T mix(T wet,T dry,bool bypassed) noexcept {
        const double blend=advance(bypassed);
        return static_cast<T>(wet*(1.0-blend)+dry*blend);
    }
    double position() const noexcept {return position_;}
private:
    double position_=0.0;
};
} // namespace a125::drum
