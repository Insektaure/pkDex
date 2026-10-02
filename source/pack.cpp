#include "pack.h"
#include "job.h"
#include "net.h"
#include "util.h"

#include <minizip/unzip.h>

#include <cstdio>
#include <string>
#include <vector>

#ifndef PKDEX_REPO
#define PKDEX_REPO "Insektaure/pkDex"
#endif

namespace {

const char* const URL = "https://github.com/" PKDEX_REPO "/releases/latest/download/pkDex_High_Res_imgs.zip";
const char* const ROOT = "sdmc:/";

constexpr uint64_t MAX_DOWNLOAD = 512ull * 1024 * 1024;
constexpr uint64_t MAX_ENTRY    = 64ull * 1024 * 1024;

// An entry name that can only land inside the SD card's root: relative, no
// drive, no backslashes, no "." or ".." anywhere in it.
bool safeRelative(const std::string& rel) {
    if (rel.empty() || rel[0] == '/') return false;
    if (rel.find('\\') != std::string::npos || rel.find(':') != std::string::npos) return false;
    for (const std::string& part : util::split(rel, "/"))
        if (part == "." || part == "..") return false;
    return true;
}

// The zip's current entry to `to`, stopping at MAX_ENTRY whatever the header
// claims: a zip bomb is stopped by what comes out, not by what it says.
bool extractCurrent(unzFile zip, const std::string& to) {
    if (unzOpenCurrentFile(zip) != UNZ_OK) return false;
    FILE* out = fopen(to.c_str(), "wb");
    if (!out) { unzCloseCurrentFile(zip); return false; }
    std::vector<char> buf(64 * 1024);
    bool ok = true;
    uint64_t written = 0;
    for (;;) {
        const int got = unzReadCurrentFile(zip, buf.data(), static_cast<unsigned>(buf.size()));
        if (got < 0) { ok = false; break; }
        if (got == 0) break;
        written += static_cast<uint64_t>(got);
        if (written > MAX_ENTRY || fwrite(buf.data(), 1, static_cast<size_t>(got), out) != static_cast<size_t>(got)) {
            ok = false;
            break;
        }
    }
    if (fclose(out) != 0) ok = false;
    // The CRC is only checked when the entry was read to its end.
    if (unzCloseCurrentFile(zip) != UNZ_OK) ok = false;
    if (!ok) remove(to.c_str());
    return ok;
}

} // anonymous namespace

namespace pack {

bool zipExists() { return util::fileExists(ZIP); }

void download(Job& job) {
    if (!net::online()) { job.finish(false, "no network connection"); return; }
    std::string error;
    const bool ok = net::download(URL, ZIP, [&job](uint64_t done, uint64_t total) {
        job.done = done;
        job.total = total;
        job.progress = total > 0 ? static_cast<float>(double(done) / double(total)) : -1.0f;
        return !job.cancel.load();
    }, error, MAX_DOWNLOAD);
    job.finish(ok, error);
}

void extract(Job& job) {
    unzFile zip = unzOpen(ZIP);
    if (!zip) { job.finish(false, "the image pack is not a readable zip"); return; }

    unz_global_info info{};
    if (unzGetGlobalInfo(zip, &info) != UNZ_OK) {
        unzClose(zip);
        job.finish(false, "the image pack could not be read");
        return;
    }
    const uint64_t total = info.number_entry;
    job.total = total;

    uint64_t i = 0;
    for (int step = unzGoToFirstFile(zip); step == UNZ_OK; step = unzGoToNextFile(zip), i++) {
        job.done = i;
        job.progress = total ? static_cast<float>(double(i) / double(total)) : -1.0f;

        unz_file_info file{};
        char name[512] = {};
        if (unzGetCurrentFileInfo(zip, &file, name, sizeof(name) - 1, nullptr, 0, nullptr, 0) != UNZ_OK) {
            unzClose(zip);
            job.finish(false, "the image pack could not be read");
            return;
        }
        std::string rel(name);
        while (!rel.empty() && rel[0] == '/') rel.erase(0, 1);
        const bool dir = !rel.empty() && rel.back() == '/';
        if (dir) rel.pop_back();
        if (!safeRelative(rel)) continue;   // not ours to write

        const std::string target = std::string(ROOT) + rel;
        if (dir) { util::ensureDir(target); continue; }
        if (!util::ensureParentDir(target) || !extractCurrent(zip, target)) {
            unzClose(zip);
            job.finish(false, "could not write " + rel);
            return;
        }
    }
    unzClose(zip);
    job.done = total;
    job.progress = 1.0f;
    job.finish(true);
}

} // namespace pack
