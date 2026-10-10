#include "ParamMapping.h"
#include <cmath>
#include <iostream>
#include <limits>

int main() {
    struct Case{double normalized;int expected;};
    for(const Case c: {
       Case{0,0},Case{0.24,0},Case{0.25,1},Case{0.33,1},
       Case{0.49,1},Case{0.50,1},Case{0.66,1},
       Case{0.74,1},Case{0.75,2},Case{0.80,2},
       Case{1.0,2},Case{-9.0,0},Case{3.0,2},
       Case{std::numeric_limits<double>::quiet_NaN(),0},
       Case{std::numeric_limits<double>::infinity(),0}}) {
        const auto index=a125::drum::characterIndex(c.normalized);
        if(index!=c.expected) {
            std::cerr<<"FAIL normalized CHARACTER step decode: "
                     <<c.normalized<<" actual="<<index
                     <<" expected="<<c.expected<<"\n";
            return 1;
        }
    }
    std::cout<<"PASS: VST3 CHARACTER indices nearest 0/0.5/1 for all intermediate automation values\n";
    return 0;
}
