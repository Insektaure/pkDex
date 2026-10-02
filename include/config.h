#pragma once
#include <string>

// sdmc:/config/pkDex/config.ini, "key=value" lines - the same file and keys as
// the pkDex releases before 2.0, so settings carry over. Read once, written
// whole on every change.
namespace config {

constexpr const char* DIR  = "sdmc:/config/pkDex";
constexpr const char* PATH = "sdmc:/config/pkDex/config.ini";

constexpr const char* CHECK_ON_LAUNCH = "toggle_check_version_on_launch";
constexpr const char* HIDE_FOOTER     = "toggle_hide_bottom_bar";
constexpr const char* LOCALE          = "i18n_locale";
constexpr const char* RESET_REGION    = "selected_region_index";   // 0 = all, then regions in order

void load();

// Bumped by every change, so a drawing of the settings can tell it is stale.
unsigned version();

bool getBool(const std::string& key, bool def);
void setBool(const std::string& key, bool value);
int getInt(const std::string& key, int def);
void setInt(const std::string& key, int value);
std::string getString(const std::string& key, const std::string& def);
void setString(const std::string& key, const std::string& value);

} // namespace config
