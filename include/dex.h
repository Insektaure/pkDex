#pragma once
#include <functional>
#include <string>
#include <vector>

struct Pokemon {
    std::string id;          // national dex number, "001"; names the sprite files
    std::string regional;    // regional dex number, "001"; keys the tracker
    std::string name;
    std::string evolution;
    std::string exclusive;   // "-" when none
    std::string locations;
    std::vector<std::string> types;     // as displayed (may be translated)
    std::vector<std::string> typesEn;   // as in the data file, for the colours
    bool shinyLocked = false;
};

// A region, in sidebar order. A child is the DLC or the other game that sits
// under the region before it.
struct Region {
    const char* id;    // file name in romfs:/data, tracker file suffix, i18n key
    bool child;
    bool alpha;        // has Alpha and Shiny Alpha capture states
};

namespace dex {

const std::vector<Region>& regions();
int count();

// Display strings, from pkdex/regions/<id>/...
std::string name(int region);      // "Kanto"
std::string game(int region);      // "Let's Go! Pikachu & Let's Go! Eevee"
std::string sidebar(int region);   // the child entry's label, "FireRed & LeafGreen"
std::string tag(int region);       // "LGPE"

// Reads every region's data file (and its translation). `progress` is called
// with 0..1 between files.
void load(const std::function<void(float)>& progress);

const std::vector<Pokemon>& list(int region);

// Index in list(region) of the Pokemon with this displayed name, or -1.
int findByName(int region, const std::string& name);
// The Pokemon with this displayed name, in `region` first and then in any
// region (an evolution can name one the region's dex lacks); null if none.
const Pokemon* findAnywhere(int region, const std::string& name);

// Sprite paths: the high-resolution pack on the SD card when it is there,
// the small sprite in the RomFS otherwise.
std::string iconPath(const Pokemon& p);
std::string spritePath(const Pokemon& p, bool shiny);
void forgetSpritePaths();   // after the pack was extracted

} // namespace dex

// An evolution line, as the data files write it:
//   "Bulbasaur – Lvl 16 → Ivysaur – Lvl 32 → Venusaur"          one line
//   "Applin – Tart Apple → Flapple | – Sweet Apple → Appletun"   branches
// Each branch after the first starts from the root, in one of the styles the
// data uses: "| – condition → X", "| → X", "| Root → X", or a bare "| X".
struct EvoStep { std::string name; std::string condition; };   // the condition that leads to `name`
struct EvoBranch { std::string from; std::vector<EvoStep> steps; };
struct EvoTree { std::string root; std::vector<EvoBranch> branches; };
// False when there is nothing to draw as nodes ("-", or no arrow at all).
bool parseEvolution(const std::string& text, EvoTree& out);

// "Route 3, 4, 24, Rock Tunnel" -> {"Route 3, 4, 24", "Rock Tunnel"}: split on
// commas outside parentheses, a bare number staying with the place before it.
std::vector<std::string> splitLocations(const std::string& text);
