#pragma once

#include <string>

namespace pkdex {

// Replace a placeholder like "{region}" or "{count}" in a template string
inline std::string formatString(const std::string& tmpl, const std::string& placeholder, const std::string& value) {
    std::string result = tmpl;
    size_t pos = result.find(placeholder);
    if (pos != std::string::npos) {
        result.replace(pos, placeholder.length(), value);
    }
    return result;
}

} // namespace pkdex
