/**
 * @file    SceneSerializer
 * @author  Samuel Martel
 * @p       https://github.com/smartel99
 * @date    3/12/2021 1:08:46 PM
 *
 * @brief
 ******************************************************************************
 * Copyright (C) 2021  Samuel Martel
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


#include <vector>

namespace YAML
{
class Node;
}
namespace Brigerad
{
/*********************************************************************************************************************/
// [SECTION] Defines
/*********************************************************************************************************************/


/*********************************************************************************************************************/
// [SECTION] Class Declarations
/*********************************************************************************************************************/
class SceneSerializer
{
public:
    SceneSerializer(const Ref<Scene>& scene);

    void Serialize(const std::string& filepath);
    void SerializeRuntime(const std::string& filepath);

    bool Deserialize(const std::string& filepath);
    bool DeserializeRuntime(const std::string& filepath);

private:
    Ref<Scene>            m_scene;
    std::vector<Entity>   m_parentEntities;
    std::vector<uint64_t> m_childNodes;
};
}    // namespace Brigerad
