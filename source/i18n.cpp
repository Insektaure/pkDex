#include "i18n.h"
#include "json.h"
#include "util.h"

#include <switch.h>
#include <unordered_map>

namespace {

std::unordered_map<std::string, std::string> g_strings;
std::string g_locale = "en-US";

void flatten(const json& j, const std::string& prefix) {
    if (j.is_object()) {
        for (auto it = j.begin(); it != j.end(); ++it)
            flatten(it.value(), prefix + "/" + it.key());
    } else if (j.is_string()) {
        g_strings[prefix] = j.get<std::string>();
    }
}

void loadFile(const std::string& path, const std::string& prefix) {
    std::string text;
    if (!util::readFile(path, text)) return;
    json doc = json::parse(text, nullptr, false, true);
    if (doc.is_discarded()) {
        // A stray comma after the last brace is the usual hand-edit slip.
        std::string t = util::trim(text);
        while (!t.empty() && (t.back() == ',' || t.back() == ';')) t.pop_back();
        doc = json::parse(t, nullptr, false, true);
    }
    if (doc.is_discarded() || !doc.is_object()) return;
    flatten(doc, prefix);
}

void loadLocale(const std::string& code) {
    const std::string dir = "romfs:/i18n/" + code + "/";
    loadFile(dir + "pkdex.json", "pkdex");
    loadFile(dir + "data.json", "data");
}

} // anonymous namespace

namespace i18n {

const std::vector<Locale>& locales() {
    static const std::vector<Locale> list = {
        {"en-US", "English (US)"},
        {"fr-FR", "Français"},
        {"de-DE", "Deutsch"},
        {"es-ES", "Español"},
        {"it-IT", "Italiano"},
        {"ja-JP", "日本語"},
    };
    return list;
}

void init(const std::string& locale) {
    g_strings.clear();
    g_locale = "en-US";
    for (const auto& l : locales())
        if (locale == l.code) g_locale = l.code;
    loadLocale("en-US");
    if (g_locale != "en-US") loadLocale(g_locale);
}

const std::string& locale() { return g_locale; }

const std::string& get(const std::string& key) {
    auto it = g_strings.find(key);
    if (it != g_strings.end()) return it->second;
    // Remembered, so the reference stays valid and the miss is only paid once.
    return g_strings.emplace(key, key).first->second;
}

bool has(const std::string& key) {
    auto it = g_strings.find(key);
    return it != g_strings.end() && it->second != key;
}

std::string fmt(const std::string& key, std::initializer_list<std::pair<const char*, std::string>> args) {
    std::string s = get(key);
    for (const auto& [name, value] : args)
        s = util::replaceAll(s, std::string("{") + name + "}", value);
    return s;
}

std::string systemLocale() {
    std::string code = "en-US";
    if (R_FAILED(setInitialize())) return code;
    u64 lang = 0;
    SetLanguage sys = SetLanguage_ENUS;
    if (R_SUCCEEDED(setGetSystemLanguage(&lang)) && R_SUCCEEDED(setMakeLanguage(lang, &sys))) {
        switch (sys) {
            case SetLanguage_FR: case SetLanguage_FRCA:  code = "fr-FR"; break;
            case SetLanguage_DE:                         code = "de-DE"; break;
            case SetLanguage_ES: case SetLanguage_ES419: code = "es-ES"; break;
            case SetLanguage_IT:                         code = "it-IT"; break;
            case SetLanguage_JA:                         code = "ja-JP"; break;
            default: break;
        }
    }
    setExit();
    return code;
}

} // namespace i18n
