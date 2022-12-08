/**
 ******************************************************************************
 * @addtogroup Layer
 * @{
 * @file    Layer
 * @author  Samuel Martel
 * @brief   Header for the Layer module.
 *
 * @date 2/29/2020 12:45:33 PM
 *
 ******************************************************************************
 */
#ifndef _Layer
#    define _Layer

/*****************************************************************************/
/* Includes */
#    include "Brigerad/Core/Core.h"
#    include "Brigerad/Core/Timestep.h"
#    include "Brigerad/Events/Event.h"

#    include <string>

namespace Brigerad
{
/*****************************************************************************/
/* Exported defines */

/*****************************************************************************/
/* Exported macro */

/*****************************************************************************/
/* Exported types */
class BRIGERAD_API Layer
{
public:
    Layer(const std::string& name = "Layer");
    virtual ~Layer();

    virtual void OnAttach() {}

    virtual void OnDetach() {}

    virtual void OnUpdate(Timestep timestep) {}

    virtual void OnImGuiRender() {}

    virtual void OnEvent(Event& event) {}

    inline const std::string& GetName() const { return m_debugName; }

protected:
    std::string m_debugName;
};

/*****************************************************************************/
/* Exported functions */

}    // namespace Brigerad
/* Have a wonderful day :) */
#endif /* _Layer */
/**
 * @}
 */
/****** END OF FILE ******/
