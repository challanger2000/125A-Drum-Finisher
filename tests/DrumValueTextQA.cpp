#include "gui/ValueText.h"
#include <cmath>
#include <iostream>
#include <string>

int main() {
    int failures=0;
    auto check=[&](unsigned id,double n,const char* expected){
        const auto actual=DrumFinisher::valueText(id,n);
        if(actual!=expected) {
            std::cerr<<"FAIL id="<<id<<" value="<<n
                     <<" expected="<<expected<<" got="<<actual<<"\n";
            ++failures;
        }
    };
    for(unsigned id=100;id<=104;++id) {
        check(id,0.0,"0 %");
        check(id,0.25,"25 %");
        check(id,0.50,"50 %");
        check(id,0.75,"75 %");
        check(id,1.0,"100 %");
    }
    check(105,0.0,"-12.0 dB");
    check(105,0.25,"-6.0 dB");
    check(105,0.5,"0.0 dB");
    check(105,0.75,"+6.0 dB");
    check(105,1.0,"+12.0 dB");
    check(100,std::nan(""),"0 %");
    check(105,std::nan(""),"-12.0 dB");
    check(105,1.5,"+12.0 dB");
    check(999,0.5,"");
    if(failures) return 1;
    std::cout<<"PASS: actual parameter formatter 0-100%, -12/+12 dB, neutral, clamp and nonfinite fallback\n";
    return 0;
}
