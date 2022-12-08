/**
 * @file   D:\dev\Brigerad\Brigerad\src\Brigerad\Core\Memory.cpp
 * @author Samuel Martel
 * @date   2020/06/07
 *
 * @brief  Source for the Memory module.
 */
#include "Memory.h"

#include <cstdlib>

namespace Brigerad
{
namespace Memory
{
void* MemAlloc(size_t size)
{
    return malloc(size);
}
void MemFree(void* ptr)
{
    if (ptr)
    {
        free(ptr);
    }
}
}
}
