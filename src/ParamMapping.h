#pragma once
#include <algorithm>
#include <cmath>

namespace a125::drum {

// VST3 StringListParameter exposes N=3 discrete indices at normalized
// 0, 1/2 and 1. Processor DSP and Controller must decode intermediates
// identically (nearest step, including automation points between steps).
inline int characterIndex(double normalized) noexcept {
    const double n=std::isfinite(normalized)
        ? std::clamp(normalized,0.0,1.0) : 0.0;
    return std::clamp(static_cast<int>(std::floor(n*2.0+0.5)),0,2);
}

} // namespace a125::drum
