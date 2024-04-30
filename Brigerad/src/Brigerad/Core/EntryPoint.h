#pragma once

#include "Log.h"

#include <filesystem>
#include <string>

#if defined(BR_PLATFORM_WINDOWS) || defined(BR_PLATFORM_LINUX)

extern Brigerad::Application* Brigerad::CreateApplication();

int main(int argc, char** argv)
{
    BR_PROFILE_BEGIN_SESSION("Init", "BrigeradProfile-Startup.json");
    Brigerad::Log::Init();

    std::string path = std::filesystem::current_path().string();
//    BR_CORE_WARN("Running from: {0}", path);

    auto app = Brigerad::CreateApplication();
    BR_PROFILE_END_SESSION();

    app->run();

    BR_PROFILE_BEGIN_SESSION("Shutdown", "BrigeradProfile-Shutdown.json");
    delete app;
    BR_PROFILE_END_SESSION();
}

#endif
