#include "i18n.hpp"

#include <borealis/core/application.hpp>
#include <borealis/core/assets.hpp>
#include <borealis/core/logger.hpp>

#ifdef USE_BOOST_FILESYSTEM
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#elif __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include("experimental/filesystem")
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#error "Failed to include <filesystem> header!"
#endif

#include <fstream>
#include <nlohmann/json.hpp>

namespace pkdex {

static nlohmann::json sDefault = {};
static nlohmann::json sCurrent = {};

static void loadLocaleDir(const std::string& locale, nlohmann::json* target)
{
    if (locale.empty()) return;
#ifdef USE_LIBROMFS
    auto localePath = brls::romfs::list("i18n/" + locale);
    if (localePath.empty())
    {
        brls::Logger::error("[pkdex::I18n] Cannot load locale {}: directory i18n/{} doesn't exist", locale, locale);
        return;
    }
    for (auto& entry : localePath)
    {
        std::string path = entry.string();
        std::string name = entry.filename().string();
        if (!brls::endsWith(name, ".json"))
            continue;
        (*target)[name.substr(0, name.length() - 5)] = nlohmann::json::parse(brls::romfs::get(path).string());
    }
#else
    std::string localePath = BRLS_ASSET("i18n/" + locale);

    if (!fs::exists(localePath))
    {
        brls::Logger::error("[pkdex::I18n] Cannot load locale {}: directory {} doesn't exist", locale, localePath);
        return;
    }
    else if (!fs::is_directory(localePath))
    {
        brls::Logger::error("[pkdex::I18n] Cannot load locale {}: {} isn't a directory", locale, localePath);
        return;
    }

    for (const fs::directory_entry& entry : fs::directory_iterator(localePath))
    {
        if (fs::is_directory(entry))
            continue;
        std::string name = entry.path().filename().string();
        if (!brls::endsWith(name, ".json"))
            continue;
        std::string path = entry.path().string();
        nlohmann::json strings;
        std::ifstream jsonStream(path);
        try
        {
            jsonStream >> strings;
        }
        catch (const std::exception& e)
        {
            brls::Logger::error("[pkdex::I18n] Error while loading \"{}\": {}", path, e.what());
        }
        (*target)[name.substr(0, name.length() - 5)] = strings;
    }
#endif
}

void I18n::load(const std::string& locale)
{
    sDefault = {};
    sCurrent = {};

    // Always load default first
    loadLocaleDir("en-US", &sDefault);

    std::string chosen = locale;
    if (chosen.empty() || chosen == "auto")
        chosen = brls::Application::getLocale();

    if (chosen != "en-US")
        loadLocaleDir(chosen, &sCurrent);
}

std::string I18n::getRawStr(const std::string& key)
{
    nlohmann::json::json_pointer pointer;
    try
    {
        pointer = nlohmann::json::json_pointer("/" + key);
    }
    catch (const std::exception& e)
    {
        brls::Logger::error("[pkdex::I18n] Error while getting string \"{}\": {}", key, e.what());
        return key;
    }

    try { return sCurrent[pointer].get<std::string>(); } catch (...) {}
    try { return sDefault[pointer].get<std::string>(); } catch (...) {}
    return key;
}

std::string I18n::getStr(const std::string& key)
{
    return getRawStr(key);
}

} // namespace pkdex
