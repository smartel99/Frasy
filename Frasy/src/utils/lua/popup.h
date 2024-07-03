/**
 * @file    popup.h
 * @author  Paul Thomas
 * @date    3/30/2023
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
#ifndef COPY_LUA_PY_FRASY_SRC_UTILS_LUA_POPUP_H
#define COPY_LUA_PY_FRASY_SRC_UTILS_LUA_POPUP_H

#include "Brigerad.h"
#include "Brigerad/Core/File.h"

#include <functional>
#include <shared_mutex>
#include <sol/sol.hpp>
#include <string>

namespace Frasy::Lua {

class Popup {
public:
    struct Element {
        enum class Kind {
            Text,
            Input,
            Button,
            Image,
        } kind;
        std::string text;
        explicit    Element(Kind kind, const std::string& text) : kind(kind), text(text) {}
        virtual ~   Element() = default;
    };

    struct Text : Element {
         Text(const std::string& text) : Element(Kind::Text, text) {}
    };

    struct Input : Element {
         Input(const std::string& text, std::size_t index) : Element(Kind::Input, text), index(index) {}
        static constexpr std::size_t vBufLen       = 50;
        char                         vBuf[vBufLen] = "";
        std::size_t                  index         = 0;
    };

    struct Button : Element {
         Button(const std::string& text, sol::function action) : Element(Kind::Button, text), action(action) {}
        sol::function action;
    };

    struct Image : Element {
        Image(const std::string& path, std::size_t width, std::size_t height)
        : Element(Kind::Image, path), width(width), height(height)
        {
        }

        std::size_t                        width  = 0;
        std::size_t                        height = 0;
        Brigerad::Ref<Brigerad::Texture2D> texture;
    };

private:
    std::string                           m_name;
    std::vector<std::unique_ptr<Element>> m_elements;
    std::vector<std::string>              m_inputs;
    std::atomic_bool                      m_consumed = false;
    std::optional<sol::function>          m_routine;
    std::shared_mutex                     m_luaMutex;

public:
                                    Popup() = default;
    explicit                        Popup(std::size_t uut, sol::table builder);
    static std::string              GetName(std::size_t uut, sol::table builder);
    const std::string&              GetName() { return m_name; }
    const std::vector<std::string>& GetInputs() { return m_inputs; }
    void                            Routine(bool once = false);
    void                            Consume()
    {
        BR_LUA_DEBUG("Consume {}", m_name);
        m_consumed = true;
    }
    void Render();
};

}    // namespace Frasy::Lua

#endif    // COPY_LUA_PY_FRASY_SRC_UTILS_LUA_POPUP_H
