#pragma once

#include "../Core/Core.h"
#include "../Core/Log.h"

#include <string>
#include <vector>

namespace Brigerad
{
enum class ShaderDataType
{
    None = 0,
    Float,
    Float2,
    Float3,
    Float4,
    Mat3,
    Mat4,
    Int,
    Int2,
    Int3,
    Int4,
    Bool
};

static uint32_t ShaderDataTypeSize(ShaderDataType type)
{
    switch (type)
    {
        case ShaderDataType::None: return 0;
        case ShaderDataType::Float: return 4;
        case ShaderDataType::Float2: return 4 * 2;
        case ShaderDataType::Float3: return 4 * 3;
        case ShaderDataType::Float4: return 4 * 4;
        case ShaderDataType::Mat3: return 4 * 3 * 3;
        case ShaderDataType::Mat4: return 4 * 4 * 4;
        case ShaderDataType::Int: return 4;
        case ShaderDataType::Int2: return 4 * 2;
        case ShaderDataType::Int3: return 4 * 3;
        case ShaderDataType::Int4: return 4 * 4;
        case ShaderDataType::Bool:
            return 1;
            //        default: BR_CORE_ASSERT(false, "Unknown ShaderDataType"); return 0;
        default:
            ::Brigerad::Log::GetLogger("asdf")->log(
              ::Brigerad::Log::FormatSourceLocation(std::source_location::current()),
              spdlog::level::err,
              "Assertion Failed: {}",
              "Unknown shader data type");
            return 0;
    }
}

struct BufferElements
{
    std::string    name;
    ShaderDataType type;
    uint32_t       size;
    uint32_t       offset;
    bool           normalized;

    BufferElements(ShaderDataType t, const std::string& n, bool norm = false)
    : name(n), type(t), size(ShaderDataTypeSize(t)), offset(0), normalized(norm)
    {
    }

    BufferElements() : type(ShaderDataType::None), size(0), offset(0), normalized(false) {}

    uint32_t GetComponentCount() const
    {
        switch (type)
        {
            case ShaderDataType::None: return 0;
            case ShaderDataType::Float: return 1;
            case ShaderDataType::Float2: return 2;
            case ShaderDataType::Float3: return 3;
            case ShaderDataType::Float4: return 4;
            case ShaderDataType::Mat3: return 9;
            case ShaderDataType::Mat4: return 16;
            case ShaderDataType::Int: return 1;
            case ShaderDataType::Int2: return 2;
            case ShaderDataType::Int3: return 3;
            case ShaderDataType::Int4: return 4;
            case ShaderDataType::Bool: return 1;
            default: BR_CORE_ASSERT(false, "Invalid ShaderDataType!", "asdf"); return 0;
        }
    }
};

class BufferLayout
{
public:
    BufferLayout(const std::initializer_list<BufferElements>& elements) : m_elements(elements)
    {
        CalculateOffsetsAndStride();
    }

    BufferLayout() {}

    const std::vector<BufferElements>& GetElements() const { return m_elements; }

    uint32_t GetStride() const { return m_stride; }

    std::vector<BufferElements>::iterator begin() { return m_elements.begin(); }
    std::vector<BufferElements>::iterator end() { return m_elements.end(); }

    std::vector<BufferElements>::const_iterator begin() const { return m_elements.begin(); }
    std::vector<BufferElements>::const_iterator end() const { return m_elements.end(); }

private:
    std::vector<BufferElements> m_elements;
    uint32_t                    m_stride = 0;

private:
    void CalculateOffsetsAndStride()
    {
        uint32_t offset = 0;
        m_stride        = 0;
        for (auto& element : m_elements)
        {
            element.offset = offset;
            offset += element.size;
            m_stride += element.size;
        }
    }
};

class VertexBuffer
{
public:
    virtual ~VertexBuffer() {}

    virtual void Bind() const   = 0;
    virtual void Unbind() const = 0;


    virtual const BufferLayout& GetLayout()                           = 0;
    virtual void                SetLayout(const BufferLayout& layout) = 0;

    virtual void SetData(const void* data, uint32_t size) = 0;

    virtual uint32_t GetId() const = 0;

    static Ref<VertexBuffer> Create(uint32_t size);
    static Ref<VertexBuffer> Create(float* vertices, uint32_t size);
};

/**
 * @brief   Wrapper around an index buffer inside of the GPU's memory.
 *
 * @note    Brigerad currently only supports 32-bit index buffers.
 */
class IndexBuffer
{
public:
    virtual ~IndexBuffer() = default;

    virtual void Bind() const   = 0;
    virtual void Unbind() const = 0;

    virtual uint32_t       GetCount() const = 0;
    virtual uint32_t GetId() const    = 0;

    static Ref<IndexBuffer> Create(uint32_t* indices, uint32_t count);
};

}    // namespace Brigerad
