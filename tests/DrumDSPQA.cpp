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
    // Independent module isolation and in-place processing contracts.
    // These tests cover real asymmetric stereo content rather than duplicated mono.
    {
        constexpr std::size_t M=16384;
        std::vector<float> l(M),r(M),wetL(M),wetR(M),inPlaceL(M),inPlaceR(M);
        for(std::size_t i=0;i<M;++i) {
            const double seconds=double(i)/48000.0;
            const double onset=std::fmod(seconds,0.17);
            l[i]=float(0.42*std::exp(-onset*23.0)*std::sin(2*pi*81*seconds)
                      +0.10*std::sin(2*pi*1700*seconds));
            r[i]=float(0.24*std::exp(-onset*33.0)*std::sin(2*pi*147*seconds)
                      -0.08*std::sin(2*pi*3700*seconds));
        }
        for(int module=0;module<5;++module) {
            Controls isolated{};
            switch(module) {
                case 0: isolated.punch=1.0f;break;
                case 1: isolated.body=1.0f;break;
                case 2: isolated.tight=1.0f;break;
                case 3: isolated.finish=1.0f;break;
                case 4: isolated.glue=1.0f;break;
            }
            run(48000,M,isolated,l,r,wetL,wetR,256);
            inPlaceL=l;
            inPlaceR=r;
            Core inplace;
            inplace.prepare(48000);
            inplace.setControls(isolated);
            for(std::size_t pos=0;pos<M;pos+=256)
                inplace.process(inPlaceL.data()+pos,inPlaceR.data()+pos,
                                inPlaceL.data()+pos,inPlaceR.data()+pos,
                                std::min(std::size_t(256),M-pos));
            check(inPlaceL==wetL && inPlaceR==wetR,
                  "isolated module in-place equals out-of-place");
            check(std::all_of(wetL.begin(),wetL.end(),[](float v){return std::isfinite(v);}) &&
                  std::all_of(wetR.begin(),wetR.end(),[](float v){return std::isfinite(v);}),
                  "isolated module finite stereo");
            double diff=0.0;
            for(std::size_t i=0;i<M;++i)
                diff+=std::abs(double(wetL[i])-l[i])+std::abs(double(wetR[i])-r[i]);
            std::cout<<"isolated module="<<module
                     <<" mean absolute stereo change="<<diff/(2*M)<<"\\n";
            check(diff>1.e-7,"isolated module actually processes audio");
        }
    }
    // Host lifecycle robustness: invalid sample-rate input must not poison DSP.
    for(double invalidRate : {std::numeric_limits<double>::quiet_NaN(),
                              std::numeric_limits<double>::infinity(),
                              -std::numeric_limits<double>::infinity()}) {
        Core invalid;
        invalid.prepare(invalidRate);
        Controls active{};
        active.punch=active.body=active.tight=active.finish=active.glue=1.0f;
        invalid.setControls(active);
        std::array<float,64> left{},right{},outL{},outR{};
        for(std::size_t i=0;i<left.size();++i) {
            left[i]=float(0.3*std::sin(0.1*double(i)));
            right[i]=float(0.2*std::cos(0.13*double(i)));
        }
        invalid.process(left.data(),right.data(),outL.data(),outR.data(),left.size());
        check(std::all_of(outL.begin(),outL.end(),[](float x){return std::isfinite(x);}),
              "invalid host sample rate left finite");
        check(std::all_of(outR.begin(),outR.end(),[](float x){return std::isfinite(x);}),
              "invalid host sample rate right finite");
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
    // Stereo image contract for dynamics-only combinations: a common gain
    // must preserve the L/R ratio, including asymmetric input and hard panning.
    for (double sr : {44100.0,48000.0,96000.0}) {
        for (float ratio : {0.0f,0.25f,0.65f,1.0f}) {
            for (float amount : {0.25f,0.5f,1.0f}) {
                for (int mode=0; mode<3; ++mode) {
                    Controls c{};
                    if(mode==0 || mode==2) { c.punch=amount; c.tight=amount; }
                    if(mode==1 || mode==2) c.glue=amount;
                    c.character=Character::Punch;
                    for(std::size_t i=0;i<N;++i) {
                        const double seconds=double(i)/sr;
                        const double beat=std::fmod(seconds,0.25);
                        x[i]=float(0.6*std::exp(-beat*30.0)*
                            (std::sin(2*pi*85.0*seconds)+0.25*std::sin(2*pi*3200.0*seconds)));
                        y[i]=ratio*x[i];
                    }
                    run(sr,N,c,x,y,a,b,127);
                    for(std::size_t i=0;i<N;++i) {
                        check(std::isfinite(a[i])&&std::isfinite(b[i]),
                              "stereo-linked dynamics finite");
                        if(std::abs(b[i]-ratio*a[i])>2.e-6f) {
                            check(false,"stereo-linked dynamics image preserved");
                            break;
                        }
                    }
                }
            }
        }
    }
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
