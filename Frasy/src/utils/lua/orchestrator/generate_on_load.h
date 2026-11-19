/**
 * @file    generate_on_load.h
 * @brief   Requests a solution generation once after each successful Orchestrator::loadUserFiles().
 */
#ifndef FRASY_SRC_UTILS_LUA_ORCHESTRATOR_GENERATE_ON_LOAD_H
#define FRASY_SRC_UTILS_LUA_ORCHESTRATOR_GENERATE_ON_LOAD_H

#include "orchestrator.h"

#include <cstdint>

namespace Frasy::Lua {
/**
 * Call update() every frame. A load is only marked as handled once its generation was accepted, so a load
 * made while running or generating is generated once the orchestrator is idle again.
 */
class GenerateOnLoad {
public:
    /// @return true if a generation was started.
    bool update(Orchestrator& orchestrator)
    {
        const auto loadId = orchestrator.getLoadId();
        if (loadId == m_handledLoadId || orchestrator.isRunning() || orchestrator.isGenerating()) { return false; }
        if (!orchestrator.generate()) { return false; }
        m_handledLoadId = loadId;
        return true;
    }

private:
    std::uint64_t m_handledLoadId = 0;
};
}    // namespace Frasy::Lua

#endif    // FRASY_SRC_UTILS_LUA_ORCHESTRATOR_GENERATE_ON_LOAD_H
