/**
 ******************************************************************************
 * @addtogroup LayerStack
 * @{
 * @file    LayerStack
 * @author  Client Microdata
 * @brief   Header for the LayerStack module.
 *
 * @date 2/29/2020 12:51:26 PM
 *
 ******************************************************************************
 */
#ifndef _LayerStack
#define _LayerStack

/*****************************************************************************/
/* Includes */
#include "Brigerad/Core/Core.h"
#include "Layer.h"

#include <vector>

namespace Brigerad
{
/*****************************************************************************/
/* Exported defines */

/*****************************************************************************/
/* Exported macro */

/*****************************************************************************/
/* Exported types */
class BRIGERAD_API LayerStack
{
public:
    LayerStack();
    ~LayerStack();

    void PushLayer(Layer *layer);
    void PushOverlay(Layer *overlay);
    void PopLayer(Layer *layer);
    void PopOverlay(Layer *overlay);

    std::vector<Layer *>::iterator begin()
    {
        return m_layers.begin();
    }
    std::vector<Layer *>::iterator end()
    {
        return m_layers.end();
    }

private:
    std::vector<Layer *> m_layers;
    unsigned int m_layerInsertIndex = 0;
};

/*****************************************************************************/
/* Exported functions */

} // namespace Brigerad
/* Have a wonderful day :) */
#endif /* _LayerStack */
/**
 * @}
 */
/****** END OF FILE ******/
