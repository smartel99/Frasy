/**
 * @file    command.cpp
 * @author  Paul Thomas
 * @date    2023-02-23
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
 * not, see <https://www.gnu.org/licenses/>.
 */

#include "command.h"

#include "Brigerad.h"

#include <spdlog/spdlog.h>

namespace Frasy::Actions::Log
{
Command Make()
{
    return Command {
      .Id = static_cast<cmd_id_t>(CommandId::Log),
      .Executor =
        [](const Commands::CommandEvent& e)
      {
          auto                      msg    = e.Pkt.FromPayload<std::string>();
          auto                      cLevel = msg.at(msg.find_first_of("(") - 2);
          spdlog::level::level_enum level;
          switch (cLevel)
          {
              case 'E': level = spdlog::level::err; break;
              case 'W': level = spdlog::level::warn; break;
              case 'I': level = spdlog::level::info; break;
              case 'D': level = spdlog::level::debug; break;
              case 'T':
              default: level = spdlog::level::trace; break;
          }

          auto endTs      = msg.find_first_of(")") + 2;
          auto clearColor = msg.find_last_of('\x1b');
          auto substr     = msg.substr(endTs, clearColor - endTs);

          BR_LOG(e.Source, level, substr);
      },
    };
}
}    // namespace Frasy::Actions::Log