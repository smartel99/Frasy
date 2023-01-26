#include "config.h"

#include <Brigerad.h>
#include <Brigerad/Core/Log.h>

namespace Frasy
{
Config::Config(const std::string& path) : m_path(path)
{
    BR_PROFILE_FUNCTION();
    // Open the file or create it if it doesn't exist.
    std::fstream file(m_path, std::ios::out | std::ios::app);
    file.close();
    std::ifstream j(m_path);

    std::string fullFile;
    std::string line;

    while (std::getline(j, line)) { fullFile += line; }

    try
    {
        m_config = config_t::parse(fullFile);
    }
    catch (config_t::parse_error& e)
    {
        BR_APP_ERROR("An error occurred while parsing the internal config: {}", e.what());
        m_config = "{}"_json;
        Save(m_path, *this);
    }

    j.close();
}

Config Config::Load(const std::string& path)
{
    return Config(path);
}

void Config::Save(const std::string& path, const Config& cfg)
{
    BR_PROFILE_FUNCTION();
    std::fstream file(path, std::ios::out);
    std::string  j = cfg.m_config.dump(-1, ' ', true, config_t::error_handler_t::replace);

    if (!file.is_open())
    {
        BR_LOG_ERROR(s_tag, "Unable to open file '{}'!", path);
        return;
    }

    // file << std::setw(4) << DO_NOT_USE::config;
    file << j;

    file.close();
}
}    // namespace Frasy
