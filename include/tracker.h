#pragma once
#include <string>
#include <vector>

struct Pokemon;

// What has been caught, per Pokemon: four flags, in the order of the files.
struct Capture {
    enum State { Regular = 0, Shiny = 1, Alpha = 2, ShinyAlpha = 3, Count = 4 };
    bool flags[Count] = {false, false, false, false};

    bool any() const { return flags[0] || flags[1] || flags[2] || flags[3]; }
    bool get(int s) const { return s >= 0 && s < Count && flags[s]; }
    void set(int s, bool v) { if (s >= 0 && s < Count) flags[s] = v; }
};

// The capture files of earlier releases, unchanged:
// sdmc:/switch/pkDex/pkDex.tracker.<region>.ini, "<region>_<regional>=1,0,0,1".
// Each region's file is read on first use and kept in memory; every change
// writes it back whole.
namespace tracker {

Capture get(const std::string& region, const std::string& regional);
void set(const std::string& region, const std::string& regional, const Capture& c);
void bulkSet(const std::string& region, const std::vector<std::string>& regionals, int state, bool value);

bool resetRegion(const std::string& region);
bool resetAll();

std::string fileName(const std::string& region);   // "pkDex.tracker.kanto.ini"

struct Counts { int any = 0; int state[Capture::Count] = {0, 0, 0, 0}; };
Counts counts(const std::string& region, const std::vector<Pokemon>& list);

// Bumped by every change, so a caller can tell when its counts are stale.
unsigned version();

} // namespace tracker
