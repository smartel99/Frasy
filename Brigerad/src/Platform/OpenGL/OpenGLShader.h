#pragma once

#include "Brigerad/Renderer/Shader.h"

#include <unordered_map>

// TODO: REMOVE:
typedef unsigned int GLenum;

namespace Brigerad
{
class OpenGLShader : public Shader
{
    public:
    OpenGLShader(const std::string& filePath);
    OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
    ~OpenGLShader() override;

    void Bind() const override;
    void Unbind() const override;

    virtual void SetInt(const std::string& name, int value) override;
    virtual void SetIntArray(const std::string& name, int* values, uint32_t count) override;

    virtual void SetFloat(const std::string& name, float value) override;
    virtual void SetFloat2(const std::string& name, const glm::vec2& value) override;
    virtual void SetFloat3(const std::string& name, const glm::vec3& value) override;
    virtual void SetFloat4(const std::string& name, const glm::vec4& value) override;

    virtual void SetMat3(const std::string& name, const glm::mat3& value) override;
    virtual void SetMat4(const std::string& name, const glm::mat4& value) override;

    void UploadUniformInt(const std::string& name, int value);
    void UploadUniformIntArray(const std::string& name, int* values, uint32_t count);

    void UploadUniformFloat(const std::string& name, float values);
    void UploadUniformFloat2(const std::string& name, const glm::vec2& values);
    void UploadUniformFloat3(const std::string& name, const glm::vec3& values);
    void UploadUniformFloat4(const std::string& name, const glm::vec4& values);

    void UploadUniformMat3(const std::string& name, const glm::mat3& matrix);
    void UploadUniformMat4(const std::string& name, const glm::mat4& matrix);

    virtual const std::string& GetName() const override { return m_name; }

    const uint32_t GetId() const { return m_rendererID; }

    private:
    std::string ReadFile(const std::string& filePath);
    std::unordered_map<GLenum, std::string> PreProcess(const std::string& source);
    void Compile(const std::unordered_map<GLenum, std::string>& shaderSrcs);

    private:
    uint32_t m_rendererID;  // Internal OpenGL program ID.
    std::string m_name;     // Debug name of the shader.
};
}  // namespace Brigerad