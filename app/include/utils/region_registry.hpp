#pragma once

#include <string>
#include <vector>

namespace pkdex {

struct RegionInfo {
    std::string id;
    std::string xmlViewName;
    bool hasAlpha;
};

inline const std::vector<RegionInfo>& getRegionRegistry() {
    static const std::vector<RegionInfo> regions = {
        {"kanto",               "KantoTab",             false},
        {"kanto_frlg",          "KantoFrlgTab",         false},
        {"sinnoh",              "SinnohTab",            false},
        {"sinnoh_arceus",       "SinnohArceusTab",      true},
        {"galar",               "GalarTab",             false},
        {"isle_armor",          "IsleArmorTab",         false},
        {"crown_tundra",        "CrownTundraTab",       false},
        {"paldea",              "PaldeaTab",            false},
        {"kitakami",            "KitakamiTab",          false},
        {"blueberry_academy",   "BlueberryTab",         false},
        {"kalos_lza",           "KalosLzaTab",          true},
        {"hyperspace_lumiose",  "HyperspaceLumioseTab", true},
    };
    return regions;
}

inline bool regionHasAlpha(const std::string& region) {
    for (const auto& info : getRegionRegistry()) {
        if (info.id == region) return info.hasAlpha;
    }
    return false;
}

} // namespace pkdex
