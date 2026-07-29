# Building

## CMakeLists.txt Setup

Your application's `CMakeLists.txt` must do the following to integrate Frasy:

**1. Include Frasy's compile options**

This sets up C++23, compiler flags, and build type configurations:

```cmake
include(vendor/frasy/compile_options.cmake)
```

**2. Add the Frasy submodule**

This builds the `Frasy` static library target and makes `frasy_build_options` available:

```cmake
add_subdirectory(vendor/frasy)
```

**3. Link against Frasy**

```cmake
target_link_libraries(${PROJECT_NAME} PRIVATE
        frasy_build_options
        Frasy
)
```

**4. Sync runtime assets**

Frasy provides a `target_sync_assets()` helper that mirrors a directory into the build output on
every build. You need it to keep your Lua products and `config.json` in sync:

```cmake
include(vendor/frasy/sync_assets.cmake)

# Copy config.json to the output directory
add_custom_target(refresh_config ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_newer
        "${SRC_DIR}/config.json"
        "$<TARGET_FILE_DIR:${APP_NAME}>/config.json"
)

# Mirror src/lua/user/ into the output directory
target_sync_assets(refresh_lua_user
        "${SRC_DIR}/lua/user"
        "$<TARGET_FILE_DIR:${APP_NAME}>/lua/user"
)

# refresh_lua_core and refresh_config must complete before refresh_lua_user
add_dependencies(refresh_lua_user
        refresh_lua_core
        refresh_config
)
add_dependencies(${PROJECT_NAME} refresh_lua_user)
```

!!! note
    `refresh_lua_core` is defined by `add_subdirectory(vendor/frasy)` — it syncs the Frasy Lua
    core SDK into the output directory automatically.

A minimal but complete `CMakeLists.txt` looks like this:

```cmake
cmake_minimum_required(VERSION 3.22)

set(APP_NAME "MyApp")
project(${APP_NAME} LANGUAGES CXX VERSION 1.0.0)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${APP_NAME}_v${CMAKE_PROJECT_VERSION}")

include(vendor/frasy/compile_options.cmake)

file(GLOB_RECURSE SOURCES
        "${SRC_DIR}/my_frasy_interpreter.cpp"
        "${SRC_DIR}/layers/*.cpp"
)

add_subdirectory(vendor/frasy)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE frasy_build_options Frasy)

add_custom_target(refresh_config ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_newer
        "${SRC_DIR}/config.json"
        "$<TARGET_FILE_DIR:${APP_NAME}>/config.json"
)

include(vendor/frasy/sync_assets.cmake)
target_sync_assets(refresh_lua_user
        "${SRC_DIR}/lua/user"
        "$<TARGET_FILE_DIR:${APP_NAME}>/lua/user"
)
add_dependencies(refresh_lua_user refresh_lua_core refresh_config)
add_dependencies(${PROJECT_NAME} refresh_lua_user)
```

---

## Configure with CMake

From the root of your application repository, configure the project with CMake. Choose a build
type based on your intent:

| Build Type | Use case |
|---|---|
| `Debug` | Local development — no optimization, assertions enabled |
| `Release` | Optimized build, no debug info |
| `RelWithDebInfo` | **Recommended for production** — optimized with debug info for crash stacktraces |
| `MinSizeRel` | Size-optimized build |

```bat
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

!!! warning "Use RelWithDebInfo for production"
    The application must be built with `RelWithDebInfo` to properly recover stacktraces from crash
    dumps. A `Release` build will produce unusable crash reports.

---

## Build

```bat
cmake --build build
```

The output is placed under:

```
build/bin/<APP_NAME>_v<VERSION>/
```

For example, with the default settings (`APP_NAME=FrasyApp`, `VERSION=0.0.1`):

```
build/bin/FrasyApp_v0.0.1/
```

This directory contains the executable, assets, Lua core SDK, and your product's Lua files — everything needed to run the application.

---

## What the Build Does Automatically

The CMake build handles several steps automatically on every build:

- **`generate_hashes`** — runs `generate_hashes.bat` to hash all Lua scripts for runtime integrity verification
- **`refresh_lua_core`** — syncs the Frasy Lua core SDK into the output directory
- **`refresh_frasy_assets`** — syncs application assets (icons, textures) into the output directory
- **`refresh_lua_user`** — syncs your `src/lua/user/` products into the output directory
- **`refresh_config`** — copies `src/config.json` into the output directory

You do not need to run any of these manually when building through CMake.

---

## Customizing the Build

### Changing the application name and version

Edit the top of `CMakeLists.txt`:

```cmake
set(APP_NAME "MyApp")
project(${APP_NAME}
        LANGUAGES CXX
        VERSION 1.0.0
)
```

### Adding source files

C++ files under `src/layers/` are picked up automatically. To add files from other directories,
extend the `GLOB_RECURSE` call in `CMakeLists.txt`:

```cmake
file(GLOB_RECURSE SOURCES
        "${SRC_DIR}/my_frasy_interpreter.cpp"
        "${SRC_DIR}/layers/*.cpp"
        "${SRC_DIR}/my_other_dir/*.cpp"
)
```

---

## IDE Setup

### Visual Studio 2022

Open Visual Studio 2022 and use **File → Open → CMake...** to open the root `CMakeLists.txt`
directly. Visual Studio will configure the project automatically.

### CLion

Open the root directory in CLion. It detects `CMakeLists.txt` automatically and configures the
project on first open.

---

## Next Steps

With a successful build, proceed to [C++ Application Setup](cpp-setup.md).
