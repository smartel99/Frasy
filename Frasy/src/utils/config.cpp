#include "config.h"

#include <Brigerad.h>
#include <Brigerad/Core/Log.h>
#include <fstream>

namespace Frasy {
Config::Config(const std::string& path) : m_path(path)
{
    BR_PROFILE_FUNCTION();
    if (!std::filesystem::exists(m_path)) { return; }

    std::ifstream ifs(m_path);
    try {
        ifs >> m_config;
    }
    catch (const std::exception& e) {
        BR_APP_ERROR("Failed to load configuration. Reason: {}", e.what());
    }
    ifs.close();
}

Config Config::load(const std::string& path)
{
    return Config(path);
}

void Config::save(const std::string& path, const Config& cfg)
{
    BR_PROFILE_FUNCTION();
    if (std::filesystem::exists(path)) { std::filesystem::remove(path); }
    std::ofstream file(path, std::ios::out);
    file << std::setw(4) << cfg.m_config << std::endl;
    file.close();
}
}    // namespace Frasy
