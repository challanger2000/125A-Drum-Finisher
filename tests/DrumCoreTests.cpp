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
    std::cout<<"Drum core contract: PASS (neutral/finite/symmetry/determinism)\n";
}
