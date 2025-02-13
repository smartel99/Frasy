/**
 * @file    struct.cpp
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
 * not, see <https://www.gnu.org/licenses/>.
 */

#include "struct.h"

#include "type_id_t.h"
#include "utils/misc/deserializer.h"
namespace Frasy::Type
{

type_id_t Struct::Manager::id        = 0;
type_id_t Struct::Field::Manager::id = 0;

static_assert(requires {
    {
        Deserialize<Struct::Field>(nullptr, nullptr)
    } -> std::same_as<Struct::Field>;
});

static_assert(requires {
    {
        Deserialize<Struct>(nullptr, nullptr)
    } -> std::same_as<Struct>;
});
}    // namespace Frasy::Type
