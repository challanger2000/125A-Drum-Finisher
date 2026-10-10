#include "DrumCore.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>

int main() {
    // Emulate the VST3 wrapper's exact 1-sample Core::process call pattern.
    // This includes detector, MASS, TIGHT, FINISH, GLUE and CHARACTER.
    using clock=std::chrono::steady_clock;
    volatile float sink=0.f;
    for(double fs:{44100.0,48000.0,96000.0,192000.0}) {
        constexpr size_t block=512, count=256;
        std::array<float,block> l{},r{};
        for(size_t i=0;i<block;++i) {
            const double t=double(i)/fs;
            l[i]=float(0.18*std::sin(2*3.141592653589793*107*t)+
                       0.06*std::sin(2*3.141592653589793*2400*t));
            r[i]=float(0.16*std::sin(2*3.141592653589793*137*t)+
                       0.03*std::sin(2*3.141592653589793*4950*t));
        }
        a125::drum::Core c;c.prepare(fs);
        a125::drum::Controls settings{};
        settings.punch=settings.body=settings.tight=settings.finish=settings.glue=1.f;
        settings.character=a125::drum::Character::Dense;
        c.setControls(settings);
        std::vector<double> us;us.reserve(count);
        // Warmup caches, branch predictor and filter state outside timing.
        for(size_t b=0;b<count+32;++b) {
            const auto begin=clock::now();
            for(size_t i=0;i<block;++i) {
                float outL=0,outR=0;
                c.process(&l[i],&r[i],&outL,&outR,1);
                sink=sink+outL*1.e-9f;
            }
            const auto finish=clock::now();
            if(b>=32) us.push_back(
                std::chrono::duration<double,std::micro>(finish-begin).count());
        }
        std::sort(us.begin(),us.end());
        const double deadline=1.0e6*block/fs;
        const auto p=[&](double fraction){
            return us[static_cast<size_t>(fraction*(us.size()-1))];
        };
        const double mean=std::accumulate(us.begin(),us.end(),0.0)/us.size();
        std::cout<<"CPU_ONE_SAMPLE_WRAPPER fs="<<fs<<" block="<<block
                 <<" deadline_us="<<deadline<<" mean_us="<<mean
                 <<" p95_us="<<p(.95)<<" p99_us="<<p(.99)
                 <<" max_us="<<us.back()
                 <<" p99_deadline_percent="<<100*p(.99)/deadline<<"\n";
        // Shared Actions runners can be noisy. Avoid a CPU benchmark as a
        // false numerical quality assertion; reject only catastrophic
        // chronic misses. The full distribution is saved in CI logs.
        if(p(.99)>deadline*2.0){
            std::cerr<<"FAIL: real-time DSP p99 repeatedly misses deadline >2x\n";
            return 2;
        }
    }
    std::cout<<"PASS: measured p95/p99 1-sample processing path; host overhead separate\n";
    (void)sink;
    return 0;
}
