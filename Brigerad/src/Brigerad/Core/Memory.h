#pragma once

namespace Brigerad
{
namespace Memory
{
    //-----------------------------------------------------------------------------
    // Helpers: Memory allocations macros
    // BR_MALLOC(), BR_FREE(), BR_PLACEMENT_NEW(), BR_DELETE()
    // We call C++ constructor on own allocated memory
    // via the placement "new(ptr) Type()" syntax.
    // Defining a custom placement new() with a dummy parameter allows us to
    // bypass including <new> which on some platforms complains when user has
    // disabled exceptions.
    //-----------------------------------------------------------------------------

    void* MemAlloc(size_t size);
    void MemFree(void* ptr);


#define BR_ALLOC(_SIZE) Brigerad::Memory::MemAlloc(_SIZE)
#define BR_FREE(_PTR) Brigerad::Memory::MemFree(_PTR)
#define BR_PLACEMENT_NEW(_PTR) new(Brigerad::Memory::BrNewDummy(), _PTR)
#define BR_NEW(_TYPE) new(Brigerad::Memory::BrNewDummy(), \
                          Brigerad::Memory::MemAlloc(sizeof(_TYPE))) _TYPE
template<typename T> void BR_DELETE(T* p)
{
    if (p)
    {
        p->~T();
        Brigerad::Memory::MemFree(p);
    }
}

}
}

struct BrNewDummy
{
};

inline void* operator new(size_t, BrNewDummy, void* ptr) { return ptr; }
/**
 * This is only required so we can use the symmetrical new().
 */
inline void operator delete(void*, BrNewDummy, void*) {}
