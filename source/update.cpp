// Self-update. See include/update.h for the shape of it; pkHouse's
// source/update.cpp is where the careful parts come from.

#include "update.h"
#include "config.h"
#include "job.h"
#include "json.h"
#include "net.h"
#include "util.h"

#include <switch.h>
#include <minizip/unzip.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <strings.h>
#include <vector>

#ifndef PKDEX_REPO
#define PKDEX_REPO "Insektaure/pkDex"
#endif

namespace {

const char* const API_URL = "https://api.github.com/repos/" PKDEX_REPO "/releases/latest";
const char* const RELEASE_PAGE = "https://github.com/" PKDEX_REPO "/releases/latest";

// Staged in the config folder rather than beside the running NRO: a half
// finished download has no business showing up in the homebrew menu.
std::string staged(const char* name) { return std::string(config::DIR) + "/" + name; }
// Where pkDex lives from 2.0.0 on, as the release zip lays it out. An install
// elsewhere (1.x kept /switch/pkDex.nro) is moved here by its next update.
const char* const APP_PATH   = "sdmc:/switch/pkDex/pkDex.nro";
const char* const LEGACY_APP = "sdmc:/switch/pkDex.nro";
const char* const MOVED_FROM = "update.moved_from";   // the copy a move left behind, to delete
const char* const ZIP_PART   = "update.zip";
const char* const ZIP_NRO    = "switch/pkdex/pkdex.nro";   // where the zip keeps the app (lower-cased)
const char* const NRO_PART   = "update.nro";
const char* const NRO_BACKUP = "update.nro.backup";

// A release is ~20 MB. These guard against a redirect to something enormous.
constexpr uint64_t MAX_DOWNLOAD = 128ull * 1024 * 1024;
constexpr size_t   MAX_API_BODY = 1024 * 1024;
constexpr uint64_t MAX_ENTRY    = 128ull * 1024 * 1024;

std::mutex    g_mutex;
Update::State g_state = Update::State::Idle;
std::string   g_version;
std::string   g_assetUrl;
std::string   g_error;
Worker        g_worker;

void setState(Update::State s, const std::string& error = std::string()) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_state = s;
    g_error = error;
}

bool endsWithNoCase(const std::string& s, const char* suffix) {
    const size_t n = strlen(suffix);
    return s.size() >= n && strcasecmp(s.c_str() + s.size() - n, suffix) == 0;
}

std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return s;
}

std::vector<int> versionParts(const std::string& raw) {
    std::vector<int> out;
    size_t i = 0;
    if (i < raw.size() && (raw[i] == 'v' || raw[i] == 'V')) i++;
    int value = 0;
    bool any = false;
    for (; i <= raw.size(); i++) {
        if (i < raw.size() && raw[i] >= '0' && raw[i] <= '9') {
            if (value < 100000) value = value * 10 + (raw[i] - '0');
            any = true;
            continue;
        }
        if (any) out.push_back(value);
        value = 0;
        any = false;
        // Anything but a dot ends the version: "1.2.3-beta4" is 1.2.3.
        if (i < raw.size() && raw[i] != '.') break;
    }
    return out;
}

// The latest release through the API: its tag and the pkDex.nro asset. False
// with `why` set when the API would not say (offline, rate limited...).
bool fromApi(std::string& tag, std::string& url, std::string& why) {
    static const char* const headers[] = {
        "Accept: application/vnd.github+json", "X-GitHub-Api-Version: 2022-11-28", nullptr};
    std::string body, error;
    long status = 0;
    if (!net::fetch(API_URL, headers, body, status, error, MAX_API_BODY)) { why = error; return false; }
    // 403/429: the API allows 60 unauthenticated requests an hour per public
    // address, which a shared network runs out of.
    if (status != 200) { why = "the GitHub API answered " + std::to_string(status); return false; }

    const json root = json::parse(body, nullptr, false);
    if (root.is_discarded() || !root.is_object()) { why = "unreadable answer from GitHub"; return false; }
    auto tagIt = root.find("tag_name");
    if (tagIt == root.end() || !tagIt->is_string()) { why = "no release tag"; return false; }
    tag = tagIt->get<std::string>();

    // The app: pkDex.zip from 2.0.0, a bare pkDex.nro before. Never the
    // high-res image pack (also a zip) nor the retired updater.
    url.clear();
    int best = 0;
    auto assets = root.find("assets");
    if (assets != root.end() && assets->is_array()) {
        for (const auto& a : *assets) {
            if (!a.is_object()) continue;
            auto n = a.find("name");
            auto u = a.find("browser_download_url");
            if (n == a.end() || u == a.end() || !n->is_string() || !u->is_string()) continue;
            const std::string name = lower(n->get<std::string>());
            int score = 0;
            if (name == "pkdex.zip") score = 3;
            else if (name == "pkdex.nro") score = 2;
            else if (endsWithNoCase(name, ".nro") && name.find("updater") == std::string::npos) score = 1;
            if (!score) continue;
            if (score > best) { best = score; url = u->get<std::string>(); }
        }
    }
    return true;
}

// The same from the web page, which the API's rate limit does not cover:
// /releases/latest redirects to /releases/tag/<tag>, and every release
// carries pkDex.zip under its tag. A release without one fails at download.
bool fromReleasePage(std::string& tag, std::string& url, std::string& why) {
    std::string location, error;
    long status = 0;
    if (!net::redirectTarget(RELEASE_PAGE, location, status, error)) { why = error; return false; }
    const std::string marker = "/releases/tag/";
    const size_t at = location.find(marker);
    if (at == std::string::npos) { why = "GitHub answered " + std::to_string(status) + " without a release"; return false; }
    tag = location.substr(at + marker.size());
    const size_t end = tag.find_first_of("?#/");
    if (end != std::string::npos) tag.erase(end);
    if (tag.empty()) { why = "no release tag"; return false; }
    url = std::string("https://github.com/" PKDEX_REPO "/releases/download/") + tag + "/pkDex.zip";
    return true;
}

void checkNow() {
    if (!net::online()) { setState(Update::State::Failed, "no network connection"); return; }

    std::string tag, url, apiWhy, pageWhy;
    if (!fromApi(tag, url, apiWhy) && !fromReleasePage(tag, url, pageWhy)) {
        setState(Update::State::Failed, apiWhy + "; " + pageWhy);
        return;
    }
    if (!tag.empty() && (tag[0] == 'v' || tag[0] == 'V')) tag.erase(0, 1);

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_version = tag;
        g_assetUrl = url;
    }
    if (Update::compareVersions(tag, APP_VERSION) <= 0 || url.empty())
        setState(Update::State::UpToDate);
    else
        setState(Update::State::Available);
}

// Only ever writes files that are ours and open nowhere else (the backup).
// The running NRO goes through overwriteFile() instead.
bool copyFile(const std::string& from, const std::string& to) {
    FILE* in = fopen(from.c_str(), "rb");
    if (!in) return false;
    remove(to.c_str());
    FILE* out = fopen(to.c_str(), "wb");
    if (!out) { fclose(in); return false; }
    std::vector<char> buf(64 * 1024);
    bool ok = true;
    for (;;) {
        const size_t got = fread(buf.data(), 1, buf.size(), in);
        if (got == 0) { ok = feof(in) != 0; break; }
        if (fwrite(buf.data(), 1, got, out) != got) { ok = false; break; }
    }
    if (fflush(out) != 0) ok = false;
    if (fclose(out) != 0) ok = false;
    fclose(in);
    if (!ok) remove(to.c_str());
    return ok;
}

// Writes `from` over `to` in place. fopen("wb") truncates on open, and this
// filesystem answers EIO to truncating the NRO that is running; creating the
// file (harmless when it is there), opening it for writing and setting the
// length explicitly never asks for that. Committed before returning, because
// this file has to survive the relaunch.
bool overwriteFile(const std::string& from, const std::string& to, std::string& why) {
    FILE* in = fopen(from.c_str(), "rb");
    if (!in) { why = "the new version could not be read"; return false; }
    fseek(in, 0, SEEK_END);
    const long total = ftell(in);
    rewind(in);
    if (total <= 0) { fclose(in); why = "the new version is empty"; return false; }

    FsFileSystem* sd = fsdevGetDeviceFileSystem("sdmc:");
    if (!sd) { fclose(in); why = "the SD card could not be opened"; return false; }
    std::string path = to;
    const size_t colon = path.find(':');
    if (colon != std::string::npos) path.erase(0, colon + 1);   // the native API wants "/switch/..."

    fsFsCreateFile(sd, path.c_str(), total, 0);   // fails when it exists, which is fine
    FsFile file{};
    Result rc = fsFsOpenFile(sd, path.c_str(), FsOpenMode_Write, &file);
    if (R_FAILED(rc)) { fclose(in); why = "the app could not be opened for writing"; return false; }
    // Shrinks as well as grows, so a smaller build leaves no tail behind.
    rc = fsFileSetSize(&file, total);
    if (R_FAILED(rc)) { fsFileClose(&file); fclose(in); why = "the app's size could not be set"; return false; }

    std::vector<char> buf(64 * 1024);
    s64 offset = 0;
    bool ok = true;
    while (offset < total) {
        const size_t got = fread(buf.data(), 1, buf.size(), in);
        if (got == 0) { ok = false; why = "the new version could not be read"; break; }
        rc = fsFileWrite(&file, offset, buf.data(), got, FsWriteOption_None);
        if (R_FAILED(rc)) { ok = false; why = "the app could not be written"; break; }
        offset += static_cast<s64>(got);
    }
    fsFileClose(&file);
    fclose(in);
    if (ok && R_FAILED(fsFsCommit(sd))) { ok = false; why = "the SD card did not commit the write"; }
    return ok;
}

// The NACP embedded in an NRO. False when the file is not an NRO.
bool readNacp(const std::string& path, NacpStruct& nacp) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    // The header sits after the 0x10-byte start stub; the asset section (icon,
    // NACP, RomFS) is appended after the code image.
    NroHeader header{};
    NroAssetHeader assets{};
    const bool ok =
        fseek(f, sizeof(NroStart), SEEK_SET) == 0 && fread(&header, 1, sizeof(header), f) == sizeof(header)
        && header.magic == NROHEADER_MAGIC
        && fseek(f, static_cast<long>(header.size), SEEK_SET) == 0
        && fread(&assets, 1, sizeof(assets), f) == sizeof(assets)
        && assets.magic == NROASSETHEADER_MAGIC && assets.nacp.size >= sizeof(NacpStruct)
        && fseek(f, static_cast<long>(header.size + assets.nacp.offset), SEEK_SET) == 0
        && fread(&nacp, 1, sizeof(nacp), f) == sizeof(nacp);
    fclose(f);
    return ok;
}

// The app's NRO out of the release zip, to `to`. Releases keep it at
// switch/pkDex/pkDex.nro; failing that, a pkDex.nro anywhere in the zip, then
// any NRO that is not the retired updater. Read to its
// end, so the CRC is checked, and stopped at MAX_ENTRY whatever the header says.
bool unpackNro(const std::string& zipPath, const std::string& to, std::string& why) {
    unzFile zip = unzOpen(zipPath.c_str());
    if (!zip) { why = "the download is not a readable zip"; return false; }

    int best = 0;
    unz_file_pos bestPos{};
    for (int step = unzGoToFirstFile(zip); step == UNZ_OK; step = unzGoToNextFile(zip)) {
        unz_file_info info{};
        char name[512] = {};
        if (unzGetCurrentFileInfo(zip, &info, name, sizeof(name) - 1, nullptr, 0, nullptr, 0) != UNZ_OK) continue;
        std::string n = lower(name);
        const size_t slash = n.find_last_of('/');
        const std::string base = slash == std::string::npos ? n : n.substr(slash + 1);
        int score = 0;
        if (n == ZIP_NRO) score = 3;
        else if (base == "pkdex.nro") score = 2;
        else if (endsWithNoCase(base, ".nro") && base.find("updater") == std::string::npos) score = 1;
        if (score > best) {
            best = score;
            unzGetFilePos(zip, &bestPos);
        }
    }
    if (!best || unzGoToFilePos(zip, &bestPos) != UNZ_OK) {
        unzClose(zip);
        why = "the zip holds no pkDex.nro";
        return false;
    }

    bool ok = unzOpenCurrentFile(zip) == UNZ_OK;
    FILE* out = ok ? fopen(to.c_str(), "wb") : nullptr;
    if (ok && !out) { unzCloseCurrentFile(zip); ok = false; }
    if (ok) {
        std::vector<char> buf(64 * 1024);
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
        if (unzCloseCurrentFile(zip) != UNZ_OK) ok = false;   // the CRC check
    }
    unzClose(zip);
    if (!ok) { remove(to.c_str()); why = "the build in the zip could not be unpacked"; }
    return ok;
}

} // anonymous namespace

namespace Update {

void beginCheck() {
    if (g_worker.busy()) return;
    if (!net::start()) { setState(State::Failed, "networking did not start"); return; }
    setState(State::Checking);
    if (!g_worker.start(checkNow, 256 * 1024))
        setState(State::Failed, "the check could not be started");
}

void shutdown() { g_worker.join(); }

State state() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_state;
}

std::string latestVersion() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_version;
}

std::string lastError() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_error;
}

std::string backupPath() { return staged(NRO_BACKUP); }

std::string assetName() {
    std::lock_guard<std::mutex> lock(g_mutex);
    const size_t slash = g_assetUrl.find_last_of('/');
    return slash == std::string::npos ? g_assetUrl : g_assetUrl.substr(slash + 1);
}

int compareVersions(const std::string& a, const std::string& b) {
    const std::vector<int> l = versionParts(a), r = versionParts(b);
    const size_t n = std::max(l.size(), r.size());
    for (size_t i = 0; i < n; i++) {
        const int x = i < l.size() ? l[i] : 0, y = i < r.size() ? r[i] : 0;
        if (x != y) return x < y ? -1 : 1;
    }
    return 0;
}

// A fixed-size NACP field that need not be terminated, as a string.
template <size_t N> std::string nacpField(const char (&field)[N]) {
    char out[N + 1] = {};
    memcpy(out, field, N);
    return std::string(out);
}

std::string nroDisplayVersion(const std::string& path) {
    NacpStruct nacp{};
    return readNacp(path, nacp) ? nacpField(nacp.display_version) : std::string();
}

std::string nroTitle(const std::string& path) {
    NacpStruct nacp{};
    if (!readNacp(path, nacp)) return std::string();
    // The NACP opens with 16 language entries of a 0x200-byte name and a
    // 0x100-byte author; the first name is the first 0x200 bytes. Read from
    // the layout rather than a member name, which libnx versions do not agree on.
    char name[0x200];
    memcpy(name, &nacp, sizeof(name));
    return nacpField(name);
}

std::string installPath() { return APP_PATH; }

bool installsInPlace(const std::string& exePath) { return strcasecmp(exePath.c_str(), APP_PATH) == 0; }

void install(const std::string& exePath, Job& job) {
    std::string url, wanted;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        url = g_assetUrl;
        wanted = g_version;
    }
    if (url.empty() || wanted.empty()) { job.finish(false, "there is no release to install"); return; }
    if (exePath.empty() || !endsWithNoCase(exePath, ".nro")) {
        job.finish(false, "pkDex does not know where its own NRO is, so it cannot replace it");
        return;
    }

    const bool zipped = endsWithNoCase(url, ".zip");
    const std::string zipPath = staged(ZIP_PART);
    const std::string part = staged(NRO_PART);
    const std::string backup = staged(NRO_BACKUP);
    util::ensureDir(config::DIR);
    remove(zipPath.c_str());
    remove(part.c_str());
    remove(backup.c_str());

    // 1. The build, to a staging file. Nothing in use is ever the target of a
    //    transfer that might stop half way.
    job.stage = StepDownload;
    std::string error;
    const bool got = net::download(url, zipped ? zipPath : part, [&job](uint64_t done, uint64_t total) {
        job.done = done;
        job.total = total;
        job.progress = total > 0 ? static_cast<float>(double(done) / double(total)) : -1.0f;
        return !job.cancel.load();
    }, error, MAX_DOWNLOAD);
    if (!got) { job.finish(false, "the download did not finish: " + error); return; }

    // 2. The NRO out of the zip. A release that ships the bare NRO (before
    //    2.0.0) has nothing to unpack.
    job.stage = StepUnpack;
    job.progress = -1.0f;
    if (zipped) {
        const bool unpacked = unpackNro(zipPath, part, error);
        remove(zipPath.c_str());
        if (!unpacked) { job.finish(false, error); return; }
    }

    // 3. Checked before anything else is touched: a wrong or broken download
    //    changes nothing at all.
    job.stage = StepVerify;
    job.progress = -1.0f;
    const std::string v = nroDisplayVersion(part);
    if (v.empty() || compareVersions(v, wanted) != 0) {
        remove(part.c_str());
        job.finish(false, v.empty() ? "the download is not a valid NRO"
                                    : "the download is version " + v + ", not " + wanted);
        return;
    }

    // 4-6, moving an old install. The running copy is left alone - it is the
    //      backup - and the new one goes to APP_PATH through a temporary file,
    //      read back before it takes the name. The copy left behind is deleted
    //      by the new build at its first start (cleanupLegacy), not here: it is
    //      the one running.
    if (!installsInPlace(exePath)) {
        job.stage = StepBackup;
        const std::string tmp = std::string(APP_PATH) + ".new";
        job.stage = StepInstall;
        if (!util::ensureParentDir(APP_PATH) || !copyFile(part, tmp) || compareVersions(nroDisplayVersion(tmp), wanted) != 0) {
            remove(tmp.c_str());
            remove(part.c_str());
            job.finish(false, std::string("the update could not be written to ") + APP_PATH);
            return;
        }
        // FAT's rename does not replace, so a stale copy there goes first.
        remove(APP_PATH);
        if (rename(tmp.c_str(), APP_PATH) != 0 || compareVersions(nroDisplayVersion(APP_PATH), wanted) != 0) {
            remove(tmp.c_str());
            remove(part.c_str());
            job.finish(false, std::string("the update could not be written to ") + APP_PATH);
            return;
        }
        if (FILE* f = fopen(staged(MOVED_FROM).c_str(), "wb")) {
            fputs(exePath.c_str(), f);
            fclose(f);
        }
        job.stage = StepCleanup;
        remove(part.c_str());
        job.stage = StepCount;
        job.finish(true);
        return;
    }

    // 4-6. The running build is copied aside, then replaced. RomFS is mounted
    //      out of this very file and would lock it against the write (0xE02),
    //      so it is released for the duration and put back however this ends.
    struct RomfsRelease {
        RomfsRelease() { romfsExit(); }
        ~RomfsRelease() { romfsInit(); }
    } releaseRomfs;

    job.stage = StepBackup;
    if (!copyFile(exePath, backup)) {
        remove(part.c_str());
        job.finish(false, "the current version could not be backed up");
        return;
    }

    job.stage = StepInstall;
    auto restore = [&]() {
        std::string why;
        if (overwriteFile(backup, exePath, why)) remove(backup.c_str());
        remove(part.c_str());
    };
    std::string why;
    if (!overwriteFile(part, exePath, why)) {
        restore();
        job.finish(false, "the update could not be written over the current version (" + why + ")");
        return;
    }
    // Read back off the card: a write that went wrong is still recoverable
    // here, and not one step later.
    if (compareVersions(nroDisplayVersion(exePath), wanted) != 0) {
        restore();
        job.finish(false, "the installed update did not verify, so it was rolled back");
        return;
    }

    job.stage = StepCleanup;
    remove(backup.c_str());
    remove(part.c_str());
    job.stage = StepCount;
    job.finish(true);
}

void restartInto(const std::string& exePath) {
    envSetNextLoad(exePath.c_str(), exePath.c_str());
}

void cleanupLegacy(const std::string& exePath) {
    remove("sdmc:/switch/pkDexUpdater.nro");
    remove("sdmc:/switch/pkDex.nro.new");
    if (!installsInPlace(exePath)) return;   // still the old copy: it stays

    // The copy a move left behind, and the 1.x one a manual install of the zip
    // leaves next to the new one - each only when it really is pkDex, never
    // the file running now.
    std::string moved;
    const std::string marker = staged(MOVED_FROM);
    util::readFile(marker, moved);
    moved = util::trim(moved);
    for (const std::string& old : {moved, std::string(LEGACY_APP)}) {
        if (old.empty() || strcasecmp(old.c_str(), exePath.c_str()) == 0) continue;
        if (nroTitle(old) == "pkDex") remove(old.c_str());
    }
    remove(marker.c_str());
}

} // namespace Update
