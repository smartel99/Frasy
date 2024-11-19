/**
 * @file    circular_progress_indicator.h
 * @author  Samuel Martel
 * @date    2024-10-23
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


#ifndef FRASY_SRC_UTILS_IMGUI_CIRCULAR_PROGRESS_INDICATOR_H
#define FRASY_SRC_UTILS_IMGUI_CIRCULAR_PROGRESS_INDICATOR_H

#include <string_view>

namespace Frasy::Widget {
/**
 * Just like a regular progress bar, but circular.
 *
 * Optionally, text can be placed in the center.
 *
 * @param id ImGui ID of the widget.
 * @param fraction Fill rate of the progress bar. Between 0 and 1
 * @param radius Radius of the circle.
 * @param thickness Thickness of the progress bar.
 * @param label [Optional] Text to be placed in the center of the progress bar.
 */
void circularProgressIndicator(
  const char* id, float fraction, float radius, float thickness = 15.0f, std::string_view label = "");
bool circularProgressIndicatorButton(
  const char* id, float fraction, float radius, float thickness = 15.0f, std::string_view label = "");
}    // namespace Frasy::Widget
#endif    // FRASY_SRC_UTILS_IMGUI_CIRCULAR_PROGRESS_INDICATOR_H
