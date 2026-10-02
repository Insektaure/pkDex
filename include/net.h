#pragma once
#include <cstdint>
#include <functional>
#include <string>

// HTTPS over libcurl. start() runs on the main thread, before any worker asks
// for anything; the calls after it are safe from a worker.
namespace net {

// Sockets and curl, once. False when either would not start.
bool start();
void stop();

// The console reports an internet connection (Wi-Fi or wired).
bool online();

// GET into memory, at most `maxBytes`. `status` is the HTTP status.
bool fetch(const std::string& url, const char* const* headers, std::string& body,
           long& status, std::string& error, size_t maxBytes);

// HEAD without following redirects: where `url` points to (empty if it does
// not redirect).
bool redirectTarget(const std::string& url, std::string& location, long& status, std::string& error);

// GET into a file, through `<path>.part` so a stopped transfer never leaves a
// truncated file under the real name. `progress` returns false to abort.
using Progress = std::function<bool(uint64_t done, uint64_t total)>;
bool download(const std::string& url, const std::string& path, const Progress& progress,
              std::string& error, uint64_t maxBytes);

} // namespace net
