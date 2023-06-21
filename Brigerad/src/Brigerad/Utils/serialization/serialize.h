/**
 * @file    serialize.h
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

#ifndef BRIGERAD_UTILS_SERIALIZATION_SERIALIZE_H
#define BRIGERAD_UTILS_SERIALIZATION_SERIALIZE_H

#include "basic_serializer.h"
#include "is_serializable.h"
#include "serializer.h"

#include <vector>

namespace Brigerad
{
template<typename T, typename S = BasicSerializer>
static std::vector<uint8_t> Serialize(T&& t)
    requires Serializable<std::remove_cvref_t<T>> && Serializer<S, T>
{
    return S::Serialize(t);
}

template<Serializable T, typename S = BasicSerializer>
static T Deserialize(std::vector<uint8_t>&& raw)
    requires Serializer<S, T>
{
    return S::Deserialize(raw);
}

}    // namespace Brigerad

#endif    // BRIGERAD_UTILS_SERIALIZATION_SERIALIZE_H
