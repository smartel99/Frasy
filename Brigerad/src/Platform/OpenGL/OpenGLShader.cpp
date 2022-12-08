/**
 * @file   OpenGLShader.cpp
 * @author Samuel Martel
 * @date   2020/03/07
 *
 * @brief  Source for the Shader module.
 */

#include "OpenGLShader.h"

#include "../../Brigerad/Debug/Instrumentor.h"

#include <fstream>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace Brigerad
{
// ID of the shader currently bound in the GPU.
// Init to an ID that is for sure non-existant.
static uint32_t pActiveShader = -1;

/**
 * @brief Get the OpenGL enum value from the passed string.
 *
 * @param type The type of the shader
 * @return GLenum The OpenGL type of the shader
 */
static GLenum ShaderTypeFromString(const std::string& type)
{
    if (type == "vertex")
    {
        return GL_VERTEX_SHADER;
    }
    else if (type == "fragment" || type == "pixel")
    {
        return GL_FRAGMENT_SHADER;
    }
    else
    {
        BR_CORE_ASSERT(false, "Unknown shader type!");
        return 0;
    }
}


/**
 * @brief Construct a new OpenGLShader object from a file.
 *
 * @param filePath The file to get the Vertex and Fragment shaders from.
 *
 * @attention Both Vertex and Fragment shaders needs to be present in the file
 *            for successful compilation.
 */
OpenGLShader::OpenGLShader(const std::string& filePath)
{
    BR_PROFILE_FUNCTION();

    // Dump the file into a string.
    std::string src = ReadFile(filePath);
    // Split the vertex and fragment shaders into key-value pairs.
    auto shaderSources = PreProcess(src);
    // Compile the shaders.
    Compile(shaderSources);

    // Extract name from path.
    size_t lastSlash = filePath.find_last_of(R"(/\)");
    lastSlash        = lastSlash == std::string::npos ? 0 : lastSlash + 1;
    size_t lastDot   = filePath.rfind('.');

    size_t count = lastDot == std::string::npos ? filePath.size() - lastSlash :
                                                  lastDot - lastSlash;
    m_name = filePath.substr(lastSlash, count);
}


/**
 * @brief Construct a new OpenGLShader object using shaders passed as parameters.
 *
 * @param name The debug name of the shader.
 * @param vertexSrc The code of the vertex shader.
 * @param fragmentSrc The code of the fragment shader.
 */
OpenGLShader::OpenGLShader(const std::string& name,
                           const std::string& vertexSrc,
                           const std::string& fragmentSrc)
: m_rendererID(0), m_name(name)
{
    BR_PROFILE_FUNCTION();

    // Put the source strings into an ordered map.
    std::unordered_map<GLenum, std::string> srcs;
    srcs[GL_VERTEX_SHADER]   = vertexSrc;
    srcs[GL_FRAGMENT_SHADER] = fragmentSrc;
    // Compile the two shaders.
    Compile(srcs);
}


/**
 * @brief Destroy the OpenGLShader object.
 *        This removes the shader program from the OpenGL context.
 */
OpenGLShader::~OpenGLShader()
{
    BR_PROFILE_FUNCTION();
    glDeleteProgram(m_rendererID);
}


/**
 * @brief Read the entire content of the file pointed by `filePath`, if it exists.
 *
 * @param filePath The path of the file to read.
 * @return std::string The content read from the file.
 */
std::string OpenGLShader::ReadFile(const std::string& filePath)
{
    BR_PROFILE_FUNCTION();

    std::string result = "";
    std::ifstream in = std::ifstream(filePath, std::ios::in | std::ios::binary);
    if (in.is_open())
    {
        // Move to the very end of the file.
        in.seekg(0, std::ios::end);
        // Use the position of the file pointer as the size of our string.
        result.resize(in.tellg());

        // Go back to the start.
        in.seekg(0, std::ios::beg);
        // Read the entire file.
        in.read(&result[0], result.size());
        // Close the file.
        in.close();
    }
    else
    {
        BR_CORE_ERROR("Could not open file '{0}'", filePath);
    }

    return result;
}


/**
 * @brief   Pre-processes the source file to ensure a valid structure.
 *          This process takes the content of the file and ensures that there is
 *          at least one "#type vertex" token and either one "#type fragment" or
 *          "#type pixel" token.
 *
 * @param source The source file.
 * @return  std::unordered_map<GLenum, std::string> A map containing the
 *          separated shader sources.
 */
std::unordered_map<GLenum, std::string> OpenGLShader::PreProcess(const std::string& source)
{
    BR_PROFILE_FUNCTION();

    std::unordered_map<GLenum, std::string> shaderSources;

    const char* typeToken = "#type";
    size_t typeTokenLen   = strlen(typeToken);
    // Find the first "#type" token.
    size_t pos = source.find(typeToken, 0);
    // For as long as we can find a new "#type" token:
    while (pos != std::string::npos)
    {
        // Find the end of the line from the "#type" token.
        size_t eol = source.find_first_of("\r\n", pos);
        // If there's no EOL, the source is ill-formated.
        BR_CORE_ASSERT(eol != std::string::npos, "Syntax error");
        // The beginning of the type of shader, assumed to be formated as such:
        // #type [type]
        // Anything else will be considered as ill-formed, such as the
        // following:
        // #type   [type]
        size_t begin = pos + typeTokenLen + 1;
        // Get the shader type and make sure it's a valid one.
        std::string type = source.substr(begin, eol - begin);
        BR_CORE_ASSERT(ShaderTypeFromString(type), "Invalid shader type specified");

        // Find the next "#type" token.
        size_t nextLinePos = source.find_first_not_of("\r\n", eol);
        pos                = source.find(typeToken, nextLinePos);
        // Add the source to the map.
        shaderSources[ShaderTypeFromString(type)] =
          source.substr(nextLinePos,
                        pos - (nextLinePos == std::string::npos ? source.size() - 1 : nextLinePos));
    }

    return shaderSources;
}

/**
 * @brief Compile all of the provided shaders and links them into an OpenGL program.
 *
 * @param shaderSrcs The shaders to compile.
 */
void OpenGLShader::Compile(const std::unordered_map<GLenum, std::string>& shaderSrcs)
{
    BR_PROFILE_FUNCTION();
    // Create an OpenGL program.
    GLuint program = glCreateProgram();
    BR_CORE_ASSERT(shaderSrcs.size() <= 2, "Maximum 2 shaders per file");
    std::array<GLuint, 2> shaderIDs;
    int shaderIdIdx = 0;

    // For each shaders in the map:
    for (auto& kv : shaderSrcs)
    {
        GLenum type               = kv.first;
        const std::string& source = kv.second;

        // Create an empty shader handle.
        GLuint shader = glCreateShader(type);

        // Send the shader source code to GL.
        const GLchar* sourceCStr = (const GLchar*)source.c_str();
        glShaderSource(shader, 1, &sourceCStr, nullptr);

        // Compile the shader.
        glCompileShader(shader);

        // Make sure that the shader compiled successfully.
        GLint isCompiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

            // The maxLength includes the NULL terminator.
            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

            // We don't need that shader anymore.
            glDeleteShader(shader);

            BR_CORE_ERROR("{0}", infoLog.data());
            BR_CORE_ASSERT(false, "Unable to compile vertex shader");
        }

        // Attach our shaders to our program
        glAttachShader(program, shader);
        shaderIDs[shaderIdIdx++] = shader;
    }

    // Vertex and Fragment shaders are successfully compiled.
    // Now time to link them together into a program.
    // Link our program.
    glLinkProgram(program);

    // Note the different functions here: glGetProgram* instead of glGetShader*.
    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
    if (isLinked == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL terminator.
        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

        // Delete the program.
        glDeleteProgram(program);

        // We don't need the shaders anymore.
        for (auto id : shaderIDs)
        {
            glDeleteShader(id);
        }

        BR_CORE_ERROR("{0}", infoLog.data());
        BR_CORE_ASSERT(false, "Unable to link shader");

        return;
    }

    // Detach shaders after successful link.
    for (auto id : shaderIDs)
    {
        glDetachShader(program, id);
    }
    m_rendererID = program;
}


/**
 * @brief Bind the shader, if it is not already bound.
 */
void OpenGLShader::Bind() const
{
    BR_PROFILE_FUNCTION();

    // If this shader is not the currently bound shader:
    if (m_rendererID != pActiveShader)
    {
        // Bind it.
        pActiveShader = m_rendererID;
        glUseProgram(m_rendererID);
    }
}

/**
 * @brief Unbind the shader.
 *
 */
void OpenGLShader::Unbind() const
{
    BR_PROFILE_FUNCTION();

    pActiveShader = 0;
    glUseProgram(0);
}


/**
 * @brief Set an integer uniform value in the shader.
 *
 * @param name The name of the uniform.
 * @param value The value to assign to that uniform.
 */
void OpenGLShader::SetInt(const std::string& name, int value)
{
    BR_PROFILE_FUNCTION();

    UploadUniformInt(name, value);
}

/**
 * @brief Set a uniform array of integer values in the shader.
 *
 * @param name The name of the uniform.
 * @param values The values to assign to that uniform.
 * @param count The number of values to assign to that uniform.
 */
void OpenGLShader::SetIntArray(const std::string& name, int* values, uint32_t count)
{
    BR_PROFILE_FUNCTION();

    UploadUniformIntArray(name, values, count);
}

/**
 * @brief Set a float uniform value in the shader.
 *
 * @param name The name of the uniform.
 * @param value The value to assign to that uniform.
 */
void OpenGLShader::SetFloat(const std::string& name, float value)
{
    BR_PROFILE_FUNCTION();

    UploadUniformFloat(name, value);
}

/**
 * @brief Set a float2 vector uniform value in the shader.
 *
 * @param name The name of the uniform.
 * @param value The value to assign to that uniform.
 */
void OpenGLShader::SetFloat2(const std::string& name, const glm::vec2& value)
{
    BR_PROFILE_FUNCTION();

    UploadUniformFloat2(name, value);
}

/**
 * @brief Set a float3 vector uniform value in the shader.
 *
 * @param name The name of the uniform.
 * @param value The value to assign to that uniform.
 */
void OpenGLShader::SetFloat3(const std::string& name, const glm::vec3& value)
{
    BR_PROFILE_FUNCTION();

    UploadUniformFloat3(name, value);
}

/**
 * @brief Set a float4 vector uniform value in the shader.
 *
 * @param name The name of the uniform.
 * @param value The value to assign to that uniform.
 */
void OpenGLShader::SetFloat4(const std::string& name, const glm::vec4& value)
{
    BR_PROFILE_FUNCTION();

    UploadUniformFloat4(name, value);
}

/**
 * @brief Set a mat3 uniform value in the shader.
 *
 * @param name The name of the uniform.
 * @param value The value to assign to that uniform.
 */
void OpenGLShader::SetMat3(const std::string& name, const glm::mat3& value)
{
    BR_PROFILE_FUNCTION();

    UploadUniformMat3(name, value);
}

/**
 * @brief Set a mat4 vector uniform value in the shader.
 *
 * @param name The name of the uniform.
 * @param value The value to assign to that uniform.
 */
void OpenGLShader::SetMat4(const std::string& name, const glm::mat4& value)
{
    BR_PROFILE_FUNCTION();

    UploadUniformMat4(name, value);
}


/* ------------------------------------------------------------------------- */
/* OpenGL-level API                                                          */
/* ------------------------------------------------------------------------- */

/**
 * @brief Upload an integer to an uniform in the GPU.
 *
 * @param name The name of the uniform.
 * @param value The value to upload in the GPU.
 */
void OpenGLShader::UploadUniformInt(const std::string& name, int value)
{
    GLint location = glGetUniformLocation(m_rendererID, name.c_str());
    glUniform1i(location, value);
}

void OpenGLShader::UploadUniformIntArray(const std::string& name, int* values, uint32_t count)
{
    GLint location = glGetUniformLocation(m_rendererID, name.c_str());
    glUniform1iv(location, count, values);
}

/**
 * @brief Upload a float to an uniform in the GPU.
 *
 * @param name The name of the uniform.
 * @param value The value to upload in the GPU.
 */
void OpenGLShader::UploadUniformFloat(const std::string& name, float value)
{
    GLint location = glGetUniformLocation(m_rendererID, name.c_str());
    glUniform1f(location, value);
}

/**
 * @brief Upload a float2 vector to an uniform in the GPU.
 *
 * @param name The name of the uniform.
 * @param value The value to upload in the GPU.
 */
void OpenGLShader::UploadUniformFloat2(const std::string& name, const glm::vec2& values)
{
    GLint location = glGetUniformLocation(m_rendererID, name.c_str());
    glUniform2f(location, values.x, values.y);
}

/**
 * @brief Upload a float3 vector to an uniform in the GPU.
 *
 * @param name The name of the uniform.
 * @param value The value to upload in the GPU.
 */
void OpenGLShader::UploadUniformFloat3(const std::string& name, const glm::vec3& values)
{
    GLint location = glGetUniformLocation(m_rendererID, name.c_str());
    glUniform3f(location, values.x, values.y, values.z);
}

/**
 * @brief Upload a float4 vector to an uniform in the GPU.
 *
 * @param name The name of the uniform.
 * @param value The value to upload in the GPU.
 */
void OpenGLShader::UploadUniformFloat4(const std::string& name, const glm::vec4& values)
{
    GLint location = glGetUniformLocation(m_rendererID, name.c_str());
    glUniform4f(location, values.x, values.y, values.z, values.w);
}

/**
 * @brief Upload a mat3 vector to an uniform in the GPU.
 *
 * @param name The name of the uniform.
 * @param value The value to upload in the GPU.
 */
void OpenGLShader::UploadUniformMat3(const std::string& name, const glm::mat3& matrix)
{
    GLint location = glGetUniformLocation(m_rendererID, name.c_str());
    glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
}

/**
 * @brief Upload a mat4 vector to an uniform in the GPU.
 *
 * @param name The name of the uniform.
 * @param value The value to upload in the GPU.
 */
void OpenGLShader::UploadUniformMat4(const std::string& name, const glm::mat4& matrix)
{
    GLint location = glGetUniformLocation(m_rendererID, name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
}

}  // namespace Brigerad
