/**
 * @file    ScriptEngineRegistryECS
 * @author  Samuel Martel
 * @p       https://github.com/smartel99
 * @date    10/18/2020 3:57:30 PM
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

/*********************************************************************************************************************/
// [SECTION] Includes
/*********************************************************************************************************************/
#include "ScriptEngineRegistry.h"

#define SOL_ALL_SAFETIES_ON 1
#define SOL_SAFE_USERTYPE   1
#define SOL_CHECK_ARGUMENTS 1
#include <sol/sol.hpp>

#include "../Scene/Components.h"
#include "../Scene/Entity.h"

#include <string>


namespace Brigerad
{
namespace Scripting
{
extern sol::state* GetState();
}
/*********************************************************************************************************************/
// [SECTION] Private Macro Definitions
/*********************************************************************************************************************/


/*********************************************************************************************************************/
// [SECTION] Private Function Declarations
/*********************************************************************************************************************/
inline static void RegisterTagComponent();
inline static void RegisterTransformComponent();
inline static void RegisterColorRendererComponent();

/*********************************************************************************************************************/
// [SECTION] Public Method Definitions
/*********************************************************************************************************************/


/*********************************************************************************************************************/
// [SECTION] Private Method Definitions
/*********************************************************************************************************************/
void Brigerad::ScriptEngineRegistry::RegisterEntity()
{
    [[maybe_unused]] auto lua = Scripting::GetState();
}

void Brigerad::ScriptEngineRegistry::RegisterComponents()
{
    RegisterTagComponent();
    RegisterTransformComponent();
    RegisterColorRendererComponent();
}

void RegisterTagComponent()
{
    auto lua = Scripting::GetState();

    auto tagComponent   = lua->new_usertype<TagComponent>("TagComponent", sol::no_constructor);
    tagComponent["tag"] = &TagComponent::tag;
}

void RegisterTransformComponent()
{
    auto lua = Scripting::GetState();

    auto transComponent =
      lua->new_usertype<TransformComponent>("TransformComponent", sol::no_constructor);
    transComponent["GetPosition"] = &TransformComponent::GetPosition;
    transComponent["SetPosition"] = &TransformComponent::SetPosition;
    transComponent["GetRotation"] = &TransformComponent::GetRotation;
    transComponent["SetRotation"] = &TransformComponent::SetRotation;
    transComponent["GetScale"]    = &TransformComponent::GetScale;
    transComponent["SetScale"]    = &TransformComponent::SetScale;
}

void RegisterColorRendererComponent()
{
    auto lua = Scripting::GetState();

    auto colorComponent =
      lua->new_usertype<ColorRendererComponent>("ColorRendererComponent", sol::no_constructor);
    colorComponent["color"] = &ColorRendererComponent::color;
}

/*********************************************************************************************************************/
// [SECTION] Private Function Declarations
/*********************************************************************************************************************/
}    // namespace Brigerad
