/**
 * @file    types.h
 * @author  Samuel Martel
 * @date    2024-05-09
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


#ifndef FRASY_SRC_UTILS_IMGUI_TYPES_H
#define FRASY_SRC_UTILS_IMGUI_TYPES_H

#include <imgui.h>

#include <cstdint>

namespace Frasy {
template<typename T>
concept FitsInImGuiSliderInt = !std::same_as<T, uint32_t> && !std::same_as<T, uint64_t> && !std::same_as<T, int64_t> &&
                               !std::same_as<T, float> && !std::same_as<T, double>;

template<typename T>
struct ImGuiTypeFromType;

template<>
struct ImGuiTypeFromType<int8_t> {
    static constexpr auto value = ImGuiDataType_S8;
};

template<>
struct ImGuiTypeFromType<uint8_t> {
    static constexpr auto value = ImGuiDataType_U8;
};

template<>
struct ImGuiTypeFromType<int16_t> {
    static constexpr auto value = ImGuiDataType_S16;
};

template<>
struct ImGuiTypeFromType<uint16_t> {
    static constexpr auto value = ImGuiDataType_U16;
};

template<>
struct ImGuiTypeFromType<int32_t> {
    static constexpr auto value = ImGuiDataType_S32;
};

template<>
struct ImGuiTypeFromType<uint32_t> {
    static constexpr auto value = ImGuiDataType_U32;
};

template<>
struct ImGuiTypeFromType<int64_t> {
    static constexpr auto value = ImGuiDataType_S64;
};

template<>
struct ImGuiTypeFromType<uint64_t> {
    static constexpr auto value = ImGuiDataType_U64;
};

template<>
struct ImGuiTypeFromType<float> {
    static constexpr auto value = ImGuiDataType_Float;
};

template<>
struct ImGuiTypeFromType<double> {
    static constexpr auto value = ImGuiDataType_Double;
};

template<typename T>
static auto constexpr ImGuiTypeFromType_v = ImGuiTypeFromType<T>::value;

}    // namespace Frasy

#endif    // FRASY_SRC_UTILS_IMGUI_TYPES_H
