#pragma once
#include "Brigerad/Core/Layer.h"

#include "Brigerad/Events/KeyEvents.h"
#include "Brigerad/Events/MouseEvent.h"
#include "Brigerad/Events/ApplicationEvent.h"

namespace Brigerad
{
class BRIGERAD_API ImGuiLayer : public Layer
{
public:
    ImGuiLayer();
    ~ImGuiLayer() override;

    virtual void onAttach() override;
    virtual void onDetach() override;
    virtual void onEvent(Event& event) override;
    virtual void onImGuiRender() override;

    void        Begin();
    void        End();
    inline void SetIsVisible(bool isVisible) { m_open = isVisible; }
    inline void ToggleIsVisible() { m_open = !m_open; }

    inline void SetBlockEvents(bool state) { m_blockImGuiEvents = state; }

private:
    double m_time             = 0.0;
    bool   m_open             = false;
    bool   m_showMetricWindow = false;
#if defined(BR_DEBUG)
    bool m_showStyleEditor = false;
    bool m_showDemoWindow = false;
    bool m_showPlotWindow = false;
#endif

    bool   m_isProfiling        = false;
    double m_profilingStartTime = 0.0;
    double m_profilingDuration  = 1.0;

    bool m_blockImGuiEvents = false;
};
}    // namespace Brigerad
