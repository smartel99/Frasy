#pragma once

#include <memory>

#if !defined(BR_PLATFORM_WINDOWS) && !defined(BR_PLATFORM_LINUX)
#    if defined(__WIN32) || defined(WIN32) || defined(__WIN32__)
#        undef BR_PLATFORM_LINUX
#        define BR_PLATFORM_WINDOWS
#    endif
#    if defined(__LINUX__) || defined(__unix) || defined(__unix__)
#        undef BR_PLATFORM_WINDOWS
#        define BR_PLATFORM_LINUX
#    endif
#endif

#if defined(BR_PLATFORM_WINDOWS)
#    if defined(BR_DYNAMIC_LINK)
#        if defined(BR_BUILD_DLL)
#            define BRIGERAD_API __declspec(dllexport)
#        else
#            define BRIGERAD_API __declspec(dllimport)
#        endif    // BR_BUILD_DLL
#    else
#        define BRIGERAD_API
#    endif    // BR_DYNAMIC_LINK
#elif defined(BR_PLATFORM_LINUX)
#    define BRIGERAD_API
#else
#    error Brigerad only support Windows and Linux
#endif    // BR_PLATFROM_WINDOWS

// This should check the compiler used, not the OS.
#if defined(BR_PLATFORM_WINDOWS)
#    define DEBUG_BREAK(...) __debugbreak()
#elif defined(BR_PLATFORM_LINUX)
#    include <signal.h>
#    define DEBUG_BREAK(...) raise(SIGTRAP)
#else
#    error Brigerad only support Windows and Linux
#endif

// Define the function signature macro for the compiler being used.
#if defined(_MSC_VER)
// Visual Studio
#    define FUNCSIG __FUNCSIG__
#elif defined(__GNUC__)
// GCC
#    define FUNCSIG __PRETTY_FUNCTION__
#elif defined(__clang__)
// clang
#    define FUNCSIG __PRETTY_FUNCTION__
#elif defined(__MINGW32__)
// MinGW 32/MinGW-w64 32 bits
#    define FUNCSIG __PRETTY_FUNCTION__
#elif defined(__MINGW64__)
// MinGW-w64 64 bits
#    define FUNCSIG __PRETTY_FUNCTION__
#else
#    error Unsupported Compiler
#endif

#ifdef BR_ENABLE_ASSERTS
#    include "Log.h"
#    define BR_ASSERT(x, msg, ...)                                                                 \
        do {                                                                                       \
            if (!(x))                                                                              \
            {                                                                                      \
                BR_APP_ERROR("Assertion Failed: {}", fmt::format(msg __VA_OPT__(, ) __VA_ARGS__)); \
                DEBUG_BREAK();                                                                     \
            }                                                                                      \
        } while (0)
#    define BR_CORE_ASSERT(x, msg, ...)                                                             \
        do {                                                                                        \
            if (!(x))                                                                               \
            {                                                                                       \
                BR_CORE_ERROR("Assertion Failed: {}", fmt::format(msg __VA_OPT__(, ) __VA_ARGS__)); \
                DEBUG_BREAK();                                                                      \
            }                                                                                       \
        } while (0)
#else
#    define BR_ASSERT(x, msg, ...)
#    define BR_CORE_ASSERT(x, msg, ...)
#endif

#define BIT(x) (1 << (x))

#define BR_BIND_EVENT_FN(fn) \
    [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Brigerad
{
template<typename T>
using Scope = std::unique_ptr<T>;
template<typename T, typename... Args>
constexpr Scope<T> CreateScope(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T>
using Ref = std::shared_ptr<T>;
template<typename T, typename... Args>
constexpr Ref<T> CreateRef(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}
}    // namespace Brigerad
