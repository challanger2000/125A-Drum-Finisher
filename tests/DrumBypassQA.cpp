#include "BypassRamp.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
int main() {
    a125::drum::BypassRamp ramp;
    ramp.reset(false);
    double previous=0.0;
    for(int i=0;i<64;++i) {
        const double p=ramp.advance(true);
        if(p<previous || p-previous>1.0/64.0+1e-12) return EXIT_FAILURE;
        previous=p;
    }
    if(std::abs(ramp.position()-1.0)>1e-12)return EXIT_FAILURE;
    for(int i=0;i<64;++i)ramp.advance(false);
    if(std::abs(ramp.position())>1e-12)return EXIT_FAILURE;
    ramp.reset(false);
    for(int i=0;i<20;++i)ramp.advance(true);
    for(int i=0;i<20;++i)ramp.advance(false);
    if(std::abs(ramp.position())>1e-12)return EXIT_FAILURE;
    // Same ramp for both stereo channels: verify only one advance per frame.
    ramp.reset(false);
    double mix=ramp.advance(true);
    double left=(1.0-mix)*0.3+mix*0.6;
    double right=(1.0-mix)*0.1+mix*0.2;
    if(std::abs(left-0.3046875)>1e-12||std::abs(right-0.1015625)>1e-12)
        return EXIT_FAILURE;
    // Host-supplied nonfinite samples must not reach the output via dry bypass.
    if(a125::drum::finiteHostSample(std::numeric_limits<float>::quiet_NaN())!=0.0f)
        return EXIT_FAILURE;
    if(a125::drum::finiteHostSample(std::numeric_limits<double>::infinity())!=0.0)
        return EXIT_FAILURE;
    if(a125::drum::finiteHostSample(-0.25f)!=-0.25f)
        return EXIT_FAILURE;
    std::cout<<"Bypass ramp/stereo/nonfinite host sanitization: PASS\n";
    return EXIT_SUCCESS;
}
