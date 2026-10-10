#pragma once

#include "vstgui/lib/cview.h"

namespace DrumFinisher {

// A passive, non-interactive metallic faceplate drawn BEHIND all controls.
// At 100% its design surface is 1200x540 and zoom follows the host's view.
class FaceplateView final : public VSTGUI::CView {
public:
    explicit FaceplateView(const VSTGUI::CRect& size);
    void draw(VSTGUI::CDrawContext* context) override;
};

} // namespace DrumFinisher
