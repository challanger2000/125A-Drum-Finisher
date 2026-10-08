#include "DrumCore.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

using a125::drum::Controls;
using a125::drum::Core;

static constexpr double pi=3.14159265358979323846;
static constexpr double sampleRate=48000.0;
static constexpr std::size_t size=48000;
static double energy(const std::vector<float>& s) {
    double sum=0.0; for(float v:s) sum+=double(v)*v;
    return std::sqrt(sum/double(s.size()));
}
static double db(double x,double y) {
    return 20.0*std::log10(std::max(1e-12,x)/std::max(1e-12,y));
}
static std::vector<float> render(const std::vector<float>& x,Controls c) {
    Core core;core.prepare(sampleRate);core.setControls(c);
    std::vector<float> y(x.size()),right(x.size()),empty(x.size());
    // Explicit varied block sizes to mimic a DAW without altering state.
    for(std::size_t pos=0;pos<x.size();) {
        auto n=std::min(std::size_t(127),x.size()-pos);
        core.process(x.data()+pos,empty.data()+pos,y.data()+pos,right.data()+pos,n);
        pos+=n;
    }
    return y;
}
int main() {
    int fails=0;
    std::vector<float> precolored(size);
    // A synthetic compressed/harmonic-rich drum transient train.
    // This is a regression stimulus, not evidence of actual tube emulation.
    for(std::size_t i=0;i<size;++i) {
        const double time=double(i)/sampleRate;
        const double beat=std::fmod(time,0.25);
        const double envelope=std::exp(-beat*25.0);
        const double fundamental=std::sin(2*pi*90.0*time);
        const double overtones=0.25*std::sin(2*pi*270.0*time)
                              +0.15*std::sin(2*pi*540.0*time);
        precolored[i]=float(0.65*std::tanh(1.6*envelope*(fundamental+overtones)));
    }
    Controls zero{};
    auto neutral=render(precolored,zero);
    if(neutral!=precolored){std::cerr<<"FAIL zero exactness\n";++fails;}
    Controls finish{};finish.finish=0.5f;
    auto cleaned=render(precolored,finish);
    const auto rmsDelta=db(energy(cleaned),energy(precolored));
    std::cout<<"Precolored programme FINISH 50% RMS delta dB: "<<rmsDelta<<"\n";
    if(!std::isfinite(rmsDelta)||rmsDelta< -1.0||rmsDelta>0.5){
        std::cerr<<"FAIL unexpected FINISH level swing\n";++fails;
    }
    Controls glue{};glue.glue=0.5f;
    auto compact=render(precolored,glue);
    const auto glueDb=db(energy(compact),energy(precolored));
    std::cout<<"Precolored programme GLUE 50% RMS delta dB: "<<glueDb<<"\n";
    if(!std::isfinite(glueDb)||glueDb < -3.0||glueDb>0.5){
        std::cerr<<"FAIL unexpected GLUE level swing\n";++fails;
    }
    if(fails)return EXIT_FAILURE;
    std::cout<<"Precolored input regression PASS (synthetic only)\n";
    return EXIT_SUCCESS;
}
