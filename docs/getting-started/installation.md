# Installation

This page covers the prerequisites and initial setup for building a Frasy application from the template.

## Prerequisites

### Required

| Tool | Minimum Version | Notes |
|---|---|---|
| Windows | 10 or later | Frasy is Windows-only |
| [CMake](https://cmake.org/download/) | 3.22 | Build system |
| [Git](https://git-scm.com/) | Any recent | Required for submodule checkout |
| C++ compiler | C++23 support | MSVC (Visual Studio 2022) recommended |

!!! note "Compiler support"
    MSVC (Visual Studio 2022) is the primary supported compiler. GCC and Clang are partially supported
    but not the primary target. The project uses C++23 features throughout.

### Optional

| Tool | Purpose |
|---|---|
| [Visual Studio 2022](https://visualstudio.microsoft.com/) | IDE — also provides MSVC |
| [CLion](https://www.jetbrains.com/clion/) | Alternative IDE with CMake support |
| [Inno Setup](https://jrsoftware.org/isinfo.php) | Building a Windows installer for distribution |

---

## Initializing the Repository

Frasy is included as a Git submodule under `vendor/frasy`. 

Add it to your repository with `git submodule add https://github.com/smartel99/Frasy vendor/frasy`

---

## Pre-Build Steps

Two scripts must be run before the first build, and again whenever Lua scripts are added or modified.

**1. Generate script hashes**

Frasy verifies the integrity of all Lua scripts at runtime using SHA hashes. This script scans the
`src/lua/user/` and `lua/core/` directories and writes a `hash` file next to each:

```bat
.\vendor\frasy\scripts\Windows\generate_hashes.bat
```

**2. Refresh dependencies**

This script copies the Frasy Lua core SDK and assets into the build output directory so the
application can find them at runtime:

```bat
.\vendor\frasy\scripts\Windows\refresh_dependencies.bat <build-output-dir>
```

!!! tip
    When using CMake, these steps are automated as build targets (`generate_hashes` and
    `refresh_lua_core`). You only need to run them manually when working outside of a CMake build,
    or to pre-populate the output directory before the first build.

---

## Next Steps

With prerequisites installed and the repository cloned, proceed to [Building](building.md).
