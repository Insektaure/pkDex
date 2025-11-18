#pragma once

#include <string>

namespace pkdex {

class I18n {
public:
    // Load default (en-US) then the given locale ("auto" or empty = system)
    static void load(const std::string& locale);

    // Returns the translation for the given key, or the key itself if not found
    static std::string getStr(const std::string& key);

    // Returns raw, or key if not found
    static std::string getRawStr(const std::string& key);
};

} // namespace pkdex
