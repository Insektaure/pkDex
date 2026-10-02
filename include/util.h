#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace util {

bool fileExists(const std::string& path);
uint64_t fileSize(const std::string& path);   // 0 when missing

// mkdir -p for `dir`, and for everything above `path`'s last slash.
bool ensureDir(const std::string& dir);
bool ensureParentDir(const std::string& path);

bool readFile(const std::string& path, std::string& out);

std::string trim(const std::string& s);
std::vector<std::string> split(const std::string& s, const std::string& sep);
std::string replaceAll(std::string s, const std::string& from, const std::string& to);

// Upper case for the small-caps labels: ASCII, accented Latin and Cyrillic.
// Anything else (CJK, digits, punctuation) is copied as it is.
std::string toUpper(const std::string& s);

// Byte length of the UTF-8 sequence starting with `lead`.
int utf8Len(unsigned char lead);
// Decodes the code point at `i` and advances `i` past it.
char32_t utf8Next(const std::string& s, size_t& i);

std::string formatMB(uint64_t bytes);   // "12.3 MB"

} // namespace util
