#include "DrumCore.h"
#include "gui/SteelKnob.h"
#include "gui/BrandLogoView.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/controls/cbuttons.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "vstgui/uidescription/uiattributes.h"
#include <cstring>
#include <vector>
#include "BypassRamp.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "base/source/fstreamer.h"
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <array>
#include <utility>

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
        if (setup.symbolicSampleSize!=kSample32 && setup.symbolicSampleSize!=kSample64)return kResultFalse;
        core_.prepare(setup.sampleRate);
        return AudioEffect::setupProcessing(setup);
    }
    tresult PLUGIN_API setActive(TBool state) override {
        if(state){core_.reset(); bypassRamp_.reset(bypass_);}
        return AudioEffect::setActive(state);
    }
    tresult PLUGIN_API setProcessing(TBool state) override {
        // The SDK base returns kNotImplemented. Hosts require a successful
        // processing-state transition after setupProcessing and activation.
        if(state){
            core_.reset();
            bypassRamp_.reset(bypass_);
        }
        AudioEffect::setProcessing(state);
        return kResultTrue;
    }
    tresult PLUGIN_API canProcessSampleSize(int32 symbolic) override {
        return (symbolic==kSample32||symbolic==kSample64)?kResultTrue:kResultFalse;
    }
    tresult PLUGIN_API setBusArrangements(SpeakerArrangement* ins,int32 nin,
        SpeakerArrangement* outs,int32 nout) override {
        if(nin!=1||nout!=1||ins[0]!=SpeakerArr::kStereo||outs[0]!=SpeakerArr::kStereo)
            return kResultFalse;
        return AudioEffect::setBusArrangements(ins,nin,outs,nout);
    }
    tresult PLUGIN_API process(ProcessData& data) override {
        // Eight bounded parameter cursors; VST3 queues specify offsets inside the block.
        std::array<Cursor,8> cursors{};
        int used=0;
        if(data.inputParameterChanges){
            auto* changes=data.inputParameterChanges;
            for(int32 i=0;i<changes->getParameterCount();++i){
                auto* q=changes->getParameterData(i);
                if(!q||q->getPointCount()<=0)continue;
                const auto id=q->getParameterId();
                if(id<kPunch||id>kBypass)continue;
                cursors[static_cast<std::size_t>(id-kPunch)]={q,0,q->getPointCount()};
                ++used;
            }
        }
        (void)used;
        if(data.numSamples<=0) {
            // Process zero-frame parameter changes without touching audio buffers.
            for(auto& cursor:cursors) if(cursor.queue){
                int32 offset=0;ParamValue value=0;
                if(cursor.queue->getPoint(cursor.count-1,offset,value)==kResultOk)
                    apply(controls_,cursor.queue->getParameterId(),value,bypass_);
            }
            core_.setControls(controls_);
            return kResultOk;
        }
        if(data.numInputs<1||data.numOutputs<1)return kResultFalse;
        auto& in=data.inputs[0];auto& out=data.outputs[0];
        if(in.numChannels!=2||out.numChannels!=2)return kResultFalse;
        if(data.symbolicSampleSize==kSample32){
            if(!in.channelBuffers32||!out.channelBuffers32||
               !in.channelBuffers32[0]||!in.channelBuffers32[1]||
               !out.channelBuffers32[0]||!out.channelBuffers32[1])return kResultFalse;
            render(in.channelBuffers32,out.channelBuffers32,data.numSamples,cursors);
        }else if(data.symbolicSampleSize==kSample64){
            if(!in.channelBuffers64||!out.channelBuffers64||
               !in.channelBuffers64[0]||!in.channelBuffers64[1]||
               !out.channelBuffers64[0]||!out.channelBuffers64[1])return kResultFalse;
            render(in.channelBuffers64,out.channelBuffers64,data.numSamples,cursors);
        }else return kResultFalse;
        out.silenceFlags=0;
        for(int32 ch=0;ch<2;++ch){
            bool silent=true;
            for(int32 i=0;i<data.numSamples;++i){
                const double v=data.symbolicSampleSize==kSample32?
                    out.channelBuffers32[ch][i]:out.channelBuffers64[ch][i];
                if(v!=0.0){silent=false;break;}
            }
            if(silent)out.silenceFlags|=(uint64(1)<<ch);
        }
        return kResultOk;
    }
    tresult PLUGIN_API getState(IBStream* stream) override {
        return saveState(stream,controls_,bypass_)?kResultOk:kResultFalse;
    }
    tresult PLUGIN_API setState(IBStream* stream) override {
        Controls c=controls_; bool bypass=bypass_;
        if(!loadState(stream,c,bypass))return kResultFalse;
        controls_=c;bypass_=bypass;core_.setControls(c);core_.reset();
        bypassRamp_.reset(bypass_);
        return kResultOk;
    }
private:
    struct Cursor { IParamValueQueue* queue=nullptr; int32 next=0,count=0; };
    template<class Sample>
    void render(Sample** in,Sample** out,int32 count,
                std::array<Cursor,8>& cursors) noexcept {
        for(int32 i=0;i<count;++i){
            bool changed=false;
            for(auto& cursor:cursors){
                if(!cursor.queue)continue;
                while(cursor.next<cursor.count){
                    int32 offset=0;ParamValue value=0;
                    if(cursor.queue->getPoint(cursor.next,offset,value)!=kResultOk){
                        cursor.next=cursor.count;break;
                    }
                    if(offset>i)break;
                    apply(controls_,cursor.queue->getParameterId(),value,bypass_);
                    ++cursor.next;changed=true;
                }
            }
            if(changed||i==0)core_.setControls(controls_);
            // The bypass crossfade must not reintroduce NaN/Inf from the
            // original host buffer after DrumCore sanitizes the wet signal.
            // Preserve valid finite values bit-for-bit at full bypass.
            const Sample dryL=a125::drum::finiteHostSample(in[0][i]);
            const Sample dryR=a125::drum::finiteHostSample(in[1][i]);
            Sample wetL=0,wetR=0;
            // Continue the wet engine while bypassed, avoiding cold state on return.
            core_.process(&dryL,&dryR,&wetL,&wetR,1);
            const double blend=bypassRamp_.advance(bypass_);
            out[0][i]=static_cast<Sample>((1.0-blend)*wetL+blend*dryL);
            out[1][i]=static_cast<Sample>((1.0-blend)*wetR+blend*dryR);
        }
    }
    Core core_{};
    Controls controls_{};
    bool bypass_=false;
    a125::drum::BypassRamp bypassRamp_{};
};
class Controller final : public EditController, public VSTGUI::VST3EditorDelegate, public VSTGUI::IControlListener {
public:
    static FUnknown* createInstance(void*) {return static_cast<IEditController*>(new Controller);}
    tresult PLUGIN_API initialize(FUnknown* context) override {
        auto result=EditController::initialize(context);
        if(result!=kResultOk)return result;
        // Plain values match the units shown in host automation and GUI.
        for(const auto& spec : std::array<std::pair<const TChar*,ParamID>,5>{{
            {STR16("Punch"),kPunch},{STR16("Mass"),kBody},
            {STR16("Tight"),kTight},{STR16("Finish"),kFinish},
            {STR16("Glue"),kGlue}}}) {
            auto* param=new RangeParameter(spec.first,spec.second,STR16("%"),
                0.0,100.0,0.0,0,ParameterInfo::kCanAutomate);
            param->setPrecision(0);
            parameters.addParameter(param);
        }
        auto* output=new RangeParameter(STR16("Output"),kOutput,STR16("dB"),
            -12.0,12.0,0.0,0,ParameterInfo::kCanAutomate);
        output->setPrecision(1);
        parameters.addParameter(output);
        auto* character=new StringListParameter(STR16("Character"),kCharacter);
        character->appendString(STR16("TIGHT"));
        character->appendString(STR16("PUNCH"));
        character->appendString(STR16("DENSE"));
        character->getInfo().defaultNormalizedValue=0.5;
        character->setNormalized(0.5);
        parameters.addParameter(character);
        parameters.addParameter(STR16("Bypass"),nullptr,1,0,ParameterInfo::kCanAutomate|ParameterInfo::kIsBypass,kBypass);
        return kResultOk;
    }
    IPlugView* PLUGIN_API createView(FIDString name) override {
        if(!name || std::strcmp(name,ViewType::kEditor)!=0)return nullptr;
        auto* editor=new VSTGUI::VST3Editor(this,"view","DrumFinisher.uidesc");
        editor->setAllowedZoomFactors(std::vector<double>{1.0,1.5});
        editor->setZoomFactor(zoom_);
        editor_=editor;
        return editor;
    }
    VSTGUI::CView* createCustomView(VSTGUI::UTF8StringPtr name,
        const VSTGUI::UIAttributes& attributes,
        const VSTGUI::IUIDescription*,VSTGUI::VST3Editor* editor) override {
        if(!name||!editor)return nullptr;
        if(std::strcmp(name,"DrumBrandLogo")==0){
            VSTGUI::CPoint o{0.0,0.0},z{150.0,70.0};
            attributes.getPointAttribute("origin",o);
            attributes.getPointAttribute("size",z);
            return new DrumFinisher::BrandLogoView(VSTGUI::CRect(o.x,o.y,o.x+z.x,o.y+z.y));
        }
        const char* labels[]={"DrumKnobPunch","DrumKnobBody","DrumKnobTight",
                              "DrumKnobFinish","DrumKnobGlue","DrumKnobOutput"};
        int tag=-1;
        for(int n=0;n<6;++n)if(std::strcmp(name,labels[n])==0){tag=100+n;break;}
        if(tag<0)return nullptr;
        VSTGUI::CPoint origin{0.0,0.0},size{125.0,125.0};
        attributes.getPointAttribute("origin",origin);
        attributes.getPointAttribute("size",size);
        auto* knob=new DrumFinisher::SteelKnob(
            VSTGUI::CRect(origin.x,origin.y,origin.x+size.x,origin.y+size.y),
            editor,tag,DrumFinisher::SteelKnob::Style::Hero);
        knob->setDefaultValue(tag==105?0.5f:0.0f);
        return knob;
    }
    VSTGUI::CView* verifyView(VSTGUI::CView* view,
        const VSTGUI::UIAttributes&,const VSTGUI::IUIDescription*,
        VSTGUI::VST3Editor* editor) override {
        auto* control=dynamic_cast<VSTGUI::CControl*>(view);
        if(control){
            const auto tag=control->getTag();
            if(tag==9000||tag==kCharacter||tag==kBypass){
                editor_=editor;
                control->setListener(this);
                if(tag==9000)
                    control->setValueNormalized(zoom_>=1.25?1.f:0.f);
                else
                    control->setValueNormalized(static_cast<float>(
                        getParamNormalized(static_cast<ParamID>(tag))));
            }
        }
        return view;
    }
    void valueChanged(VSTGUI::CControl* control) override {
        if(!control)return;
        const auto tag=control->getTag();
        if(tag==9000 && editor_){
            zoom_=control->getValueNormalized()>=0.5f?1.5:1.0;
            editor_->setZoomFactor(zoom_);
            return;
        }
        if(tag==kCharacter||tag==kBypass){
            const auto id=static_cast<ParamID>(tag);
            const double normalized=tag==kCharacter
                ? (control->getValueNormalized()<0.25f?0.0:
                   control->getValueNormalized()<0.75f?0.5:1.0)
                : (control->getValueNormalized()>=0.5f?1.0:0.0);
            // Explicit VST3 host automation gesture for segment clicks.
            beginEdit(id);
            setParamNormalized(id,normalized);
            performEdit(id,normalized);
            endEdit(id);
            control->setValueNormalized(static_cast<float>(normalized));
        }
    }
    void willClose(VSTGUI::VST3Editor* editor) override {
        if(editor_==editor)editor_=nullptr;
    }
    tresult PLUGIN_API setComponentState(IBStream* stream) override {
        Controls c;bool bypass=false;
        if(!loadState(stream,c,bypass))return kResultFalse;
        for(ParamID id : {kPunch,kBody,kTight,kFinish,kGlue,kOutput,kCharacter,kBypass})
            setParamNormalized(id,normalized(c,id,bypass));
        return kResultOk;
    }
private:
    VSTGUI::VST3Editor* editor_=nullptr;
    double zoom_=1.0;
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
