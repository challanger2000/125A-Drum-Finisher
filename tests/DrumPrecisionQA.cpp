#include "DrumCore.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
int main() {
    a125::drum::Core f,d;
    f.prepare(48000);d.prepare(48000);
    a125::drum::Controls c;c.punch=0.4f;c.body=0.3f;c.glue=0.2f;c.finish=0.25f;
    f.setControls(c);d.setControls(c);
    constexpr int n=4096;
    std::vector<float> in32(n),out32(n),zero32(n),other32(n);
    std::vector<double> in64(n),out64(n),zero64(n),other64(n);
    for(int i=0;i<n;++i){in32[i]=float(.5*std::sin(i*.03));in64[i]=in32[i];}
    f.process(in32.data(),zero32.data(),out32.data(),other32.data(),n);
    d.process(in64.data(),zero64.data(),out64.data(),other64.data(),n);
    for(int i=0;i<n;++i){
        if(!std::isfinite(out64[i]) || std::abs(out64[i]-out32[i])>0.000005) {
            std::cerr<<"FAIL precision consistency sample "<<i<<"\n";return EXIT_FAILURE;
        }
    }
    a125::drum::Controls neutral;
    d.reset();d.setControls(neutral);
    d.process(in64.data(),zero64.data(),out64.data(),other64.data(),n);
    if(out64!=in64 || other64!=zero64){std::cerr<<"FAIL double neutral\n";return EXIT_FAILURE;}
    std::cout<<"32/64 DSP consistency and 64-bit neutral: PASS\n";
    return EXIT_SUCCESS;
}
