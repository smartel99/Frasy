/**
 * @file    Components.h
 * @author  Samuel Martel
 * @p       https://github.com/smartel99
 * @date    9/28/2020 1:39:48 PM
 *
 * @brief
 ******************************************************************************
 * Copyright (C) 2020  Samuel Martel
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *****************************************************************************/
#pragma once

/*********************************************************************************************************************/
// [SECTION] Includes
/*********************************************************************************************************************/

#include "Brigerad/Core/Log.h"
#include "Brigerad/Renderer/Texture.h"
#include "Brigerad/Scene/SceneCamera.h"
#include "Brigerad/Scene/ScriptableEntity.h"
#include "Brigerad/Scene/ImGuiWindowComponents.h"
#include "Brigerad/Scene/ImGuiTextComponents.h"
#include "Brigerad/Scene/ImGuiButtonComponents.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "imgui.h"

#include <string>
#include <ostream>


/*********************************************************************************************************************/
// [SECTION] Defines
/*********************************************************************************************************************/


/*********************************************************************************************************************/
// [SECTION] Class Declarations
/*********************************************************************************************************************/

namespace Brigerad
{

struct TagComponent
{
    std::string tag;

    std::string to_string() { return tag; }
    TagComponent()                    = default;
    TagComponent(const TagComponent&) = default;
    TagComponent(const std::string& t) : tag(t) {}
};

struct ChildEntityComponent
{
    bool   isChild = true;
    Entity parent;

    ChildEntityComponent()                            = default;
    ChildEntityComponent(const ChildEntityComponent&) = default;
    ChildEntityComponent(Entity p) : parent(p) {}
};

struct ParentEntityComponent
{
    uint64_t            uuid = 0;
    std::vector<Entity> childs;

    ParentEntityComponent()                             = default;
    ParentEntityComponent(const ParentEntityComponent&) = default;
    ParentEntityComponent(const std::vector<Entity>& c) : childs(c) {}
    ParentEntityComponent(Entity child) : childs({child}) {}
};

struct TransformComponent
{
    glm::vec3 position = glm::vec3 {0.0f, 0.0f, 0.0f};
    glm::vec3 rotation = glm::vec3 {0.0f, 0.0f, 0.0f};
    glm::vec3 scale    = glm::vec3 {1.0f, 1.0f, 1.0f};

    TransformComponent()                          = default;
    TransformComponent(const TransformComponent&) = default;
    TransformComponent(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& sc)
    : position(pos), rotation(rot), scale(sc)
    {
    }

    glm::mat4 GetTransform() const
    {
        return glm::translate(glm::mat4(1.0f), position) *
               glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), {1, 0, 0}) *
               glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), {0, 1, 0}) *
               glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), {0, 0, 1}) *
               glm::scale(glm::mat4(1.0f), scale);
    }

    const glm::vec3& GetPosition() const { return position; }
    void             SetPosition(const glm::vec3& newPos) { position = newPos; }

    const glm::vec3& GetRotation() const { return rotation; }
    void             SetRotation(const glm::vec3& newRot) { rotation = newRot; }

    const glm::vec3& GetScale() const { return scale; }
    void             SetScale(const glm::vec3& newScale) { scale = newScale; }

    operator glm::mat4() const { return GetTransform(); }
};

struct ColorRendererComponent
{
    glm::vec4 color {1.0f, 1.0f, 1.0f, 1.0f};

    ColorRendererComponent()                              = default;
    ColorRendererComponent(const ColorRendererComponent&) = default;
    ColorRendererComponent(const glm::vec4& col) : color(col) {}
};

struct TextureRendererComponent
{
    Ref<Texture2D> texture = nullptr;
    std::string    path    = "";

    TextureRendererComponent()                                = default;
    TextureRendererComponent(const TextureRendererComponent&) = default;
    TextureRendererComponent(const std::string& p) : texture(Texture2D::Create(p)), path(p) {}
};

struct CameraComponent
{
    SceneCamera camera;
    bool        primary          = true;    // TODO: Think about moving this to scene instead.
    bool        fixedAspectRatio = false;

    CameraComponent()                       = default;
    CameraComponent(const CameraComponent&) = default;
};

struct TextComponent
{
    std::string text  = "";
    float       scale = 1.0f;

    TextComponent()                     = default;
    TextComponent(const TextComponent&) = default;
    TextComponent(const std::string& t, float s) : text(t), scale(s) {}
};

struct NativeScriptComponent
{
    ScriptableEntity* instance = nullptr;

    ScriptableEntity* (*instantiateScript)()      = nullptr;
    void (*destroyScript)(NativeScriptComponent*) = nullptr;

    template<typename T>
    void Bind()
    {
        instantiateScript = []() {
            return static_cast<ScriptableEntity*>(new T());
        };

        destroyScript = [](NativeScriptComponent* nsc) {
            delete nsc->instance;
        };
    }
};

struct LuaScriptComponent
{
    LuaScriptEntity* instance = nullptr;
    std::string      path     = "";
    std::string      name     = "LuaScriptComponent";

    LuaScriptComponent(const std::string& p, const std::string& n) : path(p), name(n) {}
    ~LuaScriptComponent() = default;    //{ delete instance; }

    LuaScriptEntity* InstantiateScript() { return new LuaScriptEntity(path, name); }
    void             ReloadScript()
    {
        delete instance;
        instance = nullptr;
    }
};

}    // namespace Brigerad
