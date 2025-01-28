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
#include "glm/gtx/io.hpp"

#include <functional>
#include <shared_mutex>
#include <sol/sol.hpp>
#include <string>
#include <utility>

namespace Frasy::Lua {

class Popup {
public:
    struct Element {
        enum class Kind : std::uint8_t {
            Text,
            Input,
            Button,
            Image,
            BeginHorizontal,
            EndHorizontal,
            BeginVertical,
            EndVertical,
            SameLine,
            Spring,
        } kind;
        explicit Element(Kind kind) : kind(kind) {}
        virtual ~Element()    = default;
        virtual void render() = 0;
    };

    struct Text : Element {
        std::string text;
        Text(std::string text) : Element(Kind::Text), text(std::move(text)) {}
        void render() final;
    };

    struct Input : Element {
        static constexpr std::size_t                                     bufferLen = 50;
        std::array<char, bufferLen>                                      buffer {};
        std::string                                                      title;
        std::size_t                                                      index;
        std::function<void(const std::string& value, std::size_t index)> onChange;
        Input(std::string                                                      title,
              std::size_t                                                      index,
              std::function<void(const std::string& value, std::size_t index)> onChange)
        : Element(Kind::Input), title(std::move(title)), index(index), onChange(std::move(onChange))
        {
        }
        void render() final;
    };

    struct Button : Element {
        Button(std::string               label,
               std::array<float, 2>      size,
               sol::function             action,
               bool                      consume,
               std::function<void()>     onConsume,
               std::shared_mutex*        luaMutex,
               std::vector<std::string>* inputs)
        : Element(Kind::Button),
          label(std::move(label)),
          size(ImVec2(size[0], size[1])),
          action(std::move(action)),
          consume(consume),
          onConsume(std::move(onConsume)),
          luaMutex(luaMutex),
          inputs(inputs)
        {
        }
        std::string               label;
        ImVec2                    size;
        sol::function             action;
        bool                      consume;
        std::function<void()>     onConsume;
        std::shared_mutex*        luaMutex;
        std::vector<std::string>* inputs;
        void                      render() final;
    };

    struct Image : Element {
        std::string                        path;
        ImVec2                             size;
        Brigerad::Ref<Brigerad::Texture2D> texture;

        Image(std::string path, ImVec2 size) : Element(Kind::Image), path(std::move(path)), size(size) {}

        void render() final;
    };

    struct SameLine : Element {
        float offsetFromStartX;
        float spacing;
        SameLine(float offsetFromStartX, float spacing)
        : Element(Kind::SameLine), offsetFromStartX(offsetFromStartX), spacing(spacing)
        {
        }

        void render() final;
    };

    struct BeginHorizontal : Element {
        int    id;
        ImVec2 size;
        float  align;

        BeginHorizontal(int id, ImVec2 size, float align)
        : Element(Kind::BeginHorizontal), id(id), size(size), align(align)
        {
        }
        void render() final;
    };
    struct EndHorizontal : Element {
        EndHorizontal() : Element(Kind::EndHorizontal) {}
        void render() final;
    };
    struct BeginVertical : Element {

        int    id;
        ImVec2 size;
        float  align;

        BeginVertical(int id, ImVec2 size, float align) : Element(Kind::BeginVertical), id(id), size(size), align(align)
        {
        }
        void render() final;
    };
    struct EndVertical : Element {
        EndVertical() : Element(Kind::EndVertical) {}
        void render() final;
    };

    struct Spring : Element {
        float weight;
        float spacing;

        Spring(float weight, float spacing) : Element(Kind::Spring), weight(weight), spacing(spacing) {}
        void render() final;
    };

private:
    std::string                           m_name;
    std::vector<std::unique_ptr<Element>> m_elements;
    std::vector<std::string>              m_inputs;
    std::atomic_bool                      m_consumed = false;
    std::optional<sol::function>          m_routine;
    std::shared_mutex                     m_luaMutex;
    std::string                           m_consumeButtonText = "Cancel";

public:
    Popup() = default;
    explicit Popup(std::size_t uut, sol::table builder);
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
