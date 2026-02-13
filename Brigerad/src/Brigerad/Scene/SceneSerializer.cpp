/**
 * @file    SceneSerializer
 * @author  Samuel Martel
 * @p       https://github.com/smartel99
 * @date    3/12/2021 1:08:59 PM
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

/*********************************************************************************************************************/
// [SECTION] Includes
/*********************************************************************************************************************/
#include "SceneSerializer.h"

#include "Entity.h"
#include "Components.h"
#include "YamlConverters.h"

#include <yaml-cpp/yaml.h>

#include <fstream>

namespace Brigerad
{
/*********************************************************************************************************************/
// [SECTION] Private Macro Definitions
/*********************************************************************************************************************/

YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
    return out;
}

/*********************************************************************************************************************/
// [SECTION] Private Function Declarations
/*********************************************************************************************************************/
static void SerializeEntity(YAML::Emitter& out, Entity entity);
static void DeserializeEntity(const YAML::Node& node, Entity entity);

static void        SerializeTagComponent(YAML::Emitter& out, Entity entity);
static std::string DeserializeTagComponent(const YAML::Node& node);

static void SerializeParentEntityComponent(YAML::Emitter& out, Entity entity);
static void DeserializeParentEntityComponent(const YAML::Node& node, Entity entity);

static void SerializeChildEntityComponent(YAML::Emitter& out, Entity entity);
static void DeserializeChildEntityComponent(const YAML::Node& node, Entity entity);

static void SerializeTransformComponent(YAML::Emitter& out, Entity entity);
static void DeserializeTransformComponent(const YAML::Node& node, Entity entity);

static void SerializeColorRendererComponent(YAML::Emitter& out, Entity entity);
static void DeserializeColorRendererComponent(const YAML::Node& node, Entity entity);

static void SerializeTextureRendererComponent(YAML::Emitter& out, Entity entity);
static void DeserializeTextureRendererComponent(const YAML::Node& node, Entity entity);

static void SerializeCameraComponent(YAML::Emitter& out, Entity entity);
static void DeserializeCameraComponent(const YAML::Node& node, Entity entity);

static void SerializeTextComponent(YAML::Emitter& out, Entity entity);
static void DeserializeTextComponent(const YAML::Node& node, Entity entity);

static void SerializeNativeScriptComponent(YAML::Emitter& out, Entity entity);
static void DeserializeNativeScriptComponent(const YAML::Node& node, Entity entity);

static void SerializeLuaScriptComponent(YAML::Emitter& out, Entity entity);
static void DeserializeLuaScriptComponent(const YAML::Node& node, Entity entity);

/*********************************************************************************************************************/
// [SECTION] Public Method Definitions
/*********************************************************************************************************************/

SceneSerializer::SceneSerializer(const Ref<Scene>& scene) : m_scene(scene)
{
}

void SceneSerializer::Serialize(const std::string& filepath)
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Scene" << YAML::Value << "Unnamed Scene";
    out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
    m_scene->m_registry.each([&](auto entityId) {
        Entity entity = {entityId, m_scene.get()};
        if (!entity)
        {
            return;
        }

        SerializeEntity(out, entity);
    });
    out << YAML::EndSeq;
    out << YAML::EndMap;

    std::ofstream fout(filepath);
    fout << out.c_str();
}

void SceneSerializer::SerializeRuntime([[maybe_unused]] const std::string& filepath)
{
    BR_CORE_ASSERT(false, "Not Implemented");
}

bool SceneSerializer::Deserialize(const std::string& filepath)
{
    std::ifstream     stream(filepath);
    std::stringstream ss;
    ss << stream.rdbuf();

    YAML::Node data = YAML::Load(ss.str());
    if (!data["Scene"])
    {
        BR_CORE_ERROR("No scene data in project file '{}'!", filepath);
        return false;
    }

    std::string sceneName = data["Scene"].as<std::string>();
    BR_CORE_TRACE("Deserializing scene '{}'", sceneName);

    auto entities = data["Entities"];
    if (entities)
    {
        // Deserialize all entities except those with ChildEntityComponents.
        for (const auto& serializedEntity : entities)
        {
            uint64_t uuid = serializedEntity["Entity"].as<uint64_t>();    // #TODO

            std::string name;
            if (serializedEntity["TagComponent"])
            {
                name = DeserializeTagComponent(serializedEntity["TagComponent"]);
            }

            if (serializedEntity["ChildEntityComponent"])
            {
                m_childNodes.emplace_back(uuid);
                continue;
            }

            Entity deserializedEntity = m_scene->CreateEntity(name);
            if (serializedEntity["ParentEntityComponent"])
            {
                m_parentEntities.emplace_back(deserializedEntity);
            }

            BR_CORE_TRACE("Deserialized node with ID = {}, name = {}", uuid, name);
            DeserializeEntity(serializedEntity, deserializedEntity);
        }

        for (const auto& childId : m_childNodes)
        {
            for (const auto& entity : entities)
            {
                if (entity["Entity"].as<uint64_t>() == childId)
                {
                    uint64_t uuid = entity["Entity"].as<uint64_t>();    // #TODO

                    std::string name;
                    if (entity["TagComponent"])
                    {
                        name = DeserializeTagComponent(entity["TagComponent"]);
                    }

                    uint64_t parentId = entity["ChildEntityComponent"]["ParentID"].as<uint64_t>();
                    for (const auto& p : m_parentEntities)
                    {
                        auto& pec = p.GetComponent<ParentEntityComponent>();
                        if (pec.uuid == parentId)
                        {
                            Entity childEntity = m_scene->CreateChildEntity(name, p);
                            BR_CORE_TRACE(
                              "Deserialized child node with ID = {}, name = {}", uuid, name);
                            DeserializeEntity(entity, childEntity);
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }

    return true;
}

bool SceneSerializer::DeserializeRuntime([[maybe_unused]] const std::string& filepath)
{
    BR_CORE_ASSERT(false, "Not Implemented");
    return false;
}

/*********************************************************************************************************************/
// [SECTION] Private Method Definitions
/*********************************************************************************************************************/


/*********************************************************************************************************************/
// [SECTION] Private Function Definitions
/*********************************************************************************************************************/
static void SerializeEntity(YAML::Emitter& out, Entity entity)
{
    out << YAML::BeginMap;    // In the Entity map of the scene.
    // #TODO Entity ID goes here.
    out << YAML::Key << "Entity" << YAML::Value << (uint32_t)entity;

    if (entity.HasComponent<TagComponent>())
    {
        SerializeTagComponent(out, entity);
    }

    if (entity.HasComponent<ParentEntityComponent>())
    {
        SerializeParentEntityComponent(out, entity);
    }

    if (entity.HasComponent<ChildEntityComponent>())
    {
        SerializeChildEntityComponent(out, entity);
    }

    if (entity.HasComponent<TransformComponent>())
    {
        SerializeTransformComponent(out, entity);
    }

    if (entity.HasComponent<ColorRendererComponent>())
    {
        SerializeColorRendererComponent(out, entity);
    }

    if (entity.HasComponent<TextureRendererComponent>())
    {
        SerializeTextureRendererComponent(out, entity);
    }

    if (entity.HasComponent<CameraComponent>())
    {
        SerializeCameraComponent(out, entity);
    }

    if (entity.HasComponent<TextComponent>())
    {
        SerializeTextComponent(out, entity);
    }

    if (entity.HasComponent<NativeScriptComponent>())
    {
        SerializeNativeScriptComponent(out, entity);
    }

    if (entity.HasComponent<LuaScriptComponent>())
    {
        SerializeLuaScriptComponent(out, entity);
    }

    out << YAML::EndMap;    // End of Entity map.
}

static void DeserializeEntity(const YAML::Node& node, Entity entity)
{
    if (node["ChildEntityComponent"])
    {
        DeserializeChildEntityComponent(node["ChildEntityComponent"], entity);
    }

    if (node["ParentEntityComponent"])
    {
        DeserializeParentEntityComponent(node["ParentEntityComponent"], entity);
    }

    if (node["TransformComponent"])
    {
        DeserializeTransformComponent(node["TransformComponent"], entity);
    }

    if (node["ColorRendererComponent"])
    {
        DeserializeColorRendererComponent(node["ColorRendererComponent"], entity);
    }

    if (node["TextureRendererComponent"])
    {
        DeserializeTextureRendererComponent(node["TextureRendererComponent"], entity);
    }

    if (node["CameraComponent"])
    {
        DeserializeCameraComponent(node["CameraComponent"], entity);
    }

    if (node["TextComponent"])
    {
        DeserializeTextComponent(node["TextComponent"], entity);
    }

    if (node["NativeScriptComponent"])
    {
        DeserializeNativeScriptComponent(node["NativeScriptComponent"], entity);
    }

    if (node["LuaScriptComponent"])
    {
        DeserializeLuaScriptComponent(node["LuaScriptComponent"], entity);
    }
}

static void SerializeTagComponent(YAML::Emitter& out, Entity entity)
{
    out << YAML::Key << "TagComponent";
    out << YAML::BeginMap;    // TagComponent.

    auto& tag = entity.GetComponent<TagComponent>().tag;
    out << YAML::Key << "Tag" << YAML::Value << tag;

    out << YAML::EndMap;    // TagComponent.
}

static std::string DeserializeTagComponent(const YAML::Node& node)
{
    return node["Tag"].as<std::string>();
}


static void SerializeParentEntityComponent(YAML::Emitter& out, Entity entity)
{
    out << YAML::Key << "ParentEntityComponent";
    out << YAML::BeginMap;

    auto& pec = entity.GetComponent<ParentEntityComponent>();
    out << YAML::Key << "UUID" << YAML::Value << pec.uuid;
    out << YAML::Key << "Child";
    out << YAML::Flow;
    out << YAML::BeginSeq;
    for (const auto& c : pec.childs)
    {
        out << YAML::Value << (uint32_t)c;
    }
    out << YAML::EndSeq;
    out << YAML::EndMap;
}

static void DeserializeParentEntityComponent(const YAML::Node& node, Entity entity)
{
    auto& pec = entity.AddComponent<ParentEntityComponent>();
    pec.uuid  = node["UUID"].as<uint64_t>();
}

static void SerializeChildEntityComponent(YAML::Emitter& out, Entity entity)
{
    out << YAML::Key << "ChildEntityComponent";
    out << YAML::BeginMap;    // ChildEntityComponent.

    out << YAML::Key << "ParentID";
    out << YAML::Value << (uint32_t)entity.GetComponent<ChildEntityComponent>().parent;

    out << YAML::EndMap;    // ChildEntityComponent.
}

static void DeserializeChildEntityComponent([[maybe_unused]] const YAML::Node& node,[[maybe_unused]] Entity entity)
{
    // We don't have to do anything here since the child is already created.
}

static void SerializeTransformComponent(YAML::Emitter& out, Entity entity)
{
    out << YAML::Key << "TransformComponent";
    out << YAML::BeginMap;

    auto& tc = entity.GetComponent<TransformComponent>();
    out << YAML::Key << "Position" << YAML::Value << tc.position;
    out << YAML::Key << "Rotation" << YAML::Value << tc.rotation;
    out << YAML::Key << "Scale" << YAML::Value << tc.scale;

    out << YAML::EndMap;
}

static void DeserializeTransformComponent(const YAML::Node& node, Entity entity)
{
    // Entities always have transforms.
    auto& tc    = entity.GetComponentRef<TransformComponent>();
    tc.position = node["Position"].as<glm::vec3>();
    tc.rotation = node["Rotation"].as<glm::vec3>();
    tc.scale    = node["Scale"].as<glm::vec3>();
}

static void SerializeColorRendererComponent(YAML::Emitter& out, Entity entity)
{
    out << YAML::Key << "ColorRendererComponent";
    out << YAML::BeginMap;    // ColorRendererComponent.

    auto& crc = entity.GetComponent<ColorRendererComponent>();
    out << YAML::Key << "Color" << YAML::Value << crc.color;

    out << YAML::EndMap;    // ColorRendererComponent.
}

static void DeserializeColorRendererComponent(const YAML::Node& node, Entity entity)
{
    auto& crc = entity.AddComponent<ColorRendererComponent>();
    crc.color = node["Color"].as<glm::vec4>();
}

static void SerializeTextureRendererComponent(YAML::Emitter& out, Entity entity)
{
    out << YAML::Key << "TextureRendererComponent";
    out << YAML::BeginMap;    // TextureRendererComponent.

    auto& trc = entity.GetComponent<TextureRendererComponent>();
    out << YAML::Key << "Path" << YAML::Value << trc.path;

    out << YAML::EndMap;    // TextureRendererComponent.
}

static void DeserializeTextureRendererComponent(const YAML::Node& node, Entity entity)
{
    std::string path = node["Path"].as<std::string>();
    entity.AddComponent<TextureRendererComponent>(path);
}

static void SerializeCameraComponent(YAML::Emitter& out, Entity entity)
{
    out << YAML::Key << "CameraComponent";
    out << YAML::BeginMap;    // CameraComponent.

    out << YAML::Key << "Camera" << YAML::BeginMap;    // Camera.
    auto& cc = entity.GetComponent<CameraComponent>();
    out << YAML::Key << "ProjectionType" << YAML::Value << (int)cc.camera.GetProjectionType();
    out << YAML::Key << "OrthographicSize" << YAML::Value << cc.camera.GetOrthographicSize();
    out << YAML::Key << "OrthographicNearClip" << YAML::Value
        << cc.camera.GetOrthographicNearClip();
    out << YAML::Key << "OrthographicFarClip" << YAML::Value << cc.camera.GetOrthographicFarClip();
    out << YAML::Key << "PerspectiveNearClip" << YAML::Value << cc.camera.GetPerspectiveNearClip();
    out << YAML::Key << "PerspectiveFarClip" << YAML::Value << cc.camera.GetPerspectiveFarClip();
    out << YAML::Key << "PerspectiveFov" << YAML::Value << cc.camera.GetPerspectiveFov();
    out << YAML::EndMap;    // Camera.

    out << YAML::Key << "Primary" << YAML::Value << cc.primary;
    out << YAML::Key << "FixedAspectRatio" << YAML::Value << cc.fixedAspectRatio;

    out << YAML::EndMap;    // CameraComponent.
}

static void DeserializeCameraComponent(const YAML::Node& node, Entity entity)
{
    auto& cc = entity.AddComponent<CameraComponent>();
    cc.camera.SetProjectionType(
      (SceneCamera::ProjectionType)node["Camera"]["ProjectionType"].as<int>());
    cc.camera.SetOrthographicSize(node["Camera"]["OrthographicSize"].as<float>());
    cc.camera.SetOrthographicNearClip(node["Camera"]["OrthographicNearClip"].as<float>());
    cc.camera.SetOrthographicFarClip(node["Camera"]["OrthographicFarClip"].as<float>());
    cc.camera.SetPerspectiveNearClip(node["Camera"]["PerspectiveNearClip"].as<float>());
    cc.camera.SetPerspectiveFarClip(node["Camera"]["PerspectiveFarClip"].as<float>());
    cc.camera.SetPerspectiveFov(node["Camera"]["PerspectiveFov"].as<float>());

    cc.primary          = node["Primary"].as<bool>();
    cc.fixedAspectRatio = node["FixedAspectRatio"].as<bool>();
}

static void SerializeTextComponent(YAML::Emitter& out, Entity entity)
{
    out << YAML::Key << "TextComponent";
    out << YAML::BeginMap;    // TextComponent.

    auto& tc = entity.GetComponent<TextComponent>();
    out << YAML::Key << "Text" << YAML::Value << tc.text;
    out << YAML::Key << "Scale" << YAML::Value << tc.scale;

    out << YAML::EndMap;    // TextComponent.
}

static void DeserializeTextComponent(const YAML::Node& node, Entity entity)
{
    auto& tc = entity.AddComponent<TextComponent>();
    tc.text  = node["Text"].as<std::string>();
    tc.scale = node["Scale"].as<float>();
}

static void SerializeNativeScriptComponent(YAML::Emitter& out, Entity entity)
{
    out << YAML::Key << "NativeScriptComponent";
    out << YAML::BeginMap;    // NativeScriptComponent.

    [[maybe_unused]] auto& nsc = entity.GetComponent<NativeScriptComponent>();
    out << YAML::Key << "Placeholder" << YAML::Value << "Value";

    out << YAML::EndMap;    // NativeScriptComponent.
}

static void DeserializeNativeScriptComponent(const YAML::Node& node, Entity entity)
{
    // #TODO
}


static void SerializeLuaScriptComponent(YAML::Emitter& out, Entity entity)
{
    out << YAML::Key << "LuaScriptComponent";
    out << YAML::BeginMap;    // LuaScriptComponent.

    auto& lsc = entity.GetComponent<LuaScriptComponent>();
    out << YAML::Key << "Path" << YAML::Value << lsc.path;
    out << YAML::Key << "Name" << YAML::Value << lsc.name;

    out << YAML::EndMap;    // LuaScriptComponent.
}

static void DeserializeLuaScriptComponent(const YAML::Node& node, Entity entity)
{
    std::string path = node["Path"].as<std::string>();
    std::string name = node["Name"].as<std::string>();
    entity.AddComponent<LuaScriptComponent>(path, name);
}
}    // namespace Brigerad
