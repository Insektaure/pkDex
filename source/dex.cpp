#include "dex.h"
#include "i18n.h"
#include "json.h"
#include "util.h"

#include <cctype>
#include <unordered_map>

namespace {

const std::vector<Region> REGIONS = {
    //  id                    child  alpha  dlc
    {"kanto",              false, false, false},
    {"kanto_frlg",         true,  false, false},
    {"sinnoh",             false, false, false},
    {"sinnoh_arceus",      true,  true,  false},
    {"galar",              false, false, false},
    {"isle_armor",         true,  false, true },
    {"crown_tundra",       true,  false, true },
    {"paldea",             false, false, false},
    {"kitakami",           true,  false, true },
    {"blueberry_academy",  true,  false, true },
    {"kalos_lza",          false, true,  false},
    {"hyperspace_lumiose", true,  true,  true },
};

std::vector<std::vector<Pokemon>> g_lists(REGIONS.size());
std::unordered_map<std::string, std::string> g_spritePaths;

std::string jsonString(const json& j, const char* key) {
    auto it = j.find(key);
    return (it != j.end() && it->is_string()) ? it->get<std::string>() : std::string();
}

bool jsonBool(const json& j, const char* key) {
    auto it = j.find(key);
    return it != j.end() && it->is_boolean() && it->get<bool>();
}

std::vector<std::string> splitTypes(const std::string& s) {
    std::vector<std::string> out;
    for (const std::string& part : util::split(s, "/")) {
        std::string t = util::trim(part);
        if (!t.empty()) out.push_back(t);
    }
    return out;
}

// The translation of one field, when the locale has one.
void applyOverride(std::string& field, const std::string& region, const std::string& regional, const char* name) {
    const std::string key = "data/" + region + "/" + regional + "/" + name;
    if (i18n::has(key)) field = i18n::get(key);
}

std::vector<Pokemon> loadRegion(const std::string& region) {
    std::vector<Pokemon> out;
    std::string text;
    if (!util::readFile("romfs:/data/" + region + ".json", text)) return out;
    const json doc = json::parse(text, nullptr, false);
    if (doc.is_discarded() || !doc.is_object()) return out;
    auto list = doc.find("pokemon");
    if (list == doc.end() || !list->is_array()) return out;

    out.reserve(list->size());
    for (const auto& e : *list) {
        if (!e.is_object()) continue;
        Pokemon p;
        p.id          = jsonString(e, "id");
        p.regional    = jsonString(e, "regionalDexNumber");
        p.name        = jsonString(e, "name");
        p.evolution   = jsonString(e, "evolution");
        p.exclusive   = jsonString(e, "exclusiveVersion");
        p.locations   = jsonString(e, "locations");
        p.shinyLocked = jsonBool(e, "shinyLocked");
        std::string type = jsonString(e, "type");
        p.typesEn = splitTypes(type);
        if (!p.regional.empty()) {
            applyOverride(p.name, region, p.regional, "name");
            applyOverride(type, region, p.regional, "type");
            applyOverride(p.evolution, region, p.regional, "evolution");
            applyOverride(p.exclusive, region, p.regional, "exclusiveVersion");
            applyOverride(p.locations, region, p.regional, "locations");
        }
        p.types = splitTypes(type);
        out.push_back(std::move(p));
    }
    return out;
}

} // anonymous namespace

namespace dex {

const std::vector<Region>& regions() { return REGIONS; }
int count() { return static_cast<int>(REGIONS.size()); }

std::string name(int r)    { return tr(std::string("regions/") + REGIONS[r].id + "/name"); }
std::string game(int r)    { return tr(std::string("regions/") + REGIONS[r].id + "/game"); }
std::string sidebar(int r) { return tr(std::string("regions/") + REGIONS[r].id + "/sidebar"); }
std::string tag(int r)     { return tr(std::string("regions/") + REGIONS[r].id + "/tag"); }

void load(const std::function<void(float)>& progress) {
    for (size_t i = 0; i < REGIONS.size(); i++) {
        if (progress) progress(static_cast<float>(i) / static_cast<float>(REGIONS.size()));
        g_lists[i] = loadRegion(REGIONS[i].id);
    }
    if (progress) progress(1.0f);
}

const std::vector<Pokemon>& list(int region) { return g_lists[region]; }

int findByName(int region, const std::string& name) {
    const auto& l = g_lists[region];
    for (size_t i = 0; i < l.size(); i++)
        if (l[i].name == name) return static_cast<int>(i);
    return -1;
}

const Pokemon* findAnywhere(int region, const std::string& name) {
    const int here = findByName(region, name);
    if (here >= 0) return &g_lists[region][here];
    for (int r = 0; r < count(); r++) {
        if (r == region) continue;
        const int i = findByName(r, name);
        if (i >= 0) return &g_lists[r][i];
    }
    return nullptr;
}

std::string iconPath(const Pokemon& p) { return "romfs:/img/pokemon/icons/" + p.id + ".png"; }

std::string spritePath(const Pokemon& p, bool shiny) {
    const std::string rel = std::string("img/pokemon/") + (shiny ? "shiny/" : "full/") + p.id + ".png";
    auto it = g_spritePaths.find(rel);
    if (it != g_spritePaths.end()) return it->second;
    // Asked once per sprite: a stat on the SD card is not free.
    const std::string sd = "sdmc:/switch/pkDex/resources/" + rel;
    const std::string path = util::fileExists(sd) ? sd : "romfs:/" + rel;
    g_spritePaths[rel] = path;
    return path;
}

void forgetSpritePaths() { g_spritePaths.clear(); }

} // namespace dex

namespace {

// "Name – condition": an en dash or a spaced hyphen. A bare hyphen is part of
// the name (Porygon-Z, Ho-Oh). Either side may be empty ("– Lvl 20").
void splitStep(const std::string& s, std::string& name, std::string& cond) {
    size_t at = s.find("–");   // 3 bytes of UTF-8, like " - "
    if (at == std::string::npos) at = s.find(" - ");
    if (at == std::string::npos) {
        name = util::trim(s);
        cond.clear();
        return;
    }
    name = util::trim(s.substr(0, at));
    cond = util::trim(s.substr(at + 3));
}

} // anonymous namespace

bool parseEvolution(const std::string& text, EvoTree& out) {
    out = EvoTree{};
    const std::string t = util::trim(text);
    if (t.empty() || t == "-") return false;

    const std::vector<std::string> parts = util::split(t, "|");
    for (size_t pi = 0; pi < parts.size(); pi++) {
        const std::string part = util::trim(parts[pi]);
        if (part.empty()) continue;
        const std::vector<std::string> segs = util::split(part, "→");
        std::string name, cond;
        splitStep(segs[0], name, cond);

        EvoBranch b;
        if (pi == 0) {
            if (segs.size() < 2 || name.empty()) return false;
            out.root = name;
            b.from = name;
        } else if (segs.size() == 1) {
            // "Eevee → Vaporeon | Jolteon | Flareon": one more target of the root.
            if (name.empty()) continue;
            b.from = out.root;
            b.steps.push_back({name, std::string()});
            out.branches.push_back(b);
            continue;
        } else {
            // "– condition → X" and "→ X" start at the root, "Root → X" names it.
            b.from = name.empty() ? out.root : name;
        }
        std::string pending = cond;
        for (size_t i = 1; i < segs.size(); i++) {
            splitStep(segs[i], name, cond);
            if (name.empty()) return false;
            b.steps.push_back({name, pending});
            pending = cond;
        }
        out.branches.push_back(b);
    }
    return !out.branches.empty() && !out.branches[0].steps.empty();
}

std::vector<std::string> splitLocations(const std::string& text) {
    std::vector<std::string> parts;
    const std::string t = util::trim(text);
    if (t.empty() || t == "-") return parts;

    int depth = 0;
    std::string cur;
    auto flush = [&]() {
        const std::string p = util::trim(cur);
        cur.clear();
        if (p.empty()) return;
        if (!parts.empty() && isdigit(static_cast<unsigned char>(p[0])))
            parts.back() += ", " + p;
        else
            parts.push_back(p);
    };
    for (char c : t) {
        if (c == '(') depth++;
        else if (c == ')' && depth > 0) depth--;
        if (c == ',' && depth == 0) { flush(); continue; }
        cur += c;
    }
    flush();
    return parts;
}
