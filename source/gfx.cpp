#include "gfx.h"
#include "theme.h"
#include "util.h"

#include <SDL2/SDL_image.h>
#include <switch.h>

#include <algorithm>
#include <cmath>
#include <iterator>

Gfx gfx;

namespace {

constexpr float PI = 3.14159265f;

// The shared fonts, in Font's face order.
const PlSharedFontType FONT_TYPES[Font::Count] = {
    PlSharedFontType_Standard,
    PlSharedFontType_KO,
    PlSharedFontType_ChineseSimplified,
    PlSharedFontType_ExtChineseSimplified,
    PlSharedFontType_ChineseTraditional,
    PlSharedFontType_NintendoExt,
};

// Stroke value that makes a corner texture the outside of the arc.
constexpr int CORNER_MASK = 0xFFFF;

uint32_t pack(SDL_Color c) { return (uint32_t(c.r) << 24) | (uint32_t(c.g) << 16) | (uint32_t(c.b) << 8) | c.a; }

bool isAscii(const std::string& s) {
    for (unsigned char ch : s)
        if (ch >= 0x80) return false;
    return true;
}

} // anonymous namespace

// --- lifetime --------------------------------------------------------------------

bool Gfx::init(std::string& error) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) { error = SDL_GetError(); return false; }
    if (TTF_Init() < 0) { error = TTF_GetError(); return false; }
    if ((IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & IMG_INIT_PNG) == 0) { error = IMG_GetError(); return false; }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    win_ = SDL_CreateWindow("pkDex", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H,
                            SDL_WINDOW_SHOWN);
    if (!win_) { error = SDL_GetError(); return false; }
    r_ = SDL_CreateRenderer(win_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!r_) { error = SDL_GetError(); return false; }
    SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);

    // pl:u, not pl:s: since system 16.0.0 the shared fonts are only handed out
    // there. Standard is required; the others only widen what can be drawn.
    if (R_SUCCEEDED(plInitialize(PlServiceType_User))) {
        for (int i = 0; i < Font::Count; i++) {
            PlFontData data{};
            if (R_SUCCEEDED(plGetSharedFontByType(&data, FONT_TYPES[i])) && data.address && data.size) {
                fontData_[i] = data.address;
                fontSize_[i] = data.size;
            }
        }
    }
    if (!fontData_[Font::Standard] || !face(font(16), Font::Standard)) {
        error = "The console's system font could not be loaded.";
        return false;
    }
    return true;
}

void Gfx::shutdown() {
    for (auto& [k, e] : text_) if (e.tex) SDL_DestroyTexture(e.tex);
    text_.clear();
    forgetImages();
    for (auto& [k, t] : pinned_) if (t) SDL_DestroyTexture(t);
    pinned_.clear();
    for (auto& [k, t] : corners_) if (t) SDL_DestroyTexture(t);
    corners_.clear();
    for (auto& [k, t] : grids_) if (t) SDL_DestroyTexture(t);
    grids_.clear();
    for (auto& [k, f] : fonts_)
        for (TTF_Font* t : f.face) if (t) TTF_CloseFont(t);
    fonts_.clear();
    if (r_) SDL_DestroyRenderer(r_);
    if (win_) SDL_DestroyWindow(win_);
    r_ = nullptr;
    win_ = nullptr;
    plExit();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

void Gfx::beginFrame(SDL_Color bg) {
    frame_++;
    animating_ = false;
    SDL_RenderSetClipRect(r_, nullptr);
    SDL_SetRenderDrawColor(r_, bg.r, bg.g, bg.b, 255);
    SDL_RenderClear(r_);
}

void Gfx::present() {
    SDL_RenderPresent(r_);
    // Text drawn this frame or the one before stays; the rest goes once the
    // cache has grown, so a long scroll does not hold every label it passed.
    if (text_.size() > 700) {
        for (auto it = text_.begin(); it != text_.end();) {
            if (it->second.used + 1 < frame_) {
                if (it->second.tex) SDL_DestroyTexture(it->second.tex);
                it = text_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void Gfx::clip(const SDL_Rect* r) { SDL_RenderSetClipRect(r_, r); }

// --- fonts -------------------------------------------------------------------------

Font* Gfx::font(int size, bool bold) {
    const int key = size * 2 + (bold ? 1 : 0);
    auto it = fonts_.find(key);
    if (it != fonts_.end()) return &it->second;
    Font& f = fonts_[key];
    f.size = size;
    f.bold = bold;
    if (TTF_Font* s = face(&f, Font::Standard)) {
        f.height = TTF_FontHeight(s);
        f.ascent = TTF_FontAscent(s);
    } else {
        f.height = size;
        f.ascent = size;
    }
    return &f;
}

TTF_Font* Gfx::face(Font* f, int which) {
    if (f->face[which] || f->tried[which]) return f->face[which];
    f->tried[which] = true;
    if (!fontData_[which]) return nullptr;
    SDL_RWops* rw = SDL_RWFromConstMem(fontData_[which], static_cast<int>(fontSize_[which]));
    TTF_Font* t = rw ? TTF_OpenFontRW(rw, 1, f->size) : nullptr;
    if (t && f->bold) TTF_SetFontStyle(t, TTF_STYLE_BOLD);
    f->face[which] = t;
    return t;
}

std::vector<Gfx::Run> Gfx::runs(const std::string& s, Font* f) {
    std::vector<Run> out;
    if (s.empty()) return out;
    if (isAscii(s)) { out.push_back({Font::Standard, s}); return out; }

    TTF_Font* standard = face(f, Font::Standard);
    size_t i = 0;
    while (i < s.size()) {
        const size_t start = i;
        const char32_t c = util::utf8Next(s, i);
        int which = Font::Standard;
        if (c >= 0x20 && standard && !TTF_GlyphIsProvided32(standard, c)) {
            for (int k = 1; k < Font::Count; k++) {
                TTF_Font* t = face(f, k);
                if (t && TTF_GlyphIsProvided32(t, c)) { which = k; break; }
            }
        }
        if (!out.empty() && out.back().face == which)
            out.back().text.append(s, start, i - start);
        else
            out.push_back({which, s.substr(start, i - start)});
    }
    return out;
}

int Gfx::textW(const std::string& s, Font* f) {
    if (!f || s.empty()) return 0;
    int total = 0;
    for (const Run& run : runs(s, f)) {
        TTF_Font* t = face(f, run.face);
        if (!t) t = face(f, Font::Standard);
        int w = 0, h = 0;
        if (t && TTF_SizeUTF8(t, run.text.c_str(), &w, &h) == 0) total += w;
    }
    return total;
}

const Gfx::TextEntry& Gfx::entry(const std::string& s, Font* f, SDL_Color c) {
    // Rendered opaque and cached once per colour; text() applies the alpha,
    // so a fading label does not fill the cache with one texture per step.
    c.a = 255;
    std::string key = s;
    key += '\x1f';
    key += std::to_string(reinterpret_cast<uintptr_t>(f));
    key += '\x1f';
    key += std::to_string(pack(c));
    auto it = text_.find(key);
    if (it != text_.end()) {
        it->second.used = frame_;
        return it->second;
    }

    TextEntry e;
    e.used = frame_;
    const std::vector<Run> parts = runs(s, f);
    SDL_Surface* surf = nullptr;
    if (parts.size() == 1) {
        TTF_Font* t = face(f, parts[0].face);
        if (!t) t = face(f, Font::Standard);
        if (t) {
            surf = TTF_RenderUTF8_Blended(t, parts[0].text.c_str(), c);
            e.ascent = TTF_FontAscent(t);
        }
    } else if (!parts.empty()) {
        // Each run in its own font, then side by side on one baseline.
        std::vector<SDL_Surface*> pieces;
        std::vector<int> ascents;
        int w = 0, above = 0, below = 0;
        for (const Run& run : parts) {
            TTF_Font* t = face(f, run.face);
            if (!t) t = face(f, Font::Standard);
            SDL_Surface* p = t ? TTF_RenderUTF8_Blended(t, run.text.c_str(), c) : nullptr;
            if (!p) continue;
            const int a = TTF_FontAscent(t);
            pieces.push_back(p);
            ascents.push_back(a);
            w += p->w;
            above = std::max(above, a);
            below = std::max(below, p->h - a);
        }
        if (!pieces.empty()) {
            surf = SDL_CreateRGBSurfaceWithFormat(0, w, above + below, 32, SDL_PIXELFORMAT_RGBA32);
            int x = 0;
            for (size_t i = 0; i < pieces.size(); i++) {
                if (surf) {
                    SDL_SetSurfaceBlendMode(pieces[i], SDL_BLENDMODE_NONE);
                    SDL_Rect dst{x, above - ascents[i], pieces[i]->w, pieces[i]->h};
                    SDL_BlitSurface(pieces[i], nullptr, surf, &dst);
                }
                x += pieces[i]->w;
                SDL_FreeSurface(pieces[i]);
            }
            e.ascent = above;
        }
    }
    if (surf) {
        e.tex = SDL_CreateTextureFromSurface(r_, surf);
        e.w = surf->w;
        e.h = surf->h;
        SDL_FreeSurface(surf);
    }
    return text_.emplace(std::move(key), e).first->second;
}

void Gfx::text(const std::string& s, int x, int y, SDL_Color c, Font* f) {
    if (!f || s.empty()) return;
    const TextEntry& e = entry(s, f, c);
    if (!e.tex) return;
    SDL_Rect dst{x, y + f->ascent - e.ascent, e.w, e.h};
    SDL_SetTextureAlphaMod(e.tex, c.a);
    SDL_RenderCopy(r_, e.tex, nullptr, &dst);
}

int Gfx::textMid(const std::string& s, int x, int cy, SDL_Color c, Font* f) {
    if (!f || s.empty()) return 0;
    text(s, x, cy - f->height / 2, c, f);
    return entry(s, f, c).w;
}

void Gfx::textCenter(const std::string& s, int cx, int cy, SDL_Color c, Font* f) {
    if (!f || s.empty()) return;
    const int w = entry(s, f, c).w;
    text(s, cx - w / 2, cy - f->height / 2, c, f);
}

int Gfx::textRight(const std::string& s, int right, int cy, SDL_Color c, Font* f) {
    if (!f || s.empty()) return 0;
    const int w = entry(s, f, c).w;
    text(s, right - w, cy - f->height / 2, c, f);
    return w;
}

int Gfx::trackedW(const std::string& s, Font* f, int spacing) {
    int w = 0, n = 0;
    for (size_t i = 0; i < s.size();) {
        const size_t start = i;
        util::utf8Next(s, i);
        w += textW(s.substr(start, i - start), f);
        n++;
    }
    return n > 1 ? w + spacing * (n - 1) : w;
}

int Gfx::tracked(const std::string& s, int x, int y, SDL_Color c, Font* f, int spacing) {
    const int x0 = x;
    for (size_t i = 0; i < s.size();) {
        const size_t start = i;
        util::utf8Next(s, i);
        const std::string cp = s.substr(start, i - start);
        text(cp, x, y, c, f);
        x += textW(cp, f);
        if (i < s.size()) x += spacing;
    }
    return x - x0;
}

std::string Gfx::fit(const std::string& s, Font* f, int maxW) {
    if (textW(s, f) <= maxW) return s;
    std::vector<size_t> ends;
    for (size_t i = 0; i < s.size();) {
        util::utf8Next(s, i);
        ends.push_back(i);
    }
    for (int n = static_cast<int>(ends.size()) - 1; n > 0; n--) {
        std::string cut = util::trim(s.substr(0, ends[n - 1])) + "...";
        if (textW(cut, f) <= maxW) return cut;
    }
    return "...";
}

void Gfx::marquee(const std::string& s, int x, int cy, int maxW, SDL_Color c, Font* f, bool active) {
    if (!f || s.empty() || maxW <= 0) return;
    const int w = textW(s, f);
    if (w <= maxW) {
        textMid(s, x, cy, c, f);
        return;
    }
    if (!active) {
        textMid(fit(s, f, maxW), x, cy, c, f);
        return;
    }
    animate();
    // Restarted when it was not drawn active in the frame before.
    Marquee& m = marquees_[s];
    if (m.frame + 1 < frame_) m.start = SDL_GetTicks();
    m.frame = frame_;
    if (marquees_.size() > 64)
        for (auto it = marquees_.begin(); it != marquees_.end();)
            it = it->second.frame + 1 < frame_ ? marquees_.erase(it) : std::next(it);

    // Pause, travel to the end, pause, travel back: 40 px a second, so a
    // label can be read as it goes.
    constexpr uint32_t PAUSE = 1500;
    const int over = w - maxW;
    const uint32_t travel = static_cast<uint32_t>(over) * 1000 / 40 + 1;
    const uint32_t t = (SDL_GetTicks() - m.start) % (2 * (PAUSE + travel));
    int offset = 0;
    if (t < PAUSE)                      offset = 0;
    else if (t < PAUSE + travel)        offset = static_cast<int>((t - PAUSE) * over / travel);
    else if (t < 2 * PAUSE + travel)    offset = over;
    else                                offset = over - static_cast<int>((t - 2 * PAUSE - travel) * over / travel);

    // Inside the caller's clip, if there is one.
    SDL_Rect prev{};
    const bool hadClip = SDL_RenderIsClipEnabled(r_) == SDL_TRUE;
    if (hadClip) SDL_RenderGetClipRect(r_, &prev);
    SDL_Rect box{x, cy - f->height / 2 - 2, maxW, f->height + 4};
    if (hadClip) SDL_IntersectRect(&box, &prev, &box);
    SDL_RenderSetClipRect(r_, &box);
    textMid(s, x - offset, cy, c, f);
    SDL_RenderSetClipRect(r_, hadClip ? &prev : nullptr);
}

std::vector<std::string> Gfx::wrap(const std::string& s, Font* f, int maxW, int maxLines) {
    std::vector<std::string> out;
    for (const std::string& para : util::split(s, "\n")) {
        std::string line;
        size_t i = 0;
        while (i < para.size()) {
            const size_t start = i;
            util::utf8Next(para, i);
            const std::string ch = para.substr(start, i - start);
            const std::string cand = line + ch;
            if (line.empty() || textW(cand, f) <= maxW) {
                line = cand;
                continue;
            }
            // Over: break at the last space when there is one, else here.
            const size_t sp = line.rfind(' ');
            if (sp != std::string::npos && sp > 0) {
                out.push_back(line.substr(0, sp));
                line = line.substr(sp + 1) + ch;
            } else {
                out.push_back(line);
                line = ch;
            }
            if (line == " ") line.clear();
        }
        out.push_back(line);
    }
    if (maxLines > 0 && static_cast<int>(out.size()) > maxLines) {
        const std::string last = out[maxLines - 1];
        out.resize(maxLines);
        out.back() = fit(last + "...", f, maxW);
    }
    return out;
}

// --- shapes ------------------------------------------------------------------------

void Gfx::rect(int x, int y, int w, int h, SDL_Color c) {
    if (w <= 0 || h <= 0) return;
    SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, c.a);
    SDL_Rect rr{x, y, w, h};
    SDL_RenderFillRect(r_, &rr);
}

// SDL2 has no rounded rectangles. Each corner is a small white texture holding
// the top-left quarter of a disc (or of a ring, for outlines), anti-aliased by
// supersampling; the four corners are that texture flipped, and the straight
// parts are plain rects. Colour comes from the texture's colour and alpha mods.
SDL_Texture* Gfx::corner(int radius, int stroke) {
    const uint32_t key = uint32_t(radius) | (uint32_t(stroke) << 16);
    auto it = corners_.find(key);
    if (it != corners_.end()) return it->second;

    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, radius, radius, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surf) { corners_[key] = nullptr; return nullptr; }

    constexpr int SS = 4;
    const bool mask = stroke == CORNER_MASK;
    const double outer = radius;
    const double inner = (stroke > 0 && !mask) ? radius - stroke : -1.0;
    Uint32* px = static_cast<Uint32*>(surf->pixels);
    const int pitch = surf->pitch / 4;
    for (int y = 0; y < radius; y++) {
        for (int x = 0; x < radius; x++) {
            int hits = 0;
            for (int sy = 0; sy < SS; sy++) {
                for (int sx = 0; sx < SS; sx++) {
                    const double dx = radius - (x + (sx + 0.5) / SS);
                    const double dy = radius - (y + (sy + 0.5) / SS);
                    const double d = std::sqrt(dx * dx + dy * dy);
                    if (d <= outer && d > inner) hits++;
                }
            }
            if (mask) hits = SS * SS - hits;
            px[y * pitch + x] = SDL_MapRGBA(surf->format, 255, 255, 255, static_cast<Uint8>(hits * 255 / (SS * SS)));
        }
    }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(r_, surf);
    SDL_FreeSurface(surf);
    if (tex) SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    corners_[key] = tex;
    return tex;
}

void Gfx::corners(SDL_Texture* tex, int x, int y, int w, int h, int radius, SDL_Color c) {
    SDL_SetTextureColorMod(tex, c.r, c.g, c.b);
    SDL_SetTextureAlphaMod(tex, c.a);
    const SDL_Rect tl{x, y, radius, radius};
    const SDL_Rect tr{x + w - radius, y, radius, radius};
    const SDL_Rect bl{x, y + h - radius, radius, radius};
    const SDL_Rect br{x + w - radius, y + h - radius, radius, radius};
    SDL_RenderCopyEx(r_, tex, nullptr, &tl, 0, nullptr, SDL_FLIP_NONE);
    SDL_RenderCopyEx(r_, tex, nullptr, &tr, 0, nullptr, SDL_FLIP_HORIZONTAL);
    SDL_RenderCopyEx(r_, tex, nullptr, &bl, 0, nullptr, SDL_FLIP_VERTICAL);
    SDL_RenderCopyEx(r_, tex, nullptr, &br, 0, nullptr,
                     static_cast<SDL_RendererFlip>(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL));
}

void Gfx::fillRounded(int x, int y, int w, int h, int radius, SDL_Color c) {
    if (w <= 0 || h <= 0) return;
    radius = std::min({radius, w / 2, h / 2});
    SDL_Texture* tex = radius > 0 ? corner(radius, 0) : nullptr;
    if (!tex) { rect(x, y, w, h, c); return; }
    corners(tex, x, y, w, h, radius, c);
    rect(x + radius, y, w - 2 * radius, h, c);
    rect(x, y + radius, radius, h - 2 * radius, c);
    rect(x + w - radius, y + radius, radius, h - 2 * radius, c);
}

void Gfx::strokeRounded(int x, int y, int w, int h, int radius, int stroke, SDL_Color c) {
    if (w <= 0 || h <= 0 || stroke <= 0) return;
    radius = std::min({radius, w / 2, h / 2});
    SDL_Texture* tex = radius > 0 ? corner(radius, stroke) : nullptr;
    if (!tex) {
        rect(x, y, w, stroke, c);
        rect(x, y + h - stroke, w, stroke, c);
        rect(x, y + stroke, stroke, h - 2 * stroke, c);
        rect(x + w - stroke, y + stroke, stroke, h - 2 * stroke, c);
        return;
    }
    corners(tex, x, y, w, h, radius, c);
    rect(x + radius, y, w - 2 * radius, stroke, c);
    rect(x + radius, y + h - stroke, w - 2 * radius, stroke, c);
    rect(x, y + radius, stroke, h - 2 * radius, c);
    rect(x + w - stroke, y + radius, stroke, h - 2 * radius, c);
}

void Gfx::disc(int cx, int cy, int radius, SDL_Color c) {
    if (radius <= 0) return;
    fillRounded(cx - radius, cy - radius, radius * 2, radius * 2, radius, c);
}

void Gfx::ring(int cx, int cy, int radius, int stroke, SDL_Color c) {
    if (radius <= 0) return;
    strokeRounded(cx - radius, cy - radius, radius * 2, radius * 2, radius, stroke, c);
}

void Gfx::line(float x1, float y1, float x2, float y2, float thick, SDL_Color c) {
    const float dx = x2 - x1, dy = y2 - y1;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.001f) return;
    const float nx = -dy / len * thick * 0.5f, ny = dx / len * thick * 0.5f;
    SDL_Vertex v[4] = {
        {{x1 + nx, y1 + ny}, c, {0, 0}},
        {{x1 - nx, y1 - ny}, c, {0, 0}},
        {{x2 - nx, y2 - ny}, c, {0, 0}},
        {{x2 + nx, y2 + ny}, c, {0, 0}},
    };
    const int idx[6] = {0, 1, 2, 0, 2, 3};
    SDL_RenderGeometry(r_, nullptr, v, 4, idx, 6);
}

void Gfx::polyline(const SDL_FPoint* pts, int n, float thick, SDL_Color c, bool closed) {
    for (int i = 0; i + 1 < n; i++) line(pts[i].x, pts[i].y, pts[i + 1].x, pts[i + 1].y, thick, c);
    if (closed && n > 2) line(pts[n - 1].x, pts[n - 1].y, pts[0].x, pts[0].y, thick, c);
    // Round joints, so the corners of a check mark do not notch.
    const int jr = static_cast<int>(thick * 0.5f + 0.5f);
    if (jr >= 2)
        for (int i = closed ? 0 : 1; i < (closed ? n : n - 1); i++)
            disc(static_cast<int>(pts[i].x + 0.5f), static_cast<int>(pts[i].y + 0.5f), jr, c);
}

void Gfx::fan(const SDL_FPoint* pts, int n, SDL_FPoint centre, SDL_Color c) {
    if (n < 2) return;
    std::vector<SDL_Vertex> v;
    std::vector<int> idx;
    v.push_back({centre, c, {0, 0}});
    for (int i = 0; i < n; i++) v.push_back({pts[i], c, {0, 0}});
    for (int i = 0; i < n; i++) {
        idx.push_back(0);
        idx.push_back(1 + i);
        idx.push_back(1 + (i + 1) % n);
    }
    SDL_RenderGeometry(r_, nullptr, v.data(), static_cast<int>(v.size()), idx.data(), static_cast<int>(idx.size()));
}

void Gfx::arc(int cx, int cy, int radius, float from, float to, float thick, SDL_Color c) {
    const int steps = std::max(6, static_cast<int>(std::fabs(to - from) * radius / 3.0f));
    std::vector<SDL_FPoint> pts;
    for (int i = 0; i <= steps; i++) {
        const float a = from + (to - from) * i / steps;
        pts.push_back({cx + std::cos(a) * radius, cy + std::sin(a) * radius});
    }
    for (size_t i = 0; i + 1 < pts.size(); i++) line(pts[i].x, pts[i].y, pts[i + 1].x, pts[i + 1].y, thick, c);
}

void Gfx::dashedRing(int cx, int cy, int radius, SDL_Color c, int dashes) {
    const float step = 2.0f * PI / dashes;
    for (int i = 0; i < dashes; i++) {
        const float a = i * step;
        arc(cx, cy, radius, a, a + step * 0.5f, 1.2f, c);
    }
}

void Gfx::dotGrid(const SDL_Rect& area, SDL_Color c) {
    // Drawn once, in white, for the whole screen: each use copies its part of
    // it in its own colour, so the dots line up across panels.
    SDL_Texture*& tex = grids_[0];
    if (!tex) {
        SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_W, SCREEN_H, 32, SDL_PIXELFORMAT_RGBA32);
        if (!s) return;
        Uint32* px = static_cast<Uint32*>(s->pixels);
        const int pitch = s->pitch / 4;
        const Uint32 on = SDL_MapRGBA(s->format, 255, 255, 255, 255);
        for (int y = 8; y + 1 < SCREEN_H; y += 16)
            for (int x = 8; x + 1 < SCREEN_W; x += 16) {
                px[y * pitch + x] = on;
                px[y * pitch + x + 1] = on;
                px[(y + 1) * pitch + x] = on;
                px[(y + 1) * pitch + x + 1] = on;
            }
        tex = SDL_CreateTextureFromSurface(r_, s);
        SDL_FreeSurface(s);
        if (!tex) return;
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    }
    SDL_SetTextureColorMod(tex, c.r, c.g, c.b);
    SDL_SetTextureAlphaMod(tex, c.a);
    SDL_RenderCopy(r_, tex, &area, &area);
}

void Gfx::shade(Uint8 alpha) { rect(0, 0, SCREEN_W, SCREEN_H, SDL_Color{0, 0, 0, alpha}); }

void Gfx::battery(int x, int cy, int percent, bool charging, SDL_Color c) {
    const int w = 22, h = 12, y = cy - h / 2;
    strokeRounded(x, y, w, h, 3, 1, c);
    rect(x + w, cy - 2, 2, 4, c);
    const int inner = std::max(1, (w - 4) * std::clamp(percent, 0, 100) / 100);
    fillRounded(x + 2, y + 2, inner, h - 4, 1, charging ? col::green : c);
}

void Gfx::icon(Icon i, int cx, int cy, int size, SDL_Color c) {
    const float s = static_cast<float>(size);
    const float x = static_cast<float>(cx), y = static_cast<float>(cy);
    const float th = std::max(1.5f, s / 9.0f);
    auto P = [&](float u, float v) { return SDL_FPoint{x + u * s, y + v * s}; };

    switch (i) {
        case Icon::Check: {
            const SDL_FPoint p[3] = {P(-0.30f, 0.02f), P(-0.09f, 0.22f), P(0.31f, -0.20f)};
            polyline(p, 3, std::max(2.0f, s / 7.0f), c);
            break;
        }
        case Icon::Sparkle: {
            SDL_FPoint p[8];
            for (int k = 0; k < 8; k++) {
                const float a = -PI / 2 + k * PI / 4;
                const float rr = (k % 2 == 0) ? 0.5f : 0.14f;
                p[k] = P(std::cos(a) * rr, std::sin(a) * rr);
            }
            fan(p, 8, SDL_FPoint{x, y}, c);
            break;
        }
        case Icon::Alpha:
            textCenter("α", cx, cy, c, font(std::max(8, static_cast<int>(s * 0.8f)), true));
            break;
        case Icon::Gear: {
            ring(cx, cy, static_cast<int>(s * 0.30f), std::max(2, static_cast<int>(s * 0.11f)), c);
            for (int k = 0; k < 8; k++) {
                const float a = k * PI / 4;
                line(x + std::cos(a) * s * 0.30f, y + std::sin(a) * s * 0.30f,
                     x + std::cos(a) * s * 0.46f, y + std::sin(a) * s * 0.46f, s * 0.14f, c);
            }
            disc(cx, cy, std::max(1, static_cast<int>(s * 0.09f)), c);
            break;
        }
        case Icon::Info:
            ring(cx, cy, static_cast<int>(s * 0.45f), std::max(1, static_cast<int>(th * 0.8f)), c);
            disc(cx, static_cast<int>(y - s * 0.2f), std::max(1, static_cast<int>(s * 0.06f)), c);
            line(x, y - s * 0.04f, x, y + s * 0.24f, th, c);
            break;
        case Icon::List:
            for (int k = -1; k <= 1; k++) {
                const float v = k * 0.26f;
                disc(static_cast<int>(x - s * 0.36f), static_cast<int>(y + v * s), std::max(1, static_cast<int>(s * 0.06f)), c);
                line(x - s * 0.18f, y + v * s, x + s * 0.42f, y + v * s, th, c);
            }
            break;
        case Icon::Image: {
            const int b = static_cast<int>(s * 0.8f);
            strokeRounded(cx - b / 2, cy - b / 2, b, b, std::max(2, b / 6), std::max(1, static_cast<int>(th * 0.8f)), c);
            disc(static_cast<int>(x - s * 0.14f), static_cast<int>(y - s * 0.12f), std::max(1, static_cast<int>(s * 0.08f)), c);
            const SDL_FPoint p[3] = {P(-0.34f, 0.30f), P(0.06f, -0.02f), P(0.36f, 0.24f)};
            polyline(p, 3, th * 0.8f, c);
            break;
        }
        case Icon::Pin: {
            const int rr = static_cast<int>(s * 0.26f);
            ring(cx, static_cast<int>(y - s * 0.12f), rr, std::max(1, static_cast<int>(th * 0.8f)), c);
            disc(cx, static_cast<int>(y - s * 0.12f), std::max(1, static_cast<int>(s * 0.08f)), c);
            const SDL_FPoint p[3] = {P(-0.22f, 0.02f), P(0.0f, 0.42f), P(0.22f, 0.02f)};
            polyline(p, 3, th * 0.8f, c);
            break;
        }
        case Icon::Chevron: {
            const SDL_FPoint p[3] = {P(-0.12f, -0.26f), P(0.14f, 0.0f), P(-0.12f, 0.26f)};
            polyline(p, 3, th, c);
            break;
        }
        case Icon::ChevronLeft: {
            const SDL_FPoint p[3] = {P(0.12f, -0.26f), P(-0.14f, 0.0f), P(0.12f, 0.26f)};
            polyline(p, 3, th, c);
            break;
        }
        case Icon::ChevronUp: {
            const SDL_FPoint p[3] = {P(-0.26f, 0.12f), P(0.0f, -0.14f), P(0.26f, 0.12f)};
            polyline(p, 3, th, c);
            break;
        }
        case Icon::ChevronDown: {
            const SDL_FPoint p[3] = {P(-0.26f, -0.12f), P(0.0f, 0.14f), P(0.26f, -0.12f)};
            polyline(p, 3, th, c);
            break;
        }
        case Icon::Download: {
            line(x, y - s * 0.36f, x, y + s * 0.12f, th, c);
            const SDL_FPoint p[3] = {P(-0.22f, -0.08f), P(0.0f, 0.14f), P(0.22f, -0.08f)};
            polyline(p, 3, th, c);
            line(x - s * 0.36f, y + s * 0.36f, x + s * 0.36f, y + s * 0.36f, th, c);
            break;
        }
        case Icon::Warning: {
            const SDL_FPoint p[3] = {P(0.0f, -0.40f), P(0.44f, 0.36f), P(-0.44f, 0.36f)};
            polyline(p, 3, th, c, true);
            line(x, y - s * 0.12f, x, y + s * 0.12f, th, c);
            disc(cx, static_cast<int>(y + s * 0.24f), std::max(1, static_cast<int>(th * 0.6f)), c);
            break;
        }
        case Icon::Cross:
            line(x - s * 0.26f, y - s * 0.26f, x + s * 0.26f, y + s * 0.26f, th, c);
            line(x + s * 0.26f, y - s * 0.26f, x - s * 0.26f, y + s * 0.26f, th, c);
            break;
        case Icon::Logo:
            // The pkDex mark: a lens with a pupil and a glint, drawn in `c`.
            ring(cx, cy, static_cast<int>(s * 0.32f), std::max(2, static_cast<int>(s * 0.11f)), c);
            disc(cx, cy, std::max(2, static_cast<int>(s * 0.11f)), c);
            disc(static_cast<int>(x + s * 0.34f), static_cast<int>(y - s * 0.34f), std::max(1, static_cast<int>(s * 0.07f)), c);
            break;
        case Icon::Refresh:
            arc(cx, cy, static_cast<int>(s * 0.34f), -PI * 0.35f, PI * 1.35f, th, c);
            {
                const float a = -PI * 0.35f;
                const SDL_FPoint tip = P(std::cos(a) * 0.34f, std::sin(a) * 0.34f);
                line(tip.x, tip.y, tip.x - s * 0.18f, tip.y - s * 0.02f, th, c);
                line(tip.x, tip.y, tip.x + s * 0.02f, tip.y - s * 0.18f, th, c);
            }
            break;
    }
}

// --- images ------------------------------------------------------------------------

SDL_Texture* Gfx::image(const std::string& path, int* w, int* h) {
    auto it = images_.find(path);
    if (it == images_.end()) {
        // Room first: the entry least recently drawn goes, never one drawn now.
        if (images_.size() >= 200) {
            auto oldest = images_.end();
            for (auto i = images_.begin(); i != images_.end(); ++i)
                if (i->second.used < frame_ && (oldest == images_.end() || i->second.used < oldest->second.used))
                    oldest = i;
            if (oldest != images_.end()) {
                if (oldest->second.tex) SDL_DestroyTexture(oldest->second.tex);
                images_.erase(oldest);
            }
        }
        ImageEntry e;
        if (SDL_Surface* s = IMG_Load(path.c_str())) {
            e.tex = SDL_CreateTextureFromSurface(r_, s);
            e.w = s->w;
            e.h = s->h;
            SDL_FreeSurface(s);
            if (e.tex) {
                SDL_SetTextureBlendMode(e.tex, SDL_BLENDMODE_BLEND);
                if (e.w <= 96) SDL_SetTextureScaleMode(e.tex, SDL_ScaleModeNearest);
            }
        }
        it = images_.emplace(path, e).first;
    }
    it->second.used = frame_;
    if (w) *w = it->second.w;
    if (h) *h = it->second.h;
    return it->second.tex;
}

void Gfx::blit(SDL_Texture* t, int x, int y, int w, int h, Uint8 alpha) {
    if (!t) return;
    SDL_SetTextureAlphaMod(t, alpha);
    SDL_Rect dst{x, y, w, h};
    SDL_RenderCopy(r_, t, nullptr, &dst);
}

void Gfx::blitFit(SDL_Texture* t, int cx, int cy, int box, Uint8 alpha) {
    if (!t) return;
    int w = 0, h = 0;
    SDL_QueryTexture(t, nullptr, nullptr, &w, &h);
    if (w <= 0 || h <= 0) return;
    const float k = std::min(static_cast<float>(box) / w, static_cast<float>(box) / h);
    const int dw = static_cast<int>(w * k), dh = static_cast<int>(h * k);
    blit(t, cx - dw / 2, cy - dh / 2, dw, dh, alpha);
}

SDL_Texture* Gfx::pinned(const std::string& path) {
    auto it = pinned_.find(path);
    if (it != pinned_.end()) return it->second;
    SDL_Texture* t = nullptr;
    if (SDL_Surface* s = IMG_Load(path.c_str())) {
        t = SDL_CreateTextureFromSurface(r_, s);
        SDL_FreeSurface(s);
        if (t) SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    }
    pinned_[path] = t;
    return t;
}

void Gfx::forgetImages() {
    for (auto& [k, e] : images_) if (e.tex) SDL_DestroyTexture(e.tex);
    images_.clear();
}
