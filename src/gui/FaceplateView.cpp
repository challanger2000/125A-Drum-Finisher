#include "FaceplateView.h"

#include "vstgui/lib/cdrawcontext.h"

namespace DrumFinisher {
namespace {

using VSTGUI::CColor;
using VSTGUI::CRect;
using VSTGUI::CPoint;
using VSTGUI::CDrawContext;

void fill(CDrawContext* context, CRect rect, CColor color) {
    context->setFillColor(color);
    context->drawRect(rect, VSTGUI::kDrawFilled);
}

void outline(CDrawContext* context, CRect rect, CColor color) {
    context->setFrameColor(color);
    context->setLineWidth(1.0);
    context->drawRect(rect, VSTGUI::kDrawStroked);
}

void ruledLine(CDrawContext* context, double x1, double y1,
               double x2, double y2, CColor color) {
    context->setFrameColor(color);
    context->setLineWidth(1.0);
    context->drawLine(CPoint(x1,y1),CPoint(x2,y2));
}

void screw(CDrawContext* context, double x, double y) {
    context->setFillColor(CColor{8, 10, 13, 255});
    context->drawEllipse(CRect(x-4.2,y-4.2,x+4.2,y+4.2),
                         VSTGUI::kDrawFilled);
    context->setFrameColor(CColor{95, 103, 113, 195});
    context->setLineWidth(1.0);
    context->drawEllipse(CRect(x-3.8,y-3.8,x+3.8,y+3.8),
                         VSTGUI::kDrawStroked);
    ruledLine(context,x-1.8,y+1.5,x+1.8,y-1.5,CColor{146,151,157,115});
}
} // namespace

FaceplateView::FaceplateView(const VSTGUI::CRect& size) : CView(size) {
    setTransparency(true);
    // Never intercept a mouse click, drag or wheel action from a knob/button.
    setMouseEnabled(false);
}

void FaceplateView::draw(VSTGUI::CDrawContext* context) {
    context->setDrawMode(VSTGUI::kAntiAliasing);

    // Header tool bay: zoom and one global BYPASS control.
    fill(context,CRect(998,22,1176,122),CColor{15,18,22,120});
    outline(context,CRect(998,22,1176,122),CColor{73,78,86,135});
    ruledLine(context,999,23,1175,23,CColor{113,116,121,72});

    // Thin single separator visually anchors the spacious masthead.
    ruledLine(context,36,126,1164,126,CColor{94,99,109,112});
    ruledLine(context,36,127,1164,127,CColor{3,5,8,180});

    // Main recessed bay for five processing modules + the output trim.
    fill(context,CRect(38,146,1164,376),CColor{0,0,0,66});
    fill(context,CRect(35,142,1165,372),CColor{31,35,41,255});
    outline(context,CRect(35,142,1165,372),CColor{82,88,98,158});
    outline(context,CRect(39,146,1161,368),CColor{8,11,15,185});
    ruledLine(context,40,143,1160,143,CColor{142,147,156,54});

    // Do not box every knob individually: six evenly-spaced bays with
    // five quiet engraved separators, stronger before master OUTPUT.
    for (double x : {225.0,410.0,595.0,780.0,965.0}) {
        const bool outputDivider=x>900.0;
        ruledLine(context,x,163,x,353,
                  outputDivider?CColor{108,111,121,130}:
                                CColor{90,97,108,64});
        ruledLine(context,x+1.0,163,x+1.0,353,
                  CColor{4,7,10,130});
    }
    fill(context,CRect(971,148,1159,365),CColor{17,20,25,54});
    for (auto p : {CPoint{50,157},CPoint{1150,157},
                   CPoint{50,357},CPoint{1150,357}}) {
        screw(context,p.x,p.y);
    }

    // Separate lower character-selection recess, centered below the knobs.
    fill(context,CRect(391,390,809,484),CColor{0,0,0,69});
    fill(context,CRect(388,386,812,480),CColor{26,29,35,255});
    outline(context,CRect(388,386,812,480),CColor{78,83,90,164});
    outline(context,CRect(392,390,808,476),CColor{9,12,16,150});
    ruledLine(context,396,387,804,387,CColor{151,152,155,60});
    screw(context,402,400);
    screw(context,798,400);

    // Small restraint at the footer; no extra neon accents or clutter.
    ruledLine(context,35,498,1165,498,CColor{75,81,89,84});
    setDirty(false);
}

} // namespace DrumFinisher
