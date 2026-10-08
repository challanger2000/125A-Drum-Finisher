#include "DrumCore.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "base/source/fstreamer.h"
#include <algorithm>
#include <cstdint>
#include <cmath>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace a125::drum::vst {
static const FUID processorId(0xD910BA41,0x833A4E12,0xA31199D0,0x73810701);
static const FUID controllerId(0xD910BA41,0x833A4E12,0xA31199D0,0x73810702);
enum Param : ParamID { kPunch=100, kBody=101, kTight=102, kFinish=103,
                      kGlue=104, kOutput=105, kCharacter=106, kBypass=107 };
static constexpr int32 stateVersion=1;
static double clampUnit(double value) { return std::isfinite(value) ? std::clamp(value,0.0,1.0) : 0.0; }
static void apply(Controls& c, ParamID id, double value, bool& bypass) {
    value=clampUnit(value);
    switch(id) {
        case kPunch:c.punch=static_cast<float>(value);break;
        case kBody:c.body=static_cast<float>(value);break;
        case kTight:c.tight=static_cast<float>(value);break;
        case kFinish:c.finish=static_cast<float>(value);break;
        case kGlue:c.glue=static_cast<float>(value);break;
        case kOutput:c.outputDb=static_cast<float>(24.0*value-12.0);break;
        case kCharacter:c.character=static_cast<Character>(std::clamp(static_cast<int>(value*3.0),0,2));break;
        case kBypass:bypass=value>=0.5;break;
        default:break;
    }
}
static double normalized(const Controls& c, ParamID id, bool bypass) {
    switch(id) {
        case kPunch:return c.punch;case kBody:return c.body;case kTight:return c.tight;
        case kFinish:return c.finish;case kGlue:return c.glue;
        case kOutput:return (c.outputDb+12.0)/24.0;
        case kCharacter:return static_cast<int>(c.character)/2.0;
        case kBypass:return bypass?1.0:0.0;
        default:return 0.0;
    }
}
static bool saveState(IBStream* stream, const Controls& c, bool bypass) {
    if (!stream) return false;
    IBStreamer writer(stream,kLittleEndian);
    if (!writer.writeInt32(stateVersion))return false;
    for (ParamID id : {kPunch,kBody,kTight,kFinish,kGlue,kOutput,kCharacter,kBypass})
        if (!writer.writeDouble(normalized(c,id,bypass))) return false;
    return true;
}
static bool loadState(IBStream* stream, Controls& c, bool& bypass) {
    if (!stream)return false;
    IBStreamer reader(stream,kLittleEndian);
    int32 version=0;
    if (!reader.readInt32(version)||version!=stateVersion)return false;
    Controls incoming{};
    bool incomingBypass=false;
    for (ParamID id : {kPunch,kBody,kTight,kFinish,kGlue,kOutput,kCharacter,kBypass}) {
        double value=0;
        if (!reader.readDouble(value)||!std::isfinite(value))return false;
        apply(incoming,id,value,incomingBypass);
    }
    c=incoming;bypass=incomingBypass;
    return true;
}

class Processor final : public AudioEffect {
public:
    Processor(){setControllerClass(controllerId);}
    static FUnknown* createInstance(void*) {return static_cast<IAudioProcessor*>(new Processor);}
    tresult PLUGIN_API initialize(FUnknown* ctx) override {
        auto result=AudioEffect::initialize(ctx);
        if (result!=kResultOk)return result;
        addAudioInput(STR16("Stereo Input"),SpeakerArr::kStereo);
        addAudioOutput(STR16("Stereo Output"),SpeakerArr::kStereo);
        return kResultOk;
    }
    tresult PLUGIN_API setupProcessing(ProcessSetup& setup) override {
        if (setup.symbolicSampleSize!=kSample32)return kResultFalse;
        core_.prepare(setup.sampleRate);
        return AudioEffect::setupProcessing(setup);
    }
    tresult PLUGIN_API setActive(TBool state) override {
        if(state)core_.reset();
        return AudioEffect::setActive(state);
    }
    tresult PLUGIN_API canProcessSampleSize(int32 symbolic) override {
        return symbolic==kSample32?kResultTrue:kResultFalse;
    }
    tresult PLUGIN_API setBusArrangements(SpeakerArrangement* ins,int32 nin,
        SpeakerArrangement* outs,int32 nout) override {
        if(nin!=1||nout!=1||ins[0]!=SpeakerArr::kStereo||outs[0]!=SpeakerArr::kStereo)
            return kResultFalse;
        return AudioEffect::setBusArrangements(ins,nin,outs,nout);
    }
    tresult PLUGIN_API process(ProcessData& data) override {
        if(data.inputParameterChanges) {
            auto* changes=data.inputParameterChanges;
            for(int32 i=0;i<changes->getParameterCount();++i) {
                auto* queue=changes->getParameterData(i);
                if(!queue||queue->getPointCount()<=0)continue;
                int32 offset=0;ParamValue value=0;
                if(queue->getPoint(queue->getPointCount()-1,offset,value)==kResultOk)
                    apply(controls_,queue->getParameterId(),value,bypass_);
            }
        }
        core_.setControls(controls_);
        if(data.numSamples<=0||data.numInputs<1||data.numOutputs<1)return kResultOk;
        auto& in=data.inputs[0];auto& out=data.outputs[0];
        if(in.numChannels!=2||out.numChannels!=2||!in.channelBuffers32||!out.channelBuffers32)
            return kResultFalse;
        if(!in.channelBuffers32[0]||!in.channelBuffers32[1]||
           !out.channelBuffers32[0]||!out.channelBuffers32[1])return kResultFalse;
        if(bypass_) {
            for(int32 i=0;i<data.numSamples;++i) {
                out.channelBuffers32[0][i]=in.channelBuffers32[0][i];
                out.channelBuffers32[1][i]=in.channelBuffers32[1][i];
            }
        } else {
            core_.process(in.channelBuffers32[0],in.channelBuffers32[1],
                out.channelBuffers32[0],out.channelBuffers32[1],
                static_cast<std::size_t>(data.numSamples));
        }
        out.silenceFlags=0;
        return kResultOk;
    }
    tresult PLUGIN_API getState(IBStream* stream) override {
        return saveState(stream,controls_,bypass_)?kResultOk:kResultFalse;
    }
    tresult PLUGIN_API setState(IBStream* stream) override {
        Controls c=controls_; bool bypass=bypass_;
        if(!loadState(stream,c,bypass))return kResultFalse;
        controls_=c;bypass_=bypass;core_.setControls(c);core_.reset();
        return kResultOk;
    }
private:
    Core core_{};
    Controls controls_{};
    bool bypass_=false;
};
class Controller final : public EditController {
public:
    static FUnknown* createInstance(void*) {return static_cast<IEditController*>(new Controller);}
    tresult PLUGIN_API initialize(FUnknown* context) override {
        auto result=EditController::initialize(context);
        if(result!=kResultOk)return result;
        parameters.addParameter(STR16("Punch"),STR16("%"),0,0,ParameterInfo::kCanAutomate,kPunch);
        parameters.addParameter(STR16("Body"),STR16("%"),0,0,ParameterInfo::kCanAutomate,kBody);
        parameters.addParameter(STR16("Tight"),STR16("%"),0,0,ParameterInfo::kCanAutomate,kTight);
        parameters.addParameter(STR16("Finish"),STR16("%"),0,0,ParameterInfo::kCanAutomate,kFinish);
        parameters.addParameter(STR16("Glue"),STR16("%"),0,0,ParameterInfo::kCanAutomate,kGlue);
        parameters.addParameter(STR16("Output"),STR16("dB"),0,0.5,ParameterInfo::kCanAutomate,kOutput);
        parameters.addParameter(STR16("Character"),nullptr,2,0.5,ParameterInfo::kCanAutomate,kCharacter);
        parameters.addParameter(STR16("Bypass"),nullptr,1,0,ParameterInfo::kCanAutomate|ParameterInfo::kIsBypass,kBypass);
        return kResultOk;
    }
    tresult PLUGIN_API setComponentState(IBStream* stream) override {
        Controls c;bool bypass=false;
        if(!loadState(stream,c,bypass))return kResultFalse;
        for(ParamID id : {kPunch,kBody,kTight,kFinish,kGlue,kOutput,kCharacter,kBypass})
            setParamNormalized(id,normalized(c,id,bypass));
        return kResultOk;
    }
};
} // namespace a125::drum::vst

BEGIN_FACTORY_DEF("125A Systems","https://github.com/challanger2000","")
DEF_CLASS2(INLINE_UID_FROM_FUID(a125::drum::vst::processorId),
    PClassInfo::kManyInstances,kVstAudioEffectClass,"125A Drum Finisher",
    Vst::kDistributable,"Fx", "1.0.0", kVstVersionString,
    a125::drum::vst::Processor::createInstance)
DEF_CLASS2(INLINE_UID_FROM_FUID(a125::drum::vst::controllerId),
    PClassInfo::kManyInstances,kVstComponentControllerClass,"125A Drum Finisher Controller",
    0,"","1.0.0",kVstVersionString,a125::drum::vst::Controller::createInstance)
END_FACTORY
