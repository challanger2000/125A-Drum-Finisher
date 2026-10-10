#include "DrumCore.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

using a125::drum::Controls;
using a125::drum::Core;

int main() {
    // Render independently of the VST3 host: any nonzero output after the
    // source falls silent disproves an IAudioProcessor::kNoTail declaration.
    // Test 44.1/48/96k; 1s must bound the audible residual (< -145dBFS).
    constexpr double pi=3.14159265358979323846;
    for(double fs:{44100.0,48000.0,96000.0}) {
        const size_t total=static_cast<size_t>(fs*2.0);
        const size_t cutoff=static_cast<size_t>(fs*0.86);
        std::vector<float> l(total,0.0f),r(total,0.0f),outL(total),outR(total);
        for(int k=0;k<3;++k) {
            const size_t offset=static_cast<size_t>((0.2+0.23*k)*fs);
            const size_t length=static_cast<size_t>(0.14*fs);
            for(size_t n=0;n<length;++n) {
                double t=double(n)/fs;
                double env=std::exp(-t/0.065);
                l[offset+n]+=float(0.65*env*std::sin(2*pi*95*t));
                r[offset+n]+=float(0.45*env*std::sin(2*pi*137*t));
            }
        }
        double signal=0.0;
        for(int test=0;test<3;++test) {
            Core core;core.prepare(fs);
            Controls c{};
            if(test==0) c.body=1.0f;
            if(test==1) c.finish=1.0f;
            if(test==2) c.body=c.finish=1.0f;
            core.setControls(c);
            for(size_t at=0;at<total;at+=511) {
                size_t n=std::min<size_t>(511,total-at);
                core.process(l.data()+at,r.data()+at,
                             outL.data()+at,outR.data()+at,n);
            }
            double maxAfter=0.0,maxLate=0.0;
            for(size_t i=cutoff;i<total;++i) {
                const double mag=std::max(std::abs(double(outL[i])),
                                          std::abs(double(outR[i])));
                if(!std::isfinite(mag))return 2;
                if(i<cutoff+static_cast<size_t>(0.10*fs))
                    maxAfter=std::max(maxAfter,mag);
                if(i>=cutoff+static_cast<size_t>(1.0*fs))
                    maxLate=std::max(maxLate,mag);
            }
            std::cout<<"TAIL sample_rate="<<fs<<" module="<<test
                     <<" post_input_max="<<maxAfter
                     <<" after_1_sec_max="<<maxLate<<"\n";
            if(maxAfter<1e-6) {
                std::cerr<<"FAIL: fixture did not excite an actual DSP tail\n";
                return 2;
            }
            if(maxLate>1e-7) {
                std::cerr<<"FAIL: nominal one-second tail may be too short\n";
                return 3;
            }
            signal+=maxAfter;
        }
        if(signal<=0.0)return 4;
    }
    std::cout<<"PASS: non-zero MASS/FINISH tail; bounded 1-second VST3 host metadata\n";
    return 0;
}
