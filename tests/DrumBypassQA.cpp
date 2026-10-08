#include "BypassRamp.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
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
    std::cout<<"Bypass 64-sample monotonicity/reversal/stereo: PASS\n";
    return EXIT_SUCCESS;
}
