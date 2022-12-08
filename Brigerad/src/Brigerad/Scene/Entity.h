/**
 * @file    Entity
 * @author  Samuel Martel
 * @p       https://github.com/smartel99
 * @date    9/28/2020 2:47:12 PM
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
#include "Scene.h"

#include "entt.hpp"

/*********************************************************************************************************************/
// [SECTION] Defines
/*********************************************************************************************************************/


/*********************************************************************************************************************/
// [SECTION] Class Declarations
/*********************************************************************************************************************/
namespace Brigerad
{

class Entity
{
public:
    Entity() = default;
    Entity(entt::entity entity, Scene* scene);
    Entity(const Entity& entity) = default;
    ~Entity()                    = default;

    template<typename T, typename... Args>
    T& AddComponent(Args&&... args)
    {
        BR_CORE_ASSERT(!HasComponent<T>(), "Entity already has this component!");

        T& component = m_scene->m_registry.emplace<T>(m_entityHandle, std::forward<Args>(args)...);
        m_scene->OnComponentAdded<T>(*this, component);
        return component;
    }

    template<typename T>
    T& GetComponentRef()
    {
        BR_CORE_ASSERT(HasComponent<T>(), "Entity does not have this component!");

        return m_scene->m_registry.get<T>(m_entityHandle);
    }

    template<typename T>
    const T& GetComponent() const
    {
        BR_CORE_ASSERT(HasComponent<T>(), "Entity does not have this component!");

        return m_scene->m_registry.get<T>(m_entityHandle);
    }

    template<typename T>
    void RemoveComponent()
    {
        BR_CORE_ASSERT(HasComponent<T>(), "Entity does not have this component!");

        m_scene->m_registry.remove<T>(m_entityHandle);
    }

    template<typename T>
    bool HasComponent() const
    {
        return m_scene->m_registry.has<T>(m_entityHandle);
    }

    operator bool() const { return m_entityHandle != entt::null; }
    operator uint32_t() const { return (uint32_t)m_entityHandle; }

    bool operator==(const Entity& other) const
    {
        return (m_entityHandle == other.m_entityHandle) && (m_scene == other.m_scene);
    }
    bool operator!=(const Entity& other) const { return !(*this == other); }

    operator entt::entity() const { return m_entityHandle; }

private:
    entt::entity m_entityHandle = entt::null;
    Scene*       m_scene        = nullptr;
};
}    // namespace Brigerad
