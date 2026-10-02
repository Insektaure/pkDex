#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

constexpr int SCREEN_W = 1280;
constexpr int SCREEN_H = 720;

// One size and weight of the console's shared fonts. Standard (Latin, Greek,
// Cyrillic, Japanese) draws everything it can; a glyph it lacks comes from the
// Korean, Chinese and Nintendo extension fonts, in that order, each opened at
// this size the first time it is needed. SDL_ttf has no fallback chain of its
// own, so text is cut into runs per font and the runs are put on one baseline.
struct Font {
    enum { Standard, Korean, ChineseSimplified, ChineseSimplifiedExt, ChineseTraditional, NintendoExt, Count };
    TTF_Font* face[Count] = {};
    bool tried[Count] = {};
    int size = 0;
    bool bold = false;
    int height = 0;   // of the Standard face; what layout uses
    int ascent = 0;
};

enum class Icon {
    Check, Sparkle, Alpha, Gear, Info, List, Image, Pin, Chevron, ChevronLeft, ChevronUp, ChevronDown, Download,
    Warning, Cross,
    Logo, Refresh,
};

class Gfx {
public:
    bool init(std::string& error);
    void shutdown();
    SDL_Renderer* renderer() const { return r_; }

    void beginFrame(SDL_Color bg);
    void present();

    // Something drawn this frame moves (a scrolling title, a spinner...), so
    // the next frame must be drawn too. Reset by beginFrame().
    void animate() { animating_ = true; }
    bool animating() const { return animating_; }

    // --- text ------------------------------------------------------------------
    Font* font(int size, bool bold = false);
    int textW(const std::string& s, Font* f);
    // y is the top of the line box (Font::height tall).
    void text(const std::string& s, int x, int y, SDL_Color c, Font* f);
    // Vertically centred on cy; return the width drawn.
    int textMid(const std::string& s, int x, int cy, SDL_Color c, Font* f);
    void textCenter(const std::string& s, int cx, int cy, SDL_Color c, Font* f);
    int textRight(const std::string& s, int right, int cy, SDL_Color c, Font* f);
    // Letter-spaced, for the small-caps labels. Returns the width.
    int tracked(const std::string& s, int x, int y, SDL_Color c, Font* f, int spacing);
    int trackedW(const std::string& s, Font* f, int spacing);
    std::string fit(const std::string& s, Font* f, int maxW);
    // Vertically centred on cy, in at most maxW. Wider text is shortened, or
    // when `active` (the cursor is on it) scrolls back and forth inside that
    // width, pausing at each end - from its start each time it becomes active.
    void marquee(const std::string& s, int x, int cy, int maxW, SDL_Color c, Font* f, bool active);
    // Word-wrapped (and character-wrapped where there are no spaces, as in
    // Japanese). maxLines 0 means no limit; the last kept line is ellipsised.
    std::vector<std::string> wrap(const std::string& s, Font* f, int maxW, int maxLines = 0);

    // --- shapes ----------------------------------------------------------------
    void rect(int x, int y, int w, int h, SDL_Color c);
    void fillRounded(int x, int y, int w, int h, int radius, SDL_Color c);
    void strokeRounded(int x, int y, int w, int h, int radius, int stroke, SDL_Color c);
    void disc(int cx, int cy, int radius, SDL_Color c);
    void ring(int cx, int cy, int radius, int stroke, SDL_Color c);
    void dashedRing(int cx, int cy, int radius, SDL_Color c, int dashes);
    void line(float x1, float y1, float x2, float y2, float thick, SDL_Color c);
    void polyline(const SDL_FPoint* pts, int n, float thick, SDL_Color c, bool closed = false);
    void fan(const SDL_FPoint* pts, int n, SDL_FPoint centre, SDL_Color c);
    void arc(int cx, int cy, int radius, float from, float to, float thick, SDL_Color c);   // radians
    void dotGrid(const SDL_Rect& area, SDL_Color c);
    void shade(Uint8 alpha);   // the whole screen dimmed, for popups
    void icon(Icon i, int cx, int cy, int size, SDL_Color c);
    void battery(int x, int cy, int percent, bool charging, SDL_Color c);

    // --- images ----------------------------------------------------------------
    // Cached by path; nullptr (also cached) when the file is missing. Small
    // sprites are scaled with nearest neighbour, so they stay crisp.
    SDL_Texture* image(const std::string& path, int* w = nullptr, int* h = nullptr);
    void blit(SDL_Texture* t, int x, int y, int w, int h, Uint8 alpha = 255);
    void blitFit(SDL_Texture* t, int cx, int cy, int box, Uint8 alpha = 255);   // aspect kept, centred
    void forgetImages();
    // Loaded once and kept until shutdown, outside the cache: for what must
    // stay drawable while the RomFS is unmounted (the updater's logo).
    SDL_Texture* pinned(const std::string& path);

    void clip(const SDL_Rect* r);

private:
    struct TextEntry { SDL_Texture* tex = nullptr; int w = 0, h = 0, ascent = 0; uint64_t used = 0; };
    struct ImageEntry { SDL_Texture* tex = nullptr; int w = 0, h = 0; uint64_t used = 0; };
    struct Run { int face; std::string text; };

    TTF_Font* face(Font* f, int which);
    std::vector<Run> runs(const std::string& s, Font* f);
    const TextEntry& entry(const std::string& s, Font* f, SDL_Color c);
    SDL_Texture* corner(int radius, int stroke);
    void corners(SDL_Texture* tex, int x, int y, int w, int h, int radius, SDL_Color c);

    SDL_Window* win_ = nullptr;
    SDL_Renderer* r_ = nullptr;
    void* fontData_[Font::Count] = {};
    size_t fontSize_[Font::Count] = {};
    std::unordered_map<int, Font> fonts_;
    std::unordered_map<std::string, TextEntry> text_;
    std::unordered_map<std::string, ImageEntry> images_;
    std::unordered_map<std::string, SDL_Texture*> pinned_;
    std::unordered_map<uint32_t, SDL_Texture*> corners_;
    std::unordered_map<uint32_t, SDL_Texture*> grids_;
    struct Marquee { uint32_t start = 0; uint64_t frame = 0; };
    std::unordered_map<std::string, Marquee> marquees_;
    uint64_t frame_ = 0;
    bool animating_ = false;
};

extern Gfx gfx;
