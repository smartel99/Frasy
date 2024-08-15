/**
 ******************************************************************************
 * @addtogroup main_application_layer
 * @{
 * @file    main_application_layer.h
 * @author  Samuel Martel
 * @brief   Header for the MainApplicationLayer module.
 *
 * @date 9/23/2020 9:32:55 AM
 *
 ******************************************************************************
 */
#ifndef FRASY_SRC_LAYERS_MAIN_APPLICATION_LAYER_H
#    define FRASY_SRC_LAYERS_MAIN_APPLICATION_LAYER_H

/*****************************************************************************/
/* Includes */
#    include "can_open_viewer.h"
#    include "device_viewer.h"
#    include "log_window.h"
#    include "result_analyzer.h"
#    include "result_viewer.h"
#    include "test_viewer.h"
#    include "utils/communication/can_open/can_open.h"
#    include "utils/lua/orchestrator/orchestrator.h"

#    include <Brigerad.h>
#    include <Brigerad/Renderer/Texture.h>

#    include <vector>


/*****************************************************************************/
/* Exported defines */


/*****************************************************************************/
/* Exported macro */


/*****************************************************************************/
/* Exported types */
namespace Frasy {
class LogWindow;

class MainApplicationLayer : public Brigerad::Layer, public TestViewer::Interface {
public:
                          MainApplicationLayer()                            = default;
                          MainApplicationLayer(const MainApplicationLayer&) = delete;
    MainApplicationLayer& operator=(const MainApplicationLayer&)            = delete;
                          MainApplicationLayer(MainApplicationLayer&&)      = delete;
    MainApplicationLayer& operator=(MainApplicationLayer&&)                 = delete;

    ~MainApplicationLayer() override = default;

    void onAttach() override;
    void onDetach() override;

    void onUpdate(Brigerad::Timestep ts) override;
    void onImGuiRender() final;
    void onEvent(Brigerad::Event& e) override;

protected:
    virtual void renderControlRoom() {}

    virtual void makeLogWindowVisible();
    virtual void makeDeviceViewerVisible();
    virtual void makeCanOpenViewerVisible();
    virtual void makeResultViewerVisible();
    virtual void makeResultAnalyzerVisible();
    virtual void makeTestViewerVisible();
    void         renderAbout();
    void         renderProfiler();
    void         renderProfilerTable(const std::thread::id& id, const ProfilerDetails& details);
    void         renderProfilerTableRow(const ProfileEvent& event, float totalTime, float indent = 0.0f);
    void         renderProfilerGraphs();

private:
    void generate() override;
    void setTestEnable(const std::string& sequence, const std::string& test, bool enable) override;
    void setSequenceEnable(const std::string& sequence, bool enable) override;

protected:
    bool m_renderAbout    = false;
    bool m_renderProfiler = false;
    bool m_noMove         = true;

    std::unique_ptr<LogWindow>            m_logWindow      = nullptr;
    std::unique_ptr<DeviceViewer>         m_deviceViewer   = nullptr;
    std::unique_ptr<CanOpenViewer::Layer> m_canOpenViewer  = nullptr;
    std::unique_ptr<ResultViewer>         m_resultViewer   = nullptr;
    std::unique_ptr<ResultAnalyzer>       m_resultAnalyzer = nullptr;
    std::unique_ptr<TestViewer>           m_testViewer     = nullptr;

    Brigerad::Ref<Brigerad::Texture2D> m_run;
    Brigerad::Ref<Brigerad::Texture2D> m_runWarn;
    Brigerad::Ref<Brigerad::Texture2D> m_pass;
    Brigerad::Ref<Brigerad::Texture2D> m_fail;
    Brigerad::Ref<Brigerad::Texture2D> m_error;
    Brigerad::Ref<Brigerad::Texture2D> m_testing;
    Brigerad::Ref<Brigerad::Texture2D> m_waiting;
    Brigerad::Ref<Brigerad::Texture2D> m_idle;
    Brigerad::Ref<Brigerad::Texture2D> m_disabled;

    CanOpen::CanOpen  m_canOpen;
    Lua::Orchestrator m_orchestrator;
    Lua::Map          m_map;


private:
    struct ProfileEventInfo {
        std::string         windowName;
        const ProfileEvent* event  = nullptr;
        bool                render = true;
        //! When true, renders markers as sample. When false, renders them properly in time.
        bool displayAsSamples = true;
    };

    std::vector<ProfileEventInfo> m_profileGraphPopups;
    void                          PresetControlRoomOptions();

    void DumpProfileEvents();
};
}    // namespace Frasy
/*****************************************************************************/
/* Exported functions */


/* Have a wonderful day :) */
#endif /* FRASY_SRC_LAYERS_MAIN_APPLICATION_LAYER_H */
/**
 * @}
 */
/****** END OF FILE ******/
