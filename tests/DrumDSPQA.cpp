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
    // Non-finite host audio must not propagate even when all effects are off.
    {
        Core clean;
        clean.prepare(48000.0);
        Controls off{};
        clean.setControls(off);
        float aIn[]={std::numeric_limits<float>::quiet_NaN(),
                     std::numeric_limits<float>::infinity(),0.25f};
        float bIn[]={-std::numeric_limits<float>::infinity(),
                     0.5f,0.0f};
        float aOut[3]={},bOut[3]={};
        clean.process(aIn,bIn,aOut,bOut,3);
        check(aOut[0]==0.0f && aOut[1]==0.0f && aOut[2]==0.25f &&
              bOut[0]==0.0f && bOut[1]==0.5f && bOut[2]==0.0f,
              "neutral non-finite input sanitization");
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
    // Full-chain matrix: combinations must remain finite, repeatable and
    // partition-independent. This is a safety contract, not a sonic PASS.
    for(float amount : {0.0f,0.25f,0.5f,0.75f,1.0f}) {
        Controls combined{};
        combined.punch=amount;
        combined.body=amount;
        combined.tight=amount;
        combined.finish=amount;
        combined.glue=amount;
        combined.character=Character::Dense;
        for(double sr : {44100.0,48000.0,96000.0}) {
            for(std::size_t i=0;i<N;++i) {
                double time=double(i)/sr;
                double phase=std::fmod(time,0.5);
                double kick=0.35*std::sin(2*pi*75*time)*std::exp(-phase*24.0);
                double snare=0.16*std::sin(2*pi*210*time)*std::exp(-phase*35.0);
                double cymbal=0.07*std::sin(2*pi*6900*time)*std::exp(-phase*5.0);
                x[i]=float(kick+snare+cymbal);
                y[i]=float(0.65*kick+0.82*snare-0.9*cymbal);
            }
            run(sr,N,combined,x,y,refA,refB,N);
            for(std::size_t block : {std::size_t(1),std::size_t(64),std::size_t(511)}) {
                run(sr,N,combined,x,y,a,b,block);
                check(a==refA && b==refB,"full-chain block invariance");
                check(std::all_of(a.begin(),a.end(),[](float v){return std::isfinite(v);}),
                      "full-chain left finite");
                check(std::all_of(b.begin(),b.end(),[](float v){return std::isfinite(v);}),
                      "full-chain right finite");
            }
            if(amount==0.0f)
                check(refA==x && refB==y,"full-chain zero neutral");
            std::cout<<"full-chain amount="<<amount<<" sampleRate="<<sr
                     <<" left_rms_delta_dB="<<20*std::log10(std::max(1.e-12,rms(refA))/std::max(1.e-12,rms(x)))
                     <<" right_rms_delta_dB="<<20*std::log10(std::max(1.e-12,rms(refB))/std::max(1.e-12,rms(y)))
                     <<" left_peak="<<peak(refA)<<" right_peak="<<peak(refB)<<"\\n";
        }
    }
    if(failures){std::cerr<<"Drum DSP QA: "<<failures<<" FAILED checks\n";return EXIT_FAILURE;}
    std::cout<<"Drum DSP QA: PASS (functional contracts; sound-quality validation pending)\n";
    return EXIT_SUCCESS;
}
