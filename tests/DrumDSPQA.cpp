#include "DrumCore.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>
#include <cstdlib>

using a125::drum::Core;
using a125::drum::Controls;
using a125::drum::Character;

static int failures=0;
static void check(bool ok,const char* label) {
    if(!ok){std::cerr<<"FAIL: "<<label<<"\n";++failures;}
}
static double rms(const std::vector<float>& v) {
    double sum=0;for(float x:v)sum+=double(x)*x;
    return std::sqrt(sum/std::max<std::size_t>(1,v.size()));
}
static double peak(const std::vector<float>& v){
    double p=0;for(float x:v)p=std::max(p,std::abs(double(x)));return p;
}
static void run(double sr,std::size_t frames,const Controls& c,
                const std::vector<float>& x,const std::vector<float>& y,
                std::vector<float>& a,std::vector<float>& b,
                std::size_t block){
    Core core;core.prepare(sr);core.setControls(c);
    for(std::size_t i=0;i<frames;){
        const auto n=std::min(block,frames-i);
        core.process(x.data()+i,y.data()+i,a.data()+i,b.data()+i,n);
        i+=n;
    }
}
int main(){
    constexpr std::size_t N=16384;
    const double pi=3.14159265358979323846;
    std::vector<float> x(N),y(N),a(N),b(N),refA(N),refB(N);
    for(double sr:{44100.0,48000.0,96000.0,192000.0}){
        for(std::size_t i=0;i<N;++i){
            const double t=i/sr;
            x[i]=float(0.36*std::sin(2*pi*65*t)+0.19*std::sin(2*pi*210*t)
                       +0.10*std::sin(2*pi*5100*t));
            y[i]=float(0.21*std::sin(2*pi*80*t)+0.25*std::sin(2*pi*2900*t));
        }
        Controls zero{};
        for(std::size_t block:{std::size_t(1),std::size_t(7),std::size_t(64),std::size_t(511),std::size_t(2048)}){
            run(sr,N,zero,x,y,a,b,block);
            check(a==x && b==y,"zero amount bit-exact stereo passthrough");
        }
        Controls c{};c.punch=0.5f;c.body=0.35f;c.tight=0.4f;
        c.finish=0.35f;c.glue=0.5f;c.character=Character::Dense;
        run(sr,N,c,x,y,refA,refB,N);
        for(std::size_t block:{std::size_t(1),std::size_t(7),std::size_t(64),std::size_t(511),std::size_t(2048)}){
            run(sr,N,c,x,y,a,b,block);
            check(a==refA && b==refB,"block partition invariance");
        }
        check(std::all_of(a.begin(),a.end(),[](float v){return std::isfinite(v);}),
              "finite left channel");
        check(std::all_of(b.begin(),b.end(),[](float v){return std::isfinite(v);}),
              "finite right channel");
        const auto gainDb=20*std::log10(std::max(1.0e-12,rms(a))/std::max(1.0e-12,rms(x)));
        std::cout<<"sampleRate="<<sr<<" RMS delta dB="<<gainDb
                 <<" left peak="<<peak(a)<<" right peak="<<peak(b)<<"\n";
        check(std::isfinite(gainDb),"finite RMS delta");
        // Symmetric input must remain exactly symmetric (common stereo dynamics).
        run(sr,N,c,x,x,a,b,64);
        check(a==b,"stereo linked symmetric output");
    }
    // The final controller code must not depend on NDEBUG for a test to execute.
    std::vector<float> impulse(N,0),zeros(N,0);
    impulse[0]=0.5f;
    Controls neutral{};
    run(48000,N,neutral,impulse,zeros,a,b,1);
    check(a==impulse&&b==zeros,"single sample transient preservation at neutral");
    Controls bad{};
    bad.punch=std::numeric_limits<float>::quiet_NaN();
    bad.body=2.0f;bad.glue=-2.0f;
    run(48000,N,bad,impulse,zeros,a,b,64);
    check(std::all_of(a.begin(),a.end(),[](float v){return std::isfinite(v);}),
          "invalid control sanitization");
    if(failures){std::cerr<<"Drum DSP QA: "<<failures<<" FAILED checks\n";return EXIT_FAILURE;}
    std::cout<<"Drum DSP QA: PASS (functional contracts; sound-quality validation pending)\n";
    return EXIT_SUCCESS;
}
