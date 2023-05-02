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
#    include "device_viewer.h"
#    include "log_window.h"
#    include "result_analyzer.h"
#    include "result_viewer.h"

#    include <array>
#    include <Brigerad.h>
#    include <Brigerad/Renderer/Texture.h>


/*****************************************************************************/
/* Exported defines */


/*****************************************************************************/
/* Exported macro */


/*****************************************************************************/
/* Exported types */
namespace Frasy
{
class LogWindow;

class MainApplicationLayer : public Brigerad::Layer
{
public:
    MainApplicationLayer()           = default;
    ~MainApplicationLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;

    void OnUpdate(Brigerad::Timestep ts) override;
    void OnImGuiRender() final;
    void OnEvent(Brigerad::Event& e) override;

protected:
    virtual void RenderControlRoom();

    virtual void MakeLogWindowVisible();
    virtual void MakeDeviceViewerVisible();
    virtual void MakeResultViewerVisible();
    virtual void MakeResultAnalyzerVisible();
    void         RenderAbout();

protected:
    bool m_renderAbout = false;
    bool m_noMove      = true;

    std::unique_ptr<LogWindow>      m_logWindow      = nullptr;
    std::unique_ptr<DeviceViewer>   m_deviceViewer   = nullptr;
    std::unique_ptr<ResultViewer>   m_resultViewer   = nullptr;
    std::unique_ptr<ResultAnalyzer> m_resultAnalyzer = nullptr;

    Brigerad::Ref<Brigerad::Texture2D> m_run;
    Brigerad::Ref<Brigerad::Texture2D> m_runWarn;
    Brigerad::Ref<Brigerad::Texture2D> m_pass;
    Brigerad::Ref<Brigerad::Texture2D> m_fail;
    Brigerad::Ref<Brigerad::Texture2D> m_error;
    Brigerad::Ref<Brigerad::Texture2D> m_testing;
    Brigerad::Ref<Brigerad::Texture2D> m_waiting;
    Brigerad::Ref<Brigerad::Texture2D> m_idle;
    Brigerad::Ref<Brigerad::Texture2D> m_disabled;

private:
    void PresetControlRoomOptions();
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
