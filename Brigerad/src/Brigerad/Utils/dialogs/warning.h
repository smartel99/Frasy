/**
 * @file    warning.h
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
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */

#ifndef BRIGERAD_SRC_BRIGERAD_UTILS_DIALOGS_WARNING_H
#define BRIGERAD_SRC_BRIGERAD_UTILS_DIALOGS_WARNING_H

#include <format>
#include <string>
#include <string_view>

namespace Brigerad
{
namespace Details
{
void WarningDialogImpl(std::string_view title, const std::string& msg);
}

template<typename... Args>
void WarningDialog(std::string_view title, std::string_view msg, Args&&... args)
{
    std::string fmtMessage = std::vformat(msg, std::make_format_args(args...));
    Details::WarningDialogImpl(title, fmtMessage);
}
}    // namespace Brigerad

#endif    // BRIGERAD_SRC_BRIGERAD_UTILS_DIALOGS_WARNING_H
