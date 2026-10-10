#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <string>

namespace DrumFinisher {

// Format the five 0..100% controls and +/-12dB output exactly as users
// read them BELOW the knobs. Shared by the VST3 editor and offline CTest.
inline std::string valueText(std::uint32_t id, double normalized) {
    const double unit=std::isfinite(normalized)
        ? std::clamp(normalized,0.0,1.0) : 0.0;
    char text[32]{};
    if(id>=100 && id<=104) {
        std::snprintf(text,sizeof(text),"%.0f %%",unit*100.0);
    } else if(id==105) {
        const double db=24.0*unit-12.0;
        if(std::abs(db)<0.05) std::snprintf(text,sizeof(text),"0.0 dB");
        else std::snprintf(text,sizeof(text),"%+.1f dB",db);
    }
    return text;
}
} // namespace DrumFinisher
