#include "SubTexture2D.h"


namespace Brigerad
{
SubTexture2D::SubTexture2D(const Ref<Texture2D>& texture,
                           const glm::vec2&      min,
                           const glm::vec2&      max)
: m_texture(texture)
{
    m_texCoords[0] = {min.x, min.y};
    m_texCoords[1] = {max.x, min.y};
    m_texCoords[2] = {max.x, max.y};
    m_texCoords[3] = {min.x, max.y};
}

Ref<SubTexture2D> SubTexture2D::CreateFromCoords(const Ref<Texture2D>& texture,
                                                 const glm::vec2&      pos,
                                                 const glm::vec2&      cellSize,
                                                 const glm::vec2&      spriteSize)
{
    glm::vec2 min = {
      (pos.x * cellSize.x) / texture->GetWidth(),
      (pos.y * cellSize.y) / texture->GetHeight(),
    };
    glm::vec2 max = {
      ((pos.x + spriteSize.x) * cellSize.x) / texture->GetWidth(),
      ((pos.y + spriteSize.y) * cellSize.y) / texture->GetHeight(),
    };

    return CreateRef<SubTexture2D>(texture, min, max);
}
}    // namespace Brigerad
