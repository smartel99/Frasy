/**
 * @file    is_serializable.h
 * @author  Samuel Martel
 * @date    2022-12-12
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

#ifndef BRIGERAD_UTILS_SERIALIZATION_IS_SERIALIZABLE_H
#define BRIGERAD_UTILS_SERIALIZATION_IS_SERIALIZABLE_H

#include "../concepts/members_all_match.h"

namespace Brigerad
{
template<typename T>
struct IsASerializableType
: IsAnyOf<std::remove_cvref_t<T>, bool, uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t,
          float,
          double>
{
};

template<typename T>
concept Serializable = MembersAllMeetCheck<std::remove_cvref_t<T>, IsASerializableType>;

}    // namespace Brigerad
#endif    // BRIGERAD_UTILS_SERIALIZATION_IS_SERIALIZABLE_H
