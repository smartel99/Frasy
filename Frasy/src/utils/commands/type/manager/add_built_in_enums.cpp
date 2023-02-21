/**
 * @file    add_built_int_enums.cpp
 * @author  Paul Thomas
 * @date    2023-02-16
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
#include "../../built_in/status/error_code.h"
#include "manager.h"

namespace Frasy::Type
{
void Manager::AddBuiltInEnums()
{
    using Actions::Status::ErrorCode;
    ErrorCode::Manager::id = AddEnum(Enum {
      .Name = std::string(ErrorCode::Manager::name),
      .Fields =
        {
          Enum::Field {
            .Name        = std::string(ErrorCode::Manager::NoError::name),
            .Value       = ErrorCode::Manager::NoError::value,
            .Description = std::string(ErrorCode::Manager::NoError::description),
          },
          Enum::Field {
            .Name        = std::string(ErrorCode::Manager::NotFound::name),
            .Value       = ErrorCode::Manager::NotFound::value,
            .Description = std::string(ErrorCode::Manager::NotFound::description),
          },
          Enum::Field {
            .Name        = std::string(ErrorCode::Manager::GenericError::name),
            .Value       = ErrorCode::Manager::GenericError::value,
            .Description = std::string(ErrorCode::Manager::GenericError::description),
          },
        },
      .Description = std::string(ErrorCode::Manager::description),
    });
}
}    // namespace Frasy::Type