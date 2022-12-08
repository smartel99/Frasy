/**
 * @file    ScriptEngineRegistryGLM
 * @author  Samuel Martel
 * @p       https://github.com/smartel99
 * @date    10/18/2020 3:49:59 PM
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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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


/*********************************************************************************************************************/
// [SECTION] Public Method Definitions
/*********************************************************************************************************************/


/*********************************************************************************************************************/
// [SECTION] Private Method Definitions
/*********************************************************************************************************************/

void ScriptEngineRegistry::RegisterVec2()
{
    auto lua = Scripting::GetState();

    auto addOverloads =
      sol::overload([](const glm::vec2& v1, const glm::vec2& v2) -> glm::vec2 { return v1 + v2; },
                    [](const glm::vec2& v1, float f) -> glm::vec2 { return v1 + f; },
                    [](float f, const glm::vec2& v1) -> glm::vec2 { return v1 + f; });

    auto subOverloads =
      sol::overload([](const glm::vec2& v1, const glm::vec2& v2) -> glm::vec2 { return v1 - v2; },
                    [](const glm::vec2& v1, float f) -> glm::vec2 { return v1 - f; },
                    [](float f, const glm::vec2& v1) -> glm::vec2 { return v1 - f; });

    auto multOverloads =
      sol::overload([](const glm::vec2& v1, const glm::vec2& v2) -> glm::vec2 { return v1 * v2; },
                    [](const glm::vec2& v1, float f) -> glm::vec2 { return v1 * f; },
                    [](float f, const glm::vec2& v1) -> glm::vec2 { return v1 * f; });

    auto divOverloads =
      sol::overload([](const glm::vec2& v1, const glm::vec2& v2) -> glm::vec2 { return v1 - v2; },
                    [](const glm::vec2& v1, float f) -> glm::vec2 { return v1 - f; },
                    [](float f, const glm::vec2& v1) -> glm::vec2 { return v1 - f; });

    auto eqOverloads =
      sol::overload([](const glm::vec2& v1, const glm::vec2& v2) -> bool { return v1 == v2; });

    lua->new_usertype<glm::vec2>(
      "vec2",
      sol::constructors<glm::vec2(), glm::vec2(float), glm::vec2(float, float)>(),
      "x",
      &glm::vec2::x,
      "y",
      &glm::vec2::y,
      sol::meta_function::addition,
      addOverloads,
      sol::meta_function::subtraction,
      subOverloads,
      sol::meta_function::multiplication,
      multOverloads,
      sol::meta_function::division,
      divOverloads,
      sol::meta_function::equal_to,
      eqOverloads);
}

void ScriptEngineRegistry::RegisterVec3()
{
    auto lua = Scripting::GetState();

    auto addOverloads =
      sol::overload([](const glm::vec3& v1, const glm::vec3& v2) -> glm::vec3 { return v1 + v2; },
                    [](const glm::vec3& v1, float f) -> glm::vec3 { return v1 + f; },
                    [](float f, const glm::vec3& v1) -> glm::vec3 { return v1 + f; });

    auto subOverloads =
      sol::overload([](const glm::vec3& v1, const glm::vec3& v2) -> glm::vec3 { return v1 - v2; },
                    [](const glm::vec3& v1, float f) -> glm::vec3 { return v1 - f; },
                    [](float f, const glm::vec3& v1) -> glm::vec3 { return v1 - f; });

    auto multOverloads =
      sol::overload([](const glm::vec3& v1, const glm::vec3& v2) -> glm::vec3 { return v1 * v2; },
                    [](const glm::vec3& v1, float f) -> glm::vec3 { return v1 * f; },
                    [](float f, const glm::vec3& v1) -> glm::vec3 { return v1 * f; });

    auto divOverloads =
      sol::overload([](const glm::vec3& v1, const glm::vec3& v2) -> glm::vec3 { return v1 - v2; },
                    [](const glm::vec3& v1, float f) -> glm::vec3 { return v1 - f; },
                    [](float f, const glm::vec3& v1) -> glm::vec3 { return v1 - f; });

    auto eqOverloads =
      sol::overload([](const glm::vec3& v1, const glm::vec3& v2) -> bool { return v1 == v2; });

    lua->new_usertype<glm::vec3>(
      "vec3",
      sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(),
      "x",
      &glm::vec3::x,
      "y",
      &glm::vec3::y,
      "z",
      &glm::vec3::z,
      sol::meta_function::addition,
      addOverloads,
      sol::meta_function::subtraction,
      subOverloads,
      sol::meta_function::multiplication,
      multOverloads,
      sol::meta_function::division,
      divOverloads,
      sol::meta_function::equal_to,
      eqOverloads);
}

void ScriptEngineRegistry::RegisterVec4()
{
    auto lua = Scripting::GetState();

    auto addOverloads =
      sol::overload([](const glm::vec4& v1, const glm::vec4& v2) -> glm::vec4 { return v1 + v2; },
                    [](const glm::vec4& v1, float f) -> glm::vec4 { return v1 + f; },
                    [](float f, const glm::vec4& v1) -> glm::vec4 { return v1 + f; });

    auto subOverloads =
      sol::overload([](const glm::vec4& v1, const glm::vec4& v2) -> glm::vec4 { return v1 - v2; },
                    [](const glm::vec4& v1, float f) -> glm::vec4 { return v1 - f; },
                    [](float f, const glm::vec4& v1) -> glm::vec4 { return v1 - f; });

    auto multOverloads =
      sol::overload([](const glm::vec4& v1, const glm::vec4& v2) -> glm::vec4 { return v1 * v2; },
                    [](const glm::vec4& v1, float f) -> glm::vec4 { return v1 * f; },
                    [](float f, const glm::vec4& v1) -> glm::vec4 { return v1 * f; });

    auto divOverloads =
      sol::overload([](const glm::vec4& v1, const glm::vec4& v2) -> glm::vec4 { return v1 - v2; },
                    [](const glm::vec4& v1, float f) -> glm::vec4 { return v1 - f; },
                    [](float f, const glm::vec4& v1) -> glm::vec4 { return v1 - f; });

    auto eqOverloads =
      sol::overload([](const glm::vec4& v1, const glm::vec4& v2) -> bool { return v1 == v2; });

    lua->new_usertype<glm::vec4>(
      "vec4",
      sol::constructors<glm::vec4(), glm::vec4(float), glm::vec4(float, float, float, float)>(),
      "x",
      &glm::vec4::x,
      "y",
      &glm::vec4::y,
      "z",
      &glm::vec4::z,
      "w",
      &glm::vec4::w,
      sol::meta_function::addition,
      addOverloads,
      sol::meta_function::subtraction,
      subOverloads,
      sol::meta_function::multiplication,
      multOverloads,
      sol::meta_function::division,
      divOverloads,
      sol::meta_function::equal_to,
      eqOverloads);
}

void ScriptEngineRegistry::RegisterMat4()
{
    auto lua = Scripting::GetState();

    auto mat4         = lua->new_usertype<glm::mat4>("mat4");
    mat4["Transform"] = [](const glm::mat4& translation,
                           const glm::mat4& rotation,
                           const glm::mat4& scale) -> glm::mat4
    { return translation * rotation * scale; };
}

/*********************************************************************************************************************/
// [SECTION] Private Function Declarations
/*********************************************************************************************************************/
}    // namespace Brigerad
