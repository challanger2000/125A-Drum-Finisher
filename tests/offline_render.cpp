#include "DrumCore.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
int main(int argc,char** argv){
    if(argc!=6){std::cerr<<"usage input.f32 output.f32 sampleRate module amount\n";return 2;}
    const char* input=argv[1]; const char* output=argv[2];
    const double sampleRate=std::atof(argv[3]);const std::string module=argv[4];const float amount=std::atof(argv[5]);
    std::ifstream in(input,std::ios::binary|std::ios::ate);
    if(!in){std::cerr<<"missing input\n";return 2;}
    const auto size=in.tellg();
    if(size<0 || static_cast<std::size_t>(size)%8!=0)return 2;
    in.seekg(0);
    const auto frames=static_cast<std::size_t>(size)/8;
    std::vector<float> raw(frames*2);
    in.read(reinterpret_cast<char*>(raw.data()),size);
    std::vector<float> l(frames),r(frames),ol(frames),orr(frames);
    for(std::size_t i=0;i<frames;++i){l[i]=raw[2*i];r[i]=raw[2*i+1];}
    a125::drum::Core core;core.prepare(sampleRate);
    a125::drum::Controls c;
    if(module=="PUNCH")c.punch=amount;
    else if(module=="BODY"||module=="MASS")c.body=amount;
    else if(module=="TIGHT")c.tight=amount;
    else if(module=="FINISH")c.finish=amount;
    else if(module=="GLUE")c.glue=amount;
    else if(module=="CHAR_PUNCH" || module=="CHAR_TIGHT" || module=="CHAR_DENSE") {
        // Reproduce the user's actual screenshot configuration while varying
        // only CHARACTER: PUNCH/MASS/FINISH 100, TIGHT 30, GLUE 50.
        c.punch=1.0f;c.body=1.0f;c.tight=0.30f;c.finish=1.0f;c.glue=0.50f;
        c.character=module=="CHAR_TIGHT"?a125::drum::Character::Tight:
                   (module=="CHAR_DENSE"?a125::drum::Character::Dense:
                   a125::drum::Character::Punch);
    }
    else if(module=="ALL" || module=="ALL_TIGHT" || module=="ALL_DENSE") {
        // Test the *actual* combined chain, rather than summing independent
        // module renders. 100% each is a deliberate worst-case user preset.
        c.punch=c.body=c.tight=c.finish=c.glue=amount;
        if(module=="ALL_TIGHT") c.character=a125::drum::Character::Tight;
        if(module=="ALL_DENSE") c.character=a125::drum::Character::Dense;
    }
    else if(module!="NEUTRAL")return 2;
    core.setControls(c);
    constexpr size_t block=512;
    for(size_t offset=0;offset<frames;offset+=block){
        size_t n=std::min(block,frames-offset);
        core.process(l.data()+offset,r.data()+offset,ol.data()+offset,orr.data()+offset,n);
    }
    for(std::size_t i=0;i<frames;++i){raw[2*i]=ol[i];raw[2*i+1]=orr[i];}
    std::ofstream out(output,std::ios::binary);
    out.write(reinterpret_cast<const char*>(raw.data()),raw.size()*sizeof(float));
    if(!out)return 3;
    return 0;
}
