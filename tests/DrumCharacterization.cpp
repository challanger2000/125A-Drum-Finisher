#include "DrumCore.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>
using a125::drum::Controls;
using a125::drum::Core;
constexpr double pi=3.14159265358979323846;
constexpr double fs=48000.0;
constexpr std::size_t length=48000;
struct Result { double rms=0, peak=0, onset=0, tail=0; };
static double energy(const std::vector<float>& a,std::size_t first,std::size_t last) {
    double s=0;for(std::size_t i=first;i<last;++i)s+=double(a[i])*a[i];
    return std::sqrt(s/double(last-first));
}
static Result measure(const std::vector<float>& a) {
    Result r;
    r.rms=energy(a,0,a.size());
    r.onset=energy(a,0,960);
    r.tail=energy(a,4800,9600);
    for(float v:a)r.peak=std::max(r.peak,std::abs(double(v)));
    return r;
}
static double db(double a,double b) {
    return 20.0*std::log10(std::max(1.e-12,a)/std::max(1.e-12,b));
}
static std::vector<float> process(const std::vector<float>& x,Controls c) {
    Core dsp;dsp.prepare(fs);dsp.setControls(c);
    std::vector<float> y(x.size()),silent(x.size()),discard(x.size());
    dsp.process(x.data(),silent.data(),y.data(),discard.data(),x.size());
    return y;
}
int main() {
    std::cout<<std::fixed<<std::setprecision(3);
    int failures=0;
    // Deterministic, independent band-limited synthetic percussion probes.
    // No claim of auditory quality is made from these synthetic fixtures alone.
    const struct Fixture { const char* name;double hz;double decay; } fixtures[]={
       {"kick",65.0,0.080},{"snare_body",210.0,0.110},
       {"snare_presence",2300.0,0.085},{"cymbal",8000.0,0.300}
    };
    const char* names[]={"PUNCH","BODY","TIGHT","FINISH","GLUE"};
    for(const auto& fixture: fixtures) {
        std::vector<float> input(length);
        for(std::size_t i=0;i<length;++i) {
            const double t=double(i)/fs;
            input[i]=float(0.65*std::sin(2*pi*fixture.hz*t)*std::exp(-t/fixture.decay));
        }
        const auto reference=measure(input);
        for(int module=0;module<5;++module) {
            Controls c;
            switch(module) {
                case 0:c.punch=0.5f;break;
                case 1:c.body=0.5f;break;
                case 2:c.tight=0.5f;break;
                case 3:c.finish=0.5f;break;
                case 4:c.glue=0.5f;break;
            }
            const auto y=process(input,c);
            const auto actual=measure(y);
            const double level=db(actual.rms,reference.rms);
            const double attack=db(actual.onset,reference.onset);
            const double sustain=db(actual.tail,reference.tail);
            const double contrast=attack-sustain;
            const double peakDelta=db(actual.peak,reference.peak);
            std::cout<<fixture.name<<" "<<names[module]
                     <<" rms_dB="<<level<<" peak_dB="<<peakDelta
                     <<" attack_dB="<<attack<<" tail_dB="<<sustain
                     <<" attack_minus_tail_dB="<<contrast<<"\n";
            if(!std::isfinite(level)||!std::isfinite(contrast)||!std::isfinite(peakDelta))++failures;
            // Safety-only guard: large unintended gain excursions require review.
            if(std::abs(level)>12.0 || std::abs(peakDelta)>12.0)++failures;
        }
    }
    // The measurements above are diagnostic baselines, not subjective acceptance criteria.
    if(failures) {std::cerr<<"FAIL: "<<failures<<" safety/finite violations\n";return EXIT_FAILURE;}
    std::cout<<"Module characterization: measured. Sonic suitability NOT established.\n";
    return EXIT_SUCCESS;
}
