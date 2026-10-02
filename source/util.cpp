#include "util.h"

#include <cerrno>
#include <cstdio>
#include <sys/stat.h>

namespace util {

bool fileExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

uint64_t fileSize(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return 0;
    return static_cast<uint64_t>(st.st_size);
}

bool ensureDir(const std::string& dir) {
    if (dir.empty()) return true;
    std::string path = dir;
    if (path.back() != '/') path += '/';
    // Past the device prefix ("sdmc:/"): there is nothing to create there.
    size_t at = path.find(":/");
    at = at == std::string::npos ? 1 : at + 2;
    for (size_t slash = path.find('/', at); slash != std::string::npos; slash = path.find('/', slash + 1)) {
        const std::string part = path.substr(0, slash);
        if (part.empty()) continue;
        if (mkdir(part.c_str(), 0777) != 0 && errno != EEXIST) return false;
    }
    return true;
}

bool ensureParentDir(const std::string& path) {
    const size_t end = path.find_last_of('/');
    if (end == std::string::npos) return true;
    return ensureDir(path.substr(0, end));
}

bool readFile(const std::string& path, std::string& out) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    out.clear();
    char buf[16 * 1024];
    for (;;) {
        const size_t got = fread(buf, 1, sizeof(buf), f);
        if (got == 0) break;
        out.append(buf, got);
    }
    fclose(f);
    return true;
}

std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n')) a++;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r' || s[b - 1] == '\n')) b--;
    return s.substr(a, b - a);
}

std::vector<std::string> split(const std::string& s, const std::string& sep) {
    std::vector<std::string> out;
    if (sep.empty()) { out.push_back(s); return out; }
    size_t start = 0;
    for (;;) {
        const size_t at = s.find(sep, start);
        if (at == std::string::npos) { out.push_back(s.substr(start)); break; }
        out.push_back(s.substr(start, at - start));
        start = at + sep.size();
    }
    return out;
}

std::string replaceAll(std::string s, const std::string& from, const std::string& to) {
    if (from.empty()) return s;
    size_t at = 0;
    while ((at = s.find(from, at)) != std::string::npos) {
        s.replace(at, from.size(), to);
        at += to.size();
    }
    return s;
}

int utf8Len(unsigned char lead) {
    if (lead < 0x80)           return 1;
    if ((lead & 0xE0) == 0xC0) return 2;
    if ((lead & 0xF0) == 0xE0) return 3;
    if ((lead & 0xF8) == 0xF0) return 4;
    return 1;
}

char32_t utf8Next(const std::string& s, size_t& i) {
    const unsigned char b0 = static_cast<unsigned char>(s[i]);
    const int len = utf8Len(b0);
    if (i + len > s.size()) { i = s.size(); return U'?'; }
    char32_t c;
    switch (len) {
        case 1: c = b0; break;
        case 2: c = ((b0 & 0x1F) << 6) | (s[i + 1] & 0x3F); break;
        case 3: c = ((b0 & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F); break;
        default:
            c = ((b0 & 0x07) << 18) | ((s[i + 1] & 0x3F) << 12) | ((s[i + 2] & 0x3F) << 6) | (s[i + 3] & 0x3F);
            break;
    }
    i += len;
    return c;
}

std::string toUpper(const std::string& in) {
    auto upper = [](char32_t c) -> char32_t {
        if (c >= U'a' && c <= U'z') return c - 0x20;
        if (c >= 0x00E0 && c <= 0x00FE && c != 0x00F7) return c - 0x20;   // à..þ, not ÷
        if (c == 0x00FF) return 0x0178;                                    // ÿ
        if ((c >= 0x0100 && c <= 0x0137) || (c >= 0x014A && c <= 0x0177))
            return (c & 1) ? c - 1 : c;
        if ((c >= 0x0139 && c <= 0x0148) || (c >= 0x0179 && c <= 0x017E))
            return (c & 1) ? c : c - 1;
        if (c >= 0x0430 && c <= 0x044F) return c - 0x20;                   // а..я
        if (c >= 0x0450 && c <= 0x045F) return c - 0x50;                   // ѐ..џ
        return c;
    };

    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size();) {
        const unsigned char b0 = static_cast<unsigned char>(in[i]);
        if (b0 < 0x80) {
            out += static_cast<char>(upper(b0));
            i += 1;
        } else if ((b0 & 0xE0) == 0xC0 && i + 1 < in.size()) {
            const char32_t c = ((b0 & 0x1F) << 6) | (static_cast<unsigned char>(in[i + 1]) & 0x3F);
            const char32_t u = upper(c);
            out += static_cast<char>(0xC0 | (u >> 6));
            out += static_cast<char>(0x80 | (u & 0x3F));
            i += 2;
        } else {
            const size_t len = utf8Len(b0);
            out += in.substr(i, len);
            i += len;
        }
    }
    return out;
}

std::string formatMB(uint64_t bytes) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
    return buf;
}

} // namespace util
