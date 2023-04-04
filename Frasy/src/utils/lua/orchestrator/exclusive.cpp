/**
 * @file    exclusive.cpp
 * @author  Paul Thomas
 * @date    4/4/2023
 * @brief
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. If
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */

#include "orchestrator.h"

namespace Frasy::Lua
{
void Orchestrator::ImportExclusive(sol::state& lua, Stage stage)
{
    if (!m_exclusiveLock) m_exclusiveLock = std::make_unique<std::mutex>();
    switch (stage)
    {
        case Stage::Execution:
            lua["__exclusive"] = [&](std::size_t index, sol::function func)
            {
                m_exclusiveLock->lock();
                auto& mutex = m_exclusiveLockMap[index];
                m_exclusiveLock->unlock();
                std::lock_guard lock {mutex};
                std::cout << "Exclusive part: Start " << std::endl;
                func();
                std::cout << "Exclusive part: End " << std::endl;
            };
            break;

        case Stage::Idle:
        case Stage::Generation:
        case Stage::Validation:
        default: lua["__exclusive"] = [&](std::size_t index, sol::function func) { func(); }; break;
    }
}
}    // namespace Frasy::Lua
