/**
 * @file    warning.cpp
 * @author  Samuel Martel
 * @date    2023-04-17
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
#include "../../../src/Brigerad/Utils/dialogs/warning.h"

#include "../../../src/Brigerad/Core/Application.h"

#include <codecvt>
#include <locale>
#include <string>
#include <Windows.h>


namespace Brigerad::Details
{
void WarningDialogImpl(std::string_view title, const std::string& msg)
{
    MessageBoxA(nullptr, msg.c_str(), title.data(), MB_OK | MB_ICONWARNING | MB_TASKMODAL);
}
}    // namespace Brigerad::Details
