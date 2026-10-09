#include "DrumCore.h"
#include <array>
#include <cstdlib>
#include <cmath>
#include <iostream>
static void verify(bool ok) { if (!ok) std::exit(EXIT_FAILURE); }
int main() {
    using namespace a125::drum;
    Core core; core.prepare(48000);
    std::array<float, 512> x{},y{},a{},b{};
    for (std::size_t i=0;i<x.size();++i) {
        x[i]=0.5f*std::sin(float(i)*0.13f);
        y[i]=x[i];
    }
    core.process(x.data(),y.data(),a.data(),b.data(),x.size());
    for(std::size_t i=0;i<x.size();++i) {
        verify(a[i]==x[i] && b[i]==y[i]); // exact neutral
    }
    Controls c; c.punch=0.5f;c.body=0.4f;c.tight=0.3f;
    c.finish=0.2f;c.glue=0.5f;
    core.setControls(c);core.reset();
    core.process(x.data(),y.data(),a.data(),b.data(),x.size());
    bool changed=false;
    for(std::size_t i=0;i<x.size();++i) {
        verify(std::isfinite(a[i]) && std::isfinite(b[i]));
        verify(a[i]==b[i]); // channel symmetry
        if(a[i]!=x[i]) changed=true;
    }
    verify(changed);
    core.reset();
    core.process(x.data(),y.data(),a.data(),b.data(),x.size());
    std::array<float,512> reference=a;
    core.reset();
    core.process(x.data(),y.data(),a.data(),b.data(),x.size());
    verify(reference==a); // reset deterministic
    // PUNCH must preserve interchannel balance with asymmetric stereo input.
    Controls punchOnly; punchOnly.punch=1.0f;
    core.setControls(punchOnly); core.reset();
    for (std::size_t i=0;i<x.size();++i) y[i]=0.35f*x[i];
    core.process(x.data(),y.data(),a.data(),b.data(),x.size());
    bool punchActive=false;
    for (std::size_t i=0;i<x.size();++i) {
        verify(std::isfinite(a[i]) && std::isfinite(b[i]));
        verify(std::abs(b[i]-0.35f*a[i])<0.000002f);
        if (std::abs(a[i]-x[i])>0.000001f) punchActive=true;
    }
    verify(punchActive);
    // TIGHT must apply identical gain to different-level left and right
    // programme material, preserving stereo balance through the decay.
    Controls tightOnly; tightOnly.tight=1.0f;
    core.setControls(tightOnly); core.reset();
    core.process(x.data(),y.data(),a.data(),b.data(),x.size());
    bool tightActive=false;
    for (std::size_t i=0;i<x.size();++i) {
        verify(std::isfinite(a[i]) && std::isfinite(b[i]));
        verify(std::abs(b[i]-0.35f*a[i])<0.000002f);
        if (std::abs(a[i]-x[i])>0.000001f) tightActive=true;
    }
    verify(tightActive);
    // On a 0% -> PUNCH automation transition, envelope state should be
    // identical to the same programme processed continuously with PUNCH on.
    Core automation; automation.prepare(48000);
    Core continuous; continuous.prepare(48000);
    Controls on; on.punch=1.0f;
    continuous.setControls(on);
    std::array<float,512> dryLeft{},dryRight{},outA{},outB{},refL{},refR{};
    for (std::size_t i=0;i<dryLeft.size();++i) {
        dryLeft[i]=0.5f*std::sin(float(i)*0.12f);
        dryRight[i]=0.35f*dryLeft[i];
    }
    for (int block=0;block<8;++block) {
        automation.process(dryLeft.data(),dryRight.data(),outA.data(),outB.data(),dryLeft.size());
        continuous.process(dryLeft.data(),dryRight.data(),refL.data(),refR.data(),dryLeft.size());
    }
    automation.setControls(on);
    automation.process(dryLeft.data(),dryRight.data(),outA.data(),outB.data(),dryLeft.size());
    continuous.process(dryLeft.data(),dryRight.data(),refL.data(),refR.data(),dryLeft.size());
    for (std::size_t i=0;i<dryLeft.size();++i) {
        verify(outA[i]==refL[i] && outB[i]==refR[i]);
    }
    // MASS must act on kick fundamentals around 75 Hz without requiring
    // high-level upper-mid content. Preserve finite stereo output.
    Controls massOnly; massOnly.body=1.0f;
    core.setControls(massOnly); core.reset();
    double dryEnergy=0.0, wetEnergy=0.0;
    for (std::size_t i=0;i<x.size();++i) {
        x[i]=0.25f*std::sin(float(i)*float(2.0*3.141592653589793*75.0/48000.0));
        y[i]=x[i];
    }
    core.process(x.data(),y.data(),a.data(),b.data(),x.size());
    for(std::size_t i=128;i<x.size();++i) {
        verify(std::isfinite(a[i]) && std::isfinite(b[i]));
        verify(a[i]==b[i]);
        dryEnergy+=double(x[i])*x[i];
        wetEnergy+=double(a[i])*a[i];
    }
    verify(wetEnergy>dryEnergy);
    // MASS must preserve left/right equality with a polarity-inverted
    // coherent stereo pair; the shared sustain detector must not favor one.
    core.reset();
    for (std::size_t i=0;i<x.size();++i) y[i]=-x[i];
    core.process(x.data(),y.data(),a.data(),b.data(),x.size());
    for(std::size_t i=0;i<x.size();++i) {
        verify(std::isfinite(a[i]) && std::isfinite(b[i]));
        verify(std::abs(a[i]+b[i])<0.000002f);
    }
    std::cout<<"Drum core contract: PASS (neutral/finite/symmetry/determinism/stereo-linked punch and tight)\n";
}
