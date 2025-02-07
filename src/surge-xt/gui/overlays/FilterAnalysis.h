/*
** Surge Synthesizer is Free and Open Source Software
**
** Surge is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html
**
** Copyright 2004-2020 by various individuals as described by the Git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** Surge was a commercial product from 2004-2018, with Copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Surge
** open source in September 2018.
*/

#ifndef SURGE_XT_FilterAnalysis_H
#define SURGE_XT_FilterAnalysis_H

#include "OverlayComponent.h"
#include "SkinSupport.h"
#include "FilterConfiguration.h"
#include "SurgeGUICallbackInterfaces.h"
#include "widgets/ModulatableSlider.h"
#include "widgets/MultiSwitch.h"

class SurgeStorage;
class SurgeGUIEditor;

namespace Surge
{
namespace Overlays
{
struct FilterAnalysisEvaluator;

struct FilterAnalysis : public OverlayComponent, Surge::GUI::SkinConsumingComponent
{
    SurgeGUIEditor *editor{nullptr};
    SurgeStorage *storage{nullptr};
    FilterAnalysis(SurgeGUIEditor *e, SurgeStorage *s);
    ~FilterAnalysis();
    void mouseDrag(const juce::MouseEvent &event) override;
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseUp(const juce::MouseEvent &event) override;
    void mouseMove(const juce::MouseEvent &event) override;
    void paint(juce::Graphics &g) override;
    void onSkinChanged() override;
    void resized() override;
    void repushData();
    void forceDataRefresh() override { repushData(); }

    juce::Rectangle<float> hotzone;
    juce::Point<int> cursorHideOrigin;

    bool isHovered{false}, isPressed{false}, cursorHidden{false};

    void showCursorAt(const juce::Point<float> &w) { cursorHideOrigin = w.toInt(); }
    void showCursorAt(const juce::Point<int> &w) { cursorHideOrigin = w; }

    void guaranteeCursorShown()
    {
        if (!Surge::GUI::showCursor(storage) && cursorHidden)
        {
            auto h = localPointToGlobal(cursorHideOrigin);

            juce::Desktop::getInstance().getMainMouseSource().enableUnboundedMouseMovement(false);
            juce::Desktop::getInstance().getMainMouseSource().setScreenPosition(h.toFloat());

            cursorHidden = false;
        }
    }

    void hideCursor(const juce::Point<int> &w)
    {
        cursorHideOrigin = w;

        if (!Surge::GUI::showCursor(storage))
        {
            cursorHidden = true;
            juce::Desktop::getInstance().getMainMouseSource().enableUnboundedMouseMovement(true);
        }
    }

    int whichFilter{0};
    void selectFilter(int which);

    std::unique_ptr<Surge::Widgets::SelfDrawToggleButton> f1Button, f2Button;
    std::unique_ptr<FilterAnalysisEvaluator> evaluator;
    bool shouldRepaintOnParamChange(const SurgePatch &patch, Parameter *p) override;
    uint64_t catchUpStore{0};
    juce::Path plotPath;
};
} // namespace Overlays
} // namespace Surge

#endif // SURGE_XT_FilterAnalysis_H
