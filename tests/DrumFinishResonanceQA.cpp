#include "DrumCore.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

int main() {
    constexpr int n=48000;
    constexpr double pi=3.14159265358979323846;
    std::vector<float> in(n), zero(n), neutral(n), processed(n), right(n);
    for(int i=0;i<n;++i) {
        const double t=double(i)/48000.0;
        in[i]=float(0.23*std::sin(2*pi*380*t)+0.13*std::sin(2*pi*2700*t));
    }
    a125::drum::Core core;
    core.prepare(48000.0);
    a125::drum::Controls c;
    core.setControls(c);
    core.process(in.data(),zero.data(),neutral.data(),right.data(),n);
    if(neutral!=in||right!=zero) {
        std::cerr<<"FAIL: FINISH 0% does not preserve bit-exact signal\n";
        return EXIT_FAILURE;
    }
    core.reset();
    c.finish=1.0f;
    core.setControls(c);
    core.process(in.data(),zero.data(),processed.data(),right.data(),n);
    double inputEnergy=0.0,outputEnergy=0.0,differenceEnergy=0.0;
    for(int i=n/2;i<n;++i) {
        inputEnergy+=double(in[i])*in[i];
        outputEnergy+=double(processed[i])*processed[i];
        const double d=processed[i]-in[i];
        differenceEnergy+=d*d;
        if(!std::isfinite(processed[i])||!std::isfinite(right[i]))return EXIT_FAILURE;
    }
    const auto delta=10*std::log10(outputEnergy/inputEnergy);
    const auto relativeDifference=std::sqrt(differenceEnergy/inputEnergy);
    std::cout<<"FINISH 100% sustained two-tone: RMS delta dB="<<delta
             <<", normalized residual="<<relativeDifference<<"\n";
    // A stationary two-tone signal has no repeated drum onset.
    // FINISH is now decay/onset-gated: preserve such musical sustained tones.
    // Deliberately injected drum-ring decays are tested separately in
    // DrumFinishTargetedQA.cpp, where a nonzero suppression is mandatory.
    if(!std::isfinite(delta)||!std::isfinite(relativeDifference)||
       std::abs(delta)>0.15||relativeDifference>0.02) {
        std::cerr<<"FAIL: FINISH altered protected sustained tones\n";
        return EXIT_FAILURE;
    }
    std::cout<<"FINISH sustained-tone protection: PASS\n";
    return EXIT_SUCCESS;
}
