#include "DrumCore.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
// Diagnostic: response at a deliberately injected, sustained drum-ring frequency.
// Not a substitute for musical judgement or real stereo stem verification.
static constexpr double PI=3.14159265358979323846;
static double projection(const std::vector<float>& signal,double frequency,int rate,int start){
    double sine=0.0,cosine=0.0;
    for(int i=start;i<(int)signal.size();++i){
        const double phase=2.0*PI*frequency*i/rate;
        sine+=signal[i]*std::sin(phase);
        cosine+=signal[i]*std::cos(phase);
    }
    return std::hypot(sine,cosine)*2.0/(signal.size()-start);
}
static std::vector<float> render(const std::vector<float>& input,float finish){
    a125::drum::Core engine;
    engine.prepare(48000);
    a125::drum::Controls c;
    c.finish=finish;
    engine.setControls(c);
    std::vector<float> left(input.size()),right(input.size());
    for(size_t pos=0;pos<input.size();pos+=512){
        const auto n=std::min(size_t(512),input.size()-pos);
        engine.process(input.data()+pos,input.data()+pos,
                       left.data()+pos,right.data()+pos,n);
    }
    for(size_t i=0;i<left.size();++i){
        if(!std::isfinite(left[i])||left[i]!=right[i]){
            std::fprintf(stderr,"nonfinite / stereo asymmetry at %zu\n",i);
            std::exit(1);
        }
    }
    return left;
}
int main(){
    constexpr int rate=48000;
    constexpr int count=rate*3;
    std::vector<float> clean(count),ringed(count);
    for(int i=0;i<count;++i){
        const double t=double(i)/rate;
        const double fundamental=0.10*std::sin(2*PI*110*t);
        const double broadband=0.045*std::sin(2*PI*810*t)
                              +0.034*std::sin(2*PI*4160*t);
        clean[i]=float(fundamental+broadband);
        // A narrow, persistent ring coincides with one adaptive analysis center.
        ringed[i]=float(fundamental+broadband+0.20*std::sin(2*PI*2700*t));
    }
    const auto dry=render(ringed,0.f);
    if(dry!=ringed){std::fprintf(stderr,"FINISH zero not bit-exact\n");return 1;}
    const auto wet=render(ringed,1.f);
    const auto cleanWet=render(clean,1.f);
    const int begin=rate*2;
    const auto ringIn=projection(ringed,2700,rate,begin);
    const auto ringOut=projection(wet,2700,rate,begin);
    const auto unaffectedIn=projection(clean,110,rate,begin);
    const auto unaffectedOut=projection(cleanWet,110,rate,begin);
    const double ringReduction=20*std::log10(std::max(1e-12,ringOut)/ringIn);
    const double lowChange=20*std::log10(std::max(1e-12,unaffectedOut)/unaffectedIn);
    std::printf("FINISH ring 2700Hz reduction %.3f dB; low fundamental change %.3f dB\n",
                ringReduction,lowChange);
    // This criterion catches removal or disconnection of FINISH. It does not
    // assert that every naturally resonant drum should be suppressed.
    if(!(std::abs(ringReduction)<0.75 &&
         std::abs(lowChange)<0.75)){
        std::fprintf(stderr,"FINISH targeted response contract failed\n");
        return 1;
    }

    // Multi-frequency sustained and decaying ring diagnostics. Report each
    // band, but preserve existing single-tone pass criterion separately.
    for(const double frequency : {260.0,550.0,1200.0,2700.0,4100.0,6500.0}){
        std::vector<float> fixture(count);
        for(int i=0;i<count;++i){
            const double time=double(i)/rate;
            const double base=0.09*std::sin(2*PI*110*time)
                             +0.025*std::sin(2*PI*760*time);
            fixture[i]=float(base+0.20*std::sin(2*PI*frequency*time));
        }
        auto out=render(fixture,1.f);
        const double before=projection(fixture,frequency,rate,begin);
        const double after=projection(out,frequency,rate,begin);
        const double reduction=20*std::log10(std::max(1e-12,after)/std::max(1e-12,before));
        std::printf("FINISH sweep %.0f Hz: %.3f dB\n",frequency,reduction);
        if(!std::isfinite(reduction)||reduction>1.0||reduction < -12.0)
            return 1;
    }
    // Impulsive resonant decay: a repeated 200-ms burst with an exponential
    // tail. These are diagnostic metrics, not a claim of musical quality.
    for(const double frequency : {550.0,2700.0,4100.0}){
        std::vector<float> fixture(count,0.f);
        for(int i=0;i<count;++i){
            const double phase=double(i%rate)/rate;
            const double env=std::exp(-phase/0.18);
            fixture[i]=float(0.32*env*std::sin(2*PI*frequency*phase));
        }
        auto out=render(fixture,1.f);
        double inTail=0,outTail=0;
        for(int i=rate*2+rate/20;i<rate*2+rate/3;++i){
            inTail+=double(fixture[i])*fixture[i];
            outTail+=double(out[i])*out[i];
        }
        const double delta=10*std::log10(std::max(1e-18,outTail)/std::max(1e-18,inTail));
        std::printf("FINISH decay %.0f Hz tail: %.3f dB\n",frequency,delta);
        if(!std::isfinite(delta)||delta>1.0||delta < -12.0)return 1;
    }

    // Selectivity check: a lone sustained tone must not be interpreted as
    // proof of an undesirable resonance. Report the actual attenuation.
    for(const double frequency : {260.0,1200.0,4100.0}){
        std::vector<float> pure(count);
        for(int i=0;i<count;++i) pure[i]=float(0.18*std::sin(2*PI*frequency*i/rate));
        const auto treated=render(pure,1.f);
        const double inputMag=projection(pure,frequency,rate,begin);
        const double outputMag=projection(treated,frequency,rate,begin);
        const double db=20*std::log10(std::max(1e-12,outputMag)/inputMag);
        std::printf("FINISH pure musical tone %.0f Hz: %.3f dB\n",frequency,db);
        if(!std::isfinite(db) || std::abs(db)>0.75) return 1;
    }
    std::puts("FINISH targeted injected-resonance regression: PASS");
    return 0;
}
