/**
 ******************************************************************************
 * @addtogroup Config
 * @{
 * @file    Config
 * @author  Samuel Martel
 * @brief   Header for the Config module.
 *
 * @note https://github.com/nlohmann/json
 *
 * @date 1/4/2020 1:41:45 PM
 *
 ******************************************************************************
 */
#ifndef FRASY_UTILS_CONFIG
#    define FRASY_UTILS_CONFIG

/*****************************************************************************/
/* Includes */
#    include "../vendor/json.hpp"

#    include <string_view>

namespace Frasy
{
class Config
{
public:
    using config_t    = nlohmann::json;
    using exception_t = config_t::exception;

public:
    Config() noexcept                         = default;
    Config(const Config&) noexcept            = default;
    Config(Config&&) noexcept                 = default;
    Config& operator=(const Config&) noexcept = default;
    Config& operator=(Config&&) noexcept      = default;

    explicit Config(const std::string& path);

    Config(const std::string& key, const config_t& config) : m_path(key), m_config(config) {}

    static Config Load(const std::string& path);
    static void   Save(const std::string& path, const Config& cfg);
    void          Save() const { Save(m_path, *this); }

    template<class T>
    void SetField(const std::string& key, const T& val)
    {
        m_config[key] = val;
    }

    template<>
    void SetField(const std::string& key, const Config& val)
    {
        m_config[key] = val.m_config;
    }

    template<class T = Config>
    T GetField(const std::string& key, const T& def = {}) const
    {
        try
        {
            if constexpr (std::is_same_v<T, Config>) { return {key, m_config.at(key)}; }
            else { return static_cast<T>(m_config.at(key)); }
        }
        catch (exception_t&)
        {
            // Requested item doesn't exist.
            // throw std::invalid_argument("Field Not Found!");
            return def;
        }
    }

    template<typename T = int>
    T GetFieldFromObject(const config_t& obj, const std::string& field)
    {
        try
        {
            return obj[field];
        }
        catch (const exception_t&)
        {
            return T();
        }
    }

private:
    std::string m_path;
    config_t    m_config = {};

    static constexpr const char* s_tag = "Config";
};
}    // namespace Frasy

/* Have a wonderful day :) */
#endif /* FRASY_UTILS_CONFIG */
/**
 * @}
 */
/****** END OF FILE ******/
