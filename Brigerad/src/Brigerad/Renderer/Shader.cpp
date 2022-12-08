/**
 * @file   Shader.cpp
 * @author Samuel Martel
 * @date   2020/03/07
 *
 * @brief  Source for the Shader module.
 */

#include "Shader.h"

#include "Renderer.h"

#include "../../Platform/OpenGL/OpenGLShader.h"

namespace Brigerad
{
Ref<Shader> Shader::Create(const std::string& filePath)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            BR_CORE_ASSERT(false, "RendererAPI::None is not a valid rendering API!");
            return nullptr;
        case RendererAPI::API::OpenGL: return std::make_shared<OpenGLShader>(filePath);
        default: BR_CORE_ASSERT(false, "Invalid rendering API!"); return nullptr;
    }
}


Ref<Shader> Shader::Create(const std::string& name,
                           const std::string& vertexSrc,
                           const std::string& fragmentSrc)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            BR_CORE_ASSERT(false, "RendererAPI::None is not a valid rendering API!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return std::make_shared<OpenGLShader>(name, vertexSrc, fragmentSrc);
        default: BR_CORE_ASSERT(false, "Invalid rendering API!"); return nullptr;
    }
}



void ShaderLibrary::Add(const Ref<Shader>& shader)
{
    auto& name = shader->GetName();
    BR_CORE_ASSERT(!Exists(name), "Shader already exists!");
    Add(name, shader);
}

void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader)
{
    BR_CORE_ASSERT(!Exists(name), "Shader already exists!");
    m_shaders[name] = shader;
}


Brigerad::Ref<Brigerad::Shader> ShaderLibrary::Load(const std::string& filePath)
{
    auto shader = Shader::Create(filePath);
    Add(shader);
    return shader;
}


Brigerad::Ref<Brigerad::Shader> ShaderLibrary::Load(const std::string& name,
                                                    const std::string& filePath)
{
    auto shader = Shader::Create(filePath);
    Add(name, shader);
    return shader;
}


Brigerad::Ref<Brigerad::Shader> ShaderLibrary::Get(const std::string& name)
{
    BR_CORE_ASSERT(Exists(name), "Shader not found!");

    return m_shaders[name];
}

bool ShaderLibrary::Exists(const std::string& name) const
{
    return m_shaders.find(name) != m_shaders.end();
}


}    // namespace Brigerad
