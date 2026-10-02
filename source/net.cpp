#include "net.h"
#include "config.h"
#include "util.h"

#include <switch.h>
#include <curl/curl.h>

#include <cstdio>

namespace {

bool g_socketsUp = false;
bool g_curlUp = false;
std::string g_caBundle;

void commonOptions(CURL* curl, const std::string& url, long timeoutMs) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "pkDex/" APP_VERSION);   // GitHub requires one
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);   // release files are redirects
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    // HTTPS only, redirects included. The string form replaced the bitmask in
    // curl 7.85; devkitPro's curl predates it.
#if LIBCURL_VERSION_NUM >= 0x075500
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https");
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "https");
#else
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTPS);
#endif
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 20000L);
    if (timeoutMs > 0) curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutMs);
    // Verification stays on: what comes down here can replace the app.
    if (!g_caBundle.empty()) curl_easy_setopt(curl, CURLOPT_CAINFO, g_caBundle.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
}

struct Body { std::string* out; size_t max; };

size_t appendBody(char* data, size_t size, size_t nmemb, void* userdata) {
    auto* b = static_cast<Body*>(userdata);
    const size_t n = size * nmemb;
    if (b->out->size() + n > b->max) return 0;   // aborts the transfer
    b->out->append(data, n);
    return n;
}

size_t writeFile(char* data, size_t size, size_t nmemb, void* userdata) {
    return fwrite(data, size, nmemb, static_cast<FILE*>(userdata)) * size;
}

struct Transfer { const net::Progress* cb; uint64_t max; };

int onTransfer(void* userdata, curl_off_t total, curl_off_t now, curl_off_t, curl_off_t) {
    auto* t = static_cast<Transfer*>(userdata);
    if (static_cast<uint64_t>(total) > t->max || static_cast<uint64_t>(now) > t->max) return 1;
    if (t->cb && *t->cb && !(*t->cb)(static_cast<uint64_t>(now), static_cast<uint64_t>(total))) return 1;
    return 0;
}

} // anonymous namespace

namespace net {

bool start() {
    if (g_socketsUp && g_curlUp) return true;

    // romfs:/cacert.pem holds the Let's Encrypt and GitHub roots only, which
    // keeps mbedTLS's parse per handshake short. One on the SD card wins, so a
    // rotated root is a copied file rather than a new release.
    g_caBundle = std::string(config::DIR) + "/cacert.pem";
    if (!util::fileExists(g_caBundle)) g_caBundle = "romfs:/cacert.pem";
    if (!util::fileExists(g_caBundle)) g_caBundle.clear();

    if (!g_socketsUp) {
        if (R_FAILED(socketInitializeDefault())) return false;
        g_socketsUp = true;
    }
    if (!g_curlUp) {
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) return false;
        g_curlUp = true;
    }
    return true;
}

void stop() {
    if (g_curlUp) { curl_global_cleanup(); g_curlUp = false; }
    if (g_socketsUp) { socketExit(); g_socketsUp = false; }
}

bool online() {
    NifmInternetConnectionType type;
    u32 strength = 0;
    NifmInternetConnectionStatus status;
    const Result rc = nifmGetInternetConnectionStatus(&type, &strength, &status);
    return R_SUCCEEDED(rc) && status == NifmInternetConnectionStatus_Connected;
}

bool fetch(const std::string& url, const char* const* headers, std::string& body,
           long& status, std::string& error, size_t maxBytes) {
    status = 0;
    body.clear();
    CURL* curl = curl_easy_init();
    if (!curl) { error = "the network client did not start"; return false; }

    curl_slist* list = nullptr;
    for (const char* const* h = headers; h && *h; h++)
        list = curl_slist_append(list, *h);
    Body b{&body, maxBytes};
    commonOptions(curl, url, 20000L);
    if (list) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, appendBody);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &b);
    const CURLcode rc = curl_easy_perform(curl);
    if (rc == CURLE_OK) curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    else error = curl_easy_strerror(rc);
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    return rc == CURLE_OK;
}

bool redirectTarget(const std::string& url, std::string& location, long& status, std::string& error) {
    status = 0;
    location.clear();
    CURL* curl = curl_easy_init();
    if (!curl) { error = "the network client did not start"; return false; }
    commonOptions(curl, url, 20000L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    const CURLcode rc = curl_easy_perform(curl);
    if (rc == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        char* where = nullptr;
        if (curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &where) == CURLE_OK && where) location = where;
    } else {
        error = curl_easy_strerror(rc);
    }
    curl_easy_cleanup(curl);
    return rc == CURLE_OK;
}

bool download(const std::string& url, const std::string& path, const Progress& progress,
              std::string& error, uint64_t maxBytes) {
    const std::string part = path + ".part";
    util::ensureParentDir(path);
    remove(part.c_str());
    FILE* out = fopen(part.c_str(), "wb");
    if (!out) { error = "the file could not be created on the SD card"; return false; }

    CURL* curl = curl_easy_init();
    if (!curl) { fclose(out); remove(part.c_str()); error = "the network client did not start"; return false; }
    Transfer t{&progress, maxBytes};
    commonOptions(curl, url, 0L);
    // Give up on a transfer crawling below 1 KB/s for a minute, rather than
    // capping the whole download at a time a slow network would miss.
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);
    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 128L * 1024L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFile);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, onTransfer);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &t);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    const CURLcode rc = curl_easy_perform(curl);
    long status = 0;
    if (rc == CURLE_OK) curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_cleanup(curl);
    const bool closed = fclose(out) == 0;

    if (rc != CURLE_OK || !closed || status < 200 || status >= 300) {
        remove(part.c_str());
        error = rc == CURLE_ABORTED_BY_CALLBACK ? "cancelled"
              : rc != CURLE_OK ? std::string(curl_easy_strerror(rc))
              : !closed        ? std::string("the file could not be written to the SD card")
                               : "the server answered " + std::to_string(status);
        return false;
    }
    // FAT's rename does not replace, so the old file goes first.
    remove(path.c_str());
    if (rename(part.c_str(), path.c_str()) != 0) {
        remove(part.c_str());
        error = "the file could not be written to the SD card";
        return false;
    }
    return true;
}

} // namespace net
