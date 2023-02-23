/**
 * @file    type_name.h
 * @author  Samuel Martel
 * @date    2022-12-20
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

#ifndef FRASY_UTILS_MISC_TYPE_NAME_H
#define FRASY_UTILS_MISC_TYPE_NAME_H


#ifdef __GNUG__
#    include <cxxabi.h>
#    include <memory>
#endif

#include <string>


namespace Frasy
{
inline std::string Demangle(const char* name)
{
#ifdef __GNUG__
    int                                    status = 0;
    std::unique_ptr<char, void (*)(void*)> res {abi::__cxa_demangle(name, nullptr, nullptr, &status), std::free};

    return (status == 0) ? res.get() : name;
#else
    return name;
#endif
}

template<typename T>
std::string TypeName()
{
    return Demangle(typeid(T).name());
}

}    // namespace Frasy

#endif    // FRASY_UTILS_MISC_TYPE_NAME_H
