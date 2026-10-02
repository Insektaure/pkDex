#pragma once

// nlohmann/json, built without exceptions (the app compiles with
// -fno-exceptions). A failed conversion is then an abort, so every read off a
// parsed document checks the type first, and parsing goes through
// json::parse(text, nullptr, false), which answers a discarded value instead of
// throwing.
#ifndef JSON_NOEXCEPTION
#define JSON_NOEXCEPTION
#endif
#include "json.hpp"

using nlohmann::json;
