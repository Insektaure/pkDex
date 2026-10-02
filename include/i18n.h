#pragma once
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

// Translations: romfs:/i18n/<locale>/<file>.json, nested objects flattened to
// "<file>/<a>/<b>" keys (pkdex.json -> "pkdex/...", data.json -> "data/...").
// en-US is always loaded first, so a key a translation lacks falls back to it.
namespace i18n {

struct Locale { const char* code; const char* label; };
const std::vector<Locale>& locales();

void init(const std::string& locale);
const std::string& locale();

// The string, or the key itself when there is none.
const std::string& get(const std::string& key);
bool has(const std::string& key);

// get() with each "{name}" replaced by its value.
std::string fmt(const std::string& key, std::initializer_list<std::pair<const char*, std::string>> args);

// The console's language as one of locales(), en-US when it has none.
std::string systemLocale();

} // namespace i18n

// The UI strings live under "pkdex/".
inline const std::string& tr(const std::string& key) { return i18n::get("pkdex/" + key); }
inline std::string trf(const std::string& key, std::initializer_list<std::pair<const char*, std::string>> args) {
    return i18n::fmt("pkdex/" + key, args);
}
