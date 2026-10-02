// The regional Pokédex grid and the Pokémon detail page
// ("Dex — Kanto" and "Pokémon detail").

#include "app.h"
#include "i18n.h"
#include "theme.h"
#include "util.h"

#include <algorithm>
#include <cstdio>

using namespace layout;

namespace {

constexpr int COLS     = 3;
constexpr int ROWS     = 5;     // visible at once
constexpr int PAGE     = 30;    // L/R jump, and the "001 - 030" range
constexpr int CARD_W   = 296;
constexpr int CARD_H   = 84;
constexpr int GAP      = 12;
constexpr int GRID_Y   = 176;

const SDL_Color WHITE{255, 255, 255, 255};

std::string pad3(int n) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%03d", n);
    return buf;
}

SDL_Color primaryColor(const Pokemon& p) {
    return p.typesEn.empty() ? col::knobOff : typeColor(p.typesEn[0]);
}

SDL_Color typeColorAt(const Pokemon& p, size_t i) {
    return i < p.typesEn.size() ? typeColor(p.typesEn[i]) : col::knobOff;
}

} // anonymous namespace

// --- the grid ----------------------------------------------------------------------

void App::ensureVisible() {
    DexState& st = dexStates[region];
    const int n = static_cast<int>(dex::list(region).size());
    const int row = st.cursor / COLS;
    const int lastRow = n > 0 ? (n - 1) / COLS : 0;
    if (row < st.scroll) st.scroll = row;
    if (row >= st.scroll + ROWS) st.scroll = row - ROWS + 1;
    st.scroll = std::clamp(st.scroll, 0, std::max(0, lastRow - ROWS + 1));
}

void App::drawDex() {
    const int r = region;
    const auto& list = dex::list(r);
    DexState& st = dexStates[r];
    const int n = static_cast<int>(list.size());
    const tracker::Counts& c = counts(r);

    // Header: region and game on the left, the progress on the right.
    drawEyebrow(tr("dex/eyebrow"), CONTENT_X, 27, col::accent);
    Font* fTitle = gfx.font(40, true);
    gfx.text(gfx.fit(dex::name(r), fTitle, 560), CONTENT_X, 40, col::text, fTitle);
    Font* fGame = gfx.font(15);
    gfx.text(gfx.fit(dex::game(r), fGame, 560), CONTENT_X, 94, col::textDim, fGame);

    Font* fBig = gfx.font(32, true);
    Font* fOf  = gfx.font(14);
    const std::string of = " / " + std::to_string(n) + " " + tr("dex/caught");
    const int ofW = gfx.textRight(of, CONTENT_R, 62, col::textDim, fOf);
    gfx.textRight(std::to_string(c.any), CONTENT_R - ofW - 2, 56, col::text, fBig);

    const int barX = 960, barW = CONTENT_R - barX;
    gfx.fillRounded(barX, 79, barW, 6, 3, col::chip);
    if (n > 0 && c.any > 0) gfx.fillRounded(barX, 79, std::max(6, barW * c.any / n), 6, 3, col::green);

    {
        // The legend: one count per capture state.
        Font* f = gfx.font(12);
        const std::vector<int> states = statesOf(r);
        std::vector<std::string> labels;
        int total = 0;
        for (int s : states) {
            labels.push_back(std::to_string(c.state[s]) + " " + stateLabel(s));
            total += (total ? 16 : 0) + 12 + 6 + gfx.textW(labels.back(), f);
        }
        int x = CONTENT_R - total;
        for (size_t i = 0; i < states.size(); i++) {
            drawStateIcon(states[i], x + 6, 104, 12);
            x += 18;
            x += gfx.textMid(labels[i], x, 104, col::textDim, f) + 16;
        }
    }

    // The range of the page the cursor is on, and the page dots.
    const int pages = std::max(1, (n + PAGE - 1) / PAGE);
    const int pageIdx = n > 0 ? st.cursor / PAGE : 0;
    {
        Font* fRange = gfx.font(15, true);
        Font* fPage  = gfx.font(13);
        const std::string range = pad3(n ? pageIdx * PAGE + 1 : 0) + " - " + pad3(std::min(n, (pageIdx + 1) * PAGE));
        int x = CONTENT_X;
        x += gfx.textMid(range, x, 146, col::text, fRange) + 10;
        x += gfx.textMid(trf("dex/page", {{"page", std::to_string(pageIdx + 1)}, {"pages", std::to_string(pages)}}),
                         x, 146, col::textDim, fPage) + 16;
        if (st.multi) {
            Font* fChip = gfx.font(13, true);
            const std::string chip = trf("dex/multi", {{"count", std::to_string(st.selected.size())}});
            const int w = gfx.textW(chip, fChip) + 34;
            gfx.fillRounded(x, 133, w, 26, 13, mix(col::bg, col::accent, 0.2f));
            gfx.disc(x + 13, 146, 4, col::accent);
            gfx.textMid(chip, x + 23, 146, mix(col::accent, WHITE, 0.3f), fChip);
        }

        const int dotsW = 20 + (pages - 1) * 12;
        const int rKey = CONTENT_R - 22;
        const int dotsX = rKey - 12 - dotsW;
        drawKey("L", dotsX - 12 - 22, 146, true);
        drawKey("R", rKey, 146, true);
        int dx = dotsX;
        for (int p = 0; p < pages; p++) {
            if (p == pageIdx) { gfx.fillRounded(dx, 143, 20, 6, 3, col::text); dx += 26; }
            else              { gfx.disc(dx + 3, 146, 3, col::dot); dx += 12; }
        }
    }

    if (n == 0) {
        gfx.textCenter(tr("dex/empty"), CONTENT_X + (CONTENT_R - CONTENT_X) / 2, 400, col::textDim, gfx.font(16));
    }

    Font* fNum  = gfx.font(12);
    Font* fName = gfx.font(17, true);
    Font* fType = gfx.font(12);
    const std::vector<int> states = statesOf(r);
    const char* regionId = dex::regions()[r].id;
    for (int row = st.scroll; row < st.scroll + ROWS; row++) {
        for (int cI = 0; cI < COLS; cI++) {
            const int i = row * COLS + cI;
            if (i >= n) break;
            const Pokemon& p = list[i];
            const int x = CONTENT_X + cI * (CARD_W + GAP);
            const int y = GRID_Y + (row - st.scroll) * (CARD_H + GAP);
            const bool focused = !sidebarFocus && i == st.cursor;
            const bool picked = st.multi && st.selected.count(i) > 0;

            if (focused) drawFocusRing(x, y, CARD_W, CARD_H, 12);
            gfx.fillRounded(x, y, CARD_W, CARD_H, 12, focused ? col::cardFocus : col::card);
            if (!focused)
                gfx.strokeRounded(x, y, CARD_W, CARD_H, 12, picked ? 2 : 1,
                                  picked ? withAlpha(col::accent, 170) : col::cardBorder);

            drawPokemonThumb(p, x + 13, y + 14, 56, 10);
            if (st.multi) {
                const int mx = x + 13, my = y + 14;
                if (picked) {
                    gfx.disc(mx, my, 10, col::accent);
                    gfx.icon(Icon::Check, mx, my, 13, WHITE);
                } else {
                    gfx.disc(mx, my, 10, col::card);
                    gfx.ring(mx, my, 10, 2, col::textMuted);
                }
            }

            const int tx = x + 87;
            const Capture cap = tracker::get(regionId, p.regional);
            int iconsW = 0;
            for (auto it = states.rbegin(); it != states.rend(); ++it) {
                if (!cap.get(*it)) continue;
                drawStateIcon(*it, x + CARD_W - 22 - iconsW, y + 21, 16);
                iconsW += 22;
            }
            gfx.textMid("#" + p.regional, tx, y + 22, col::textMuted, fNum);
            gfx.textMid(gfx.fit(p.name, fName, x + CARD_W - 14 - tx), tx, y + 42, col::text, fName);
            int ty = tx;
            for (size_t t = 0; t < p.types.size() && t < 2; t++) {
                gfx.disc(ty + 3, y + 63, 3, typeColorAt(p, t));
                ty += 10;
                ty += gfx.textMid(p.types[t], ty, y + 63, col::textDim, fType) + 12;
            }
        }
    }

    // Where the visible rows sit in the whole list.
    const int rowsTotal = (n + COLS - 1) / COLS;
    if (rowsTotal > ROWS) {
        const int trackY = GRID_Y, trackH = ROWS * (CARD_H + GAP) - GAP;
        const int thumbH = std::max(24, trackH * ROWS / rowsTotal);
        const int thumbY = trackY + (trackH - thumbH) * st.scroll / std::max(1, rowsTotal - ROWS);
        gfx.fillRounded(CONTENT_R + 14, trackY, 3, trackH, 1, col::divider);
        gfx.fillRounded(CONTENT_R + 14, thumbY, 3, thumbH, 1, col::textMuted);
    }

    if (st.multi) {
        drawFooter(SIDEBAR_W, {{"L R", tr("hints/page")}, {"Y", tr("hints/apply")}, {"ZL", tr("hints/done")},
                               {"B", tr("hints/exit")}, {"A", tr("hints/select")}});
    } else {
        drawFooter(SIDEBAR_W, {{"L R", tr("hints/page")}, {"Y", tr("hints/status")}, {"X", tr("hints/bulk")},
                               {"ZL", tr("hints/multi")}, {"B", tr("hints/back")}, {"A", tr("hints/open")}});
    }
}

void App::inputDex(uint32_t pressed) {
    DexState& st = dexStates[region];
    const int n = static_cast<int>(dex::list(region).size());
    auto toSidebar = [this]() {
        sidebarFocus = true;
        sideCursor = region;
    };
    if (n == 0) {
        if (pressed & (BTN_LEFT | BTN_B)) toSidebar();
        return;
    }

    if (pressed & BTN_UP) {
        if (st.cursor >= COLS) st.cursor -= COLS;
    } else if (pressed & BTN_DOWN) {
        if (st.cursor + COLS < n) st.cursor += COLS;
        else if (st.cursor / COLS < (n - 1) / COLS) st.cursor = n - 1;   // into a short last row
    } else if (pressed & BTN_LEFT) {
        if (st.cursor % COLS == 0) { toSidebar(); return; }
        st.cursor--;
    } else if (pressed & BTN_RIGHT) {
        if (st.cursor % COLS < COLS - 1 && st.cursor + 1 < n) st.cursor++;
    }
    if (pressed & BTN_L) st.cursor = std::max(0, st.cursor - PAGE);
    if (pressed & BTN_R) st.cursor = std::min(n - 1, st.cursor + PAGE);

    if (pressed & BTN_ZL) {
        st.multi = !st.multi;
        st.selected.clear();
        if (st.multi) toast(tr("multi/on"));
    } else if (pressed & BTN_A) {
        if (st.multi) {
            if (!st.selected.erase(st.cursor)) st.selected.insert(st.cursor);
        } else {
            page = Page::Detail;
            detailIndex = st.cursor;
            detailShiny = false;
            detailLocRow = 0;
        }
    } else if (pressed & BTN_Y) {
        if (st.multi) openMultiApply();
        else openCapture(region, st.cursor);
    } else if (pressed & BTN_X) {
        openBulk();
    } else if (pressed & BTN_B) {
        if (st.multi) { st.multi = false; st.selected.clear(); }
        else toSidebar();
    }
    ensureVisible();
}

// --- the detail page -----------------------------------------------------------------

namespace {

// A pill-shaped button at the top: a key and a label, right edge or left edge
// given. Returns its width.
int topPill(App& app, int x, int y, const std::string& key, const std::string& dim, const std::string& text,
            bool enabled, bool alignRight, int maxW = 1 << 20) {
    Font* fDim = gfx.font(14);
    Font* f = gfx.font(15);
    const int dimW = dim.empty() ? 0 : gfx.textW(dim, fDim) + 8;
    const int chrome = 14 + app.keyWidth(key) + 10 + dimW + 18;
    const std::string label = gfx.fit(text, f, std::max(40, maxW - chrome));
    const int w = chrome + gfx.textW(label, f);
    if (alignRight) x -= w;
    const int h = 44, cy = y + h / 2;
    gfx.fillRounded(x, y, w, h, h / 2, enabled ? col::card : withAlpha(col::card, 110));
    gfx.strokeRounded(x, y, w, h, h / 2, 1, enabled ? col::cardBorder : withAlpha(col::cardBorder, 110));
    int tx = x + 14 + app.drawKey(key, x + 14, cy, !enabled) + 10;
    if (!dim.empty()) tx += gfx.textMid(dim, tx, cy, enabled ? col::textMuted : col::textFaint, fDim) + 8;
    gfx.textMid(label, tx, cy, enabled ? col::text : col::textFaint, f);
    return w;
}

void statCard(int x, int y, int w, const std::string& label) {
    gfx.fillRounded(x, y, w, 78, 14, col::card);
    gfx.strokeRounded(x, y, w, 78, 14, 1, col::cardBorder);
    gfx.text(gfx.fit(label, gfx.font(13), w - 30), x + 17, y + 14, col::textDim, gfx.font(13));
}

// --- evolution drawing ---------------------------------------------------------------

struct EvoContext { int region; std::string current; SDL_Color tc; };
struct ChainStyle { int radius; Font* name; Font* cond; bool currentLabel; };

// One Pokémon of an evolution: a disc with its icon, ringed when it is the one
// on the page.
void drawEvoNode(const EvoContext& ctx, const std::string& name, int cx, int cy, int radius) {
    if (name == ctx.current) {
        gfx.disc(cx, cy, radius, mix(col::card, ctx.tc, 0.2f));
        gfx.ring(cx, cy, radius + 1, 2, ctx.tc);
    } else {
        gfx.disc(cx, cy, radius, col::chip);
    }
    const Pokemon* who = dex::findAnywhere(ctx.region, name);
    SDL_Texture* t = who ? gfx.image(dex::iconPath(*who)) : nullptr;
    if (t) gfx.blitFit(t, cx, cy, radius * 3 / 2);
    else gfx.icon(Icon::Image, cx, cy, radius * 2 / 3, col::textMuted);
}

// A row of an evolution: nodes joined by the condition that leads to each,
// with a connector before the first when `lead`. Conditions, then names, are
// shortened until the row fits in maxW, so every line draws the same way.
void drawChain(const EvoContext& ctx, const std::vector<EvoStep>& items, bool lead, int x, int cy, int maxW,
               const ChainStyle& st) {
    const int n = static_cast<int>(items.size());
    std::vector<int> nameW(n), condW(n);
    for (int i = 0; i < n; i++) {
        nameW[i] = gfx.textW(items[i].name, st.name);
        condW[i] = items[i].condition.empty() ? 0 : gfx.textW(items[i].condition, st.cond) + 16;
    }
    auto connector = [&](int i, int condCap) {
        if (i == 0 && !lead) return 0;
        return condW[i] ? 6 + 10 + std::min(condW[i], condCap) + 10 + 6 : 6 + 16 + 6;
    };
    auto width = [&](int nameCap, int condCap) {
        int w = 0;
        for (int i = 0; i < n; i++) w += connector(i, condCap) + st.radius * 2 + 8 + std::min(nameW[i], nameCap);
        return w;
    };
    int nameCap = 220, condCap = 240;
    while (width(nameCap, condCap) > maxW) {
        if (condCap > 48) condCap -= 16;
        else if (nameCap > 48) nameCap -= 12;
        else break;
    }

    for (int i = 0; i < n; i++) {
        if (i > 0 || lead) {
            x += 6;
            if (condW[i]) {
                const int pw = std::min(condW[i], condCap);
                gfx.rect(x, cy, 10, 2, col::textFaint);
                x += 10;
                gfx.fillRounded(x, cy - 11, pw, 22, 7, col::chip);
                gfx.textCenter(gfx.fit(items[i].condition, st.cond, pw - 12), x + pw / 2, cy, col::textDim, st.cond);
                x += pw;
                gfx.rect(x, cy, 10, 2, col::textFaint);
                x += 10;
            } else {
                gfx.rect(x, cy, 16, 2, col::textFaint);
                x += 16;
            }
            x += 6;
        }
        drawEvoNode(ctx, items[i].name, x + st.radius, cy, st.radius);
        const int nx = x + st.radius * 2 + 8;
        const std::string label = gfx.fit(items[i].name, st.name, nameCap);
        if (st.currentLabel && items[i].name == ctx.current) {
            gfx.textMid(label, nx, cy - 8, col::text, st.name);
            gfx.textMid(tr("detail/current"), nx, cy + 11, col::textDim, gfx.font(11));
        } else {
            gfx.textMid(label, nx, cy, col::text, st.name);
        }
        x = nx + std::min(nameW[i], nameCap);
    }
}

} // anonymous namespace

void App::drawDetail() {
    const int r = region;
    const auto& list = dex::list(r);
    const int n = static_cast<int>(list.size());
    if (n == 0) { page = Page::Dex; return; }
    detailIndex = std::clamp(detailIndex, 0, n - 1);
    const Pokemon& p = list[detailIndex];
    const Capture cap = tracker::get(dex::regions()[r].id, p.regional);
    const SDL_Color tc = primaryColor(p);

    // --- the bar at the top: next and previous on the right, then back with
    //     the full game name ("Scarlet & Violet · The Teal Mask") in what is left
    {
        int right = CONTENT_R;
        if (detailIndex + 1 < n) {
            const Pokemon& next = list[detailIndex + 1];
            right -= topPill(*this, right, 16, "R", "#" + next.regional, next.name, true, true) + 10;
        } else {
            right -= topPill(*this, right, 16, "R", "", tr("detail/next"), false, true) + 10;
        }
        if (detailIndex > 0) {
            const Pokemon& prev = list[detailIndex - 1];
            right -= topPill(*this, right, 16, "L", "#" + prev.regional, prev.name, true, true);
        } else {
            right -= topPill(*this, right, 16, "L", "", tr("detail/previous"), false, true);
        }
        topPill(*this, 40, 16, "B", "", dex::game(r), true, false, right - 16 - 40);
    }

    // --- the sprite panel
    const int px = 40, py = 76, pw = 480, ph = 568;
    const SDL_Color panel = mix(col::bg, tc, 0.07f);
    gfx.fillRounded(px, py, pw, ph, 24, panel);
    {
        const SDL_Rect inner{px + 14, py + 14, pw - 28, ph - 28};
        gfx.clip(&inner);
        gfx.dotGrid(inner, mix(panel, tc, 0.22f));
        gfx.clip(nullptr);
    }
    gfx.strokeRounded(px, py, pw, ph, 24, 1, mix(col::cardBorder, tc, 0.15f));
    gfx.text(p.regional, px + 24, py + 6, mix(panel, tc, 0.14f), gfx.font(150, true));

    {
        // What has been caught, as pills in the corner.
        static const SDL_Color colors[Capture::Count] = {col::green, col::gold, col::alpha, col::shinyAlpha};
        Font* f = gfx.font(14, true);
        int right = px + pw - 30;
        const std::vector<int> states = statesOf(r);
        for (auto it = states.rbegin(); it != states.rend(); ++it) {
            const int s = *it;
            if (!cap.get(s)) continue;
            const std::string label = s == Capture::Regular ? tr("detail/caught") : stateLabel(s);
            const int w = 14 + 16 + 8 + gfx.textW(label, f) + 14;
            const int x = right - w;
            gfx.fillRounded(x, py + 29, w, 30, 15, mix(panel, colors[s], 0.2f));
            drawStateIcon(s, x + 14 + 8, py + 44, 16);
            gfx.textMid(label, x + 14 + 16 + 8, py + 44, mix(colors[s], WHITE, 0.25f), f);
            right = x - 8;
        }
    }

    const int cx = px + pw / 2, cy = py + 273, radius = 141;
    gfx.disc(cx, cy, radius, mix(panel, col::bg, 0.5f));
    gfx.dashedRing(cx, cy, radius, mix(panel, tc, 0.3f), 96);
    if (SDL_Texture* t = gfx.image(dex::spritePath(p, detailShiny))) {
        gfx.blitFit(t, cx, cy, 250);
    } else {
        gfx.icon(Icon::Image, cx, cy - 12, 34, col::textMuted);
        gfx.textCenter(tr("detail/no_sprite"), cx, cy + 22, col::textMuted, gfx.font(13));
    }

    {
        // Regular | Shiny
        const int sx = cx - 140, sy = py + ph - 80, sw = 280, sh = 52;
        gfx.fillRounded(sx, sy, sw, sh, sh / 2, SDL_Color{0, 0, 0, 90});
        const int half = (sw - 8) / 2;
        gfx.fillRounded(sx + 4 + (detailShiny ? half : 0), sy + 4, half, sh - 8, (sh - 8) / 2, col::text);
        Font* f = gfx.font(16, true);
        gfx.textCenter(stateLabel(Capture::Regular), sx + 4 + half / 2, sy + sh / 2,
                       detailShiny ? col::textDim : col::bg, f);
        const std::string shiny = stateLabel(Capture::Shiny);
        const int shinyW = gfx.textW(shiny, f) + 20;
        const int shinyX = sx + 4 + half + (half - shinyW) / 2;
        drawStateIcon(Capture::Shiny, shinyX + 6, sy + sh / 2, 14);
        gfx.textMid(shiny, shinyX + 20, sy + sh / 2, detailShiny ? col::bg : col::textDim, f);
    }

    // --- the facts
    const int x0 = 568, w0 = CONTENT_R - x0;
    gfx.text(trf("detail/numbers", {{"national", p.id}, {"region", dex::name(r)}, {"regional", p.regional}}),
             x0, 84, col::textDim, gfx.font(13));
    Font* fName = gfx.font(64, true);
    gfx.text(gfx.fit(p.name, fName, w0), x0 - 2, 100, col::text, fName);

    {
        Font* f = gfx.font(15, true);
        int x = x0;
        for (size_t i = 0; i < p.types.size(); i++) {
            const SDL_Color c = typeColorAt(p, i);
            const int w = 28 + gfx.textW(p.types[i], f) + 16;
            gfx.fillRounded(x, 187, w, 34, 17, mix(col::bg, c, 0.2f));
            gfx.disc(x + 16, 204, 4, c);
            gfx.textMid(p.types[i], x + 28, 204, mix(c, WHITE, 0.35f), f);
            x += w + 8;
        }
    }

    {
        const int cw = 161, gap = 9, y = 241;
        Font* fNum = gfx.font(24, true);
        Font* fVal = gfx.font(18, true);
        statCard(x0, y, cw, tr("detail/national_no"));
        gfx.textMid(p.id, x0 + 17, y + 51, col::text, fNum);
        statCard(x0 + (cw + gap), y, cw, tr("detail/regional_no"));
        gfx.textMid(p.regional, x0 + (cw + gap) + 17, y + 51, col::text, fNum);

        const int sx = x0 + 2 * (cw + gap);
        statCard(sx, y, cw, tr("detail/shiny"));
        const SDL_Color sc = p.shinyLocked ? col::danger : col::green;
        gfx.disc(sx + 21, y + 51, 4, sc);
        gfx.textMid(gfx.fit(tr(p.shinyLocked ? "detail/locked" : "detail/available"), fVal, cw - 50), sx + 32, y + 51,
                    mix(sc, WHITE, 0.15f), fVal);

        const int vx = x0 + 3 * (cw + gap);
        statCard(vx, y, cw, tr("detail/version"));
        const std::string ex = util::trim(p.exclusive);
        const bool none = ex.empty() || ex == "-";
        gfx.textMid(gfx.fit(none ? tr("detail/none") : ex, fVal, cw - 34), vx + 17, y + 51,
                    none ? col::textDim : col::text, fVal);
    }

    // --- evolution: always as nodes - one row for a single line, the root
    //     and one row per branch otherwise (two columns past four, for Eevee)
    drawEyebrow(tr("detail/evolution"), x0, 343, col::textDim);
    const int ey = 367;
    int eh = 86;
    {
        EvoTree tree;
        const bool drawn = parseEvolution(p.evolution, tree);
        const EvoContext ctx{r, p.name, tc};
        const int nb = drawn ? static_cast<int>(tree.branches.size()) : 0;
        const int cols = nb > 4 ? 2 : 1;
        const int rows = nb > 1 ? (nb + cols - 1) / cols : 1;
        constexpr int PITCH = 40;
        if (nb > 1) eh = std::max(86, rows * PITCH + 20);
        const int ecy = ey + eh / 2;
        gfx.fillRounded(x0, ey, w0, eh, 14, col::card);
        gfx.strokeRounded(x0, ey, w0, eh, 14, 1, col::cardBorder);

        if (!drawn) {
            const std::string evo = util::trim(p.evolution);
            const bool none = evo.empty() || evo == "-";
            Font* f = gfx.font(15);
            const auto lines = gfx.wrap(none ? tr("detail/no_evolution") : evo, f, w0 - 40, 3);
            int ly = ecy - static_cast<int>(lines.size()) * 22 / 2;
            for (const auto& l : lines) {
                gfx.text(l, x0 + 20, ly, none ? col::textDim : col::text, f);
                ly += 22;
            }
        } else if (nb == 1) {
            std::vector<EvoStep> items = {{tree.root, std::string()}};
            items.insert(items.end(), tree.branches[0].steps.begin(), tree.branches[0].steps.end());
            const ChainStyle big{26, gfx.font(15, true), gfx.font(12, true), true};
            drawChain(ctx, items, false, x0 + 20, ecy, w0 - 40, big);
        } else {
            // The root on the left, its name under it; a spine per column.
            Font* fRoot = gfx.font(12, true);
            const int rootW = std::max(48, std::min(96, gfx.textW(tree.root, fRoot)));
            const int rootCx = x0 + 20 + rootW / 2;
            drawEvoNode(ctx, tree.root, rootCx, ecy - 8, 22);
            gfx.textCenter(gfx.fit(tree.root, fRoot, 96), rootCx, ecy + 26, col::text, fRoot);

            const int spine0 = x0 + 20 + rootW + 14;
            const int colW = (x0 + w0 - 20 - spine0 - (cols - 1) * 16) / cols;
            const int top = ey + (eh - rows * PITCH) / 2 + PITCH / 2;
            gfx.rect(rootCx + 22, ecy - 9, spine0 - rootCx - 22, 2, col::textFaint);
            const ChainStyle small{16, gfx.font(14, true), gfx.font(11, true), false};
            for (int c = 0; c < cols; c++) {
                const int sx = spine0 + c * (colW + 16);
                const int inCol = std::min(rows, nb - c * rows);
                if (inCol <= 0) continue;
                const int firstCy = top, lastCy = top + (inCol - 1) * PITCH;
                const int fromY = c == 0 ? std::min(firstCy, ecy - 9) : firstCy;
                const int toY = c == 0 ? std::max(lastCy, ecy - 9) : lastCy;
                gfx.rect(sx, fromY, 2, toY - fromY + 2, col::textFaint);
                if (c > 0) gfx.rect(spine0, firstCy, sx - spine0, 2, col::textFaint);
                for (int i = 0; i < inCol; i++) {
                    const EvoBranch& br = tree.branches[c * rows + i];
                    std::vector<EvoStep> items;
                    if (br.from != tree.root) items.push_back({br.from, std::string()});
                    items.insert(items.end(), br.steps.begin(), br.steps.end());
                    drawChain(ctx, items, true, sx + 2, top + i * PITCH, colW - 2, small);
                }
            }
        }
    }

    // --- locations, as pills that wrap under the evolution card; Up/Down
    //     scroll them when there are more rows than fit (Paldea lists a dozen
    //     places for some Pokémon)
    const int locY = ey + eh + 30;
    drawEyebrow(tr("detail/locations"), x0, locY, col::textDim);
    {
        const std::vector<std::string> places = splitLocations(p.locations);
        Font* f = gfx.font(15);
        if (places.empty()) gfx.text(tr("detail/unknown_location"), x0, locY + 27, col::textDim, f);

        struct Pill { std::string label; int w; };
        std::vector<std::vector<Pill>> rows;
        int x = x0;
        for (const std::string& place : places) {
            Pill pill{gfx.fit(place, f, w0 - 52), 0};
            pill.w = 36 + gfx.textW(pill.label, f) + 16;
            if (rows.empty() || (x + pill.w > CONTENT_R && x > x0)) { rows.emplace_back(); x = x0; }
            rows.back().push_back(pill);
            x += pill.w + 10;
        }

        constexpr int H = 40;
        const int pillsY = locY + 18;
        const int visible = std::max(1, (644 - pillsY + 10) / (H + 10));
        const int total = static_cast<int>(rows.size());
        detailLocRow = std::clamp(detailLocRow, 0, std::max(0, total - visible));
        if (total > visible) {
            const int last = std::min(total, detailLocRow + visible);
            Font* fs = gfx.font(12);
            const int iy = locY + 6;
            const int w = gfx.textRight(trf("detail/rows", {{"first", std::to_string(detailLocRow + 1)},
                                                           {"last", std::to_string(last)},
                                                           {"total", std::to_string(total)}}),
                                        CONTENT_R, iy, col::textMuted, fs);
            gfx.icon(Icon::ChevronDown, CONTENT_R - w - 12, iy, 12, last < total ? col::text : col::textFaint);
            gfx.icon(Icon::ChevronUp, CONTENT_R - w - 28, iy, 12, detailLocRow > 0 ? col::text : col::textFaint);
        }
        for (int r2 = detailLocRow; r2 < total && r2 < detailLocRow + visible; r2++) {
            const int y = pillsY + (r2 - detailLocRow) * (H + 10);
            int px2 = x0;
            for (const Pill& pill : rows[r2]) {
                gfx.fillRounded(px2, y, pill.w, H, 12, col::card);
                gfx.strokeRounded(px2, y, pill.w, H, 12, 1, col::cardBorder);
                gfx.icon(Icon::Pin, px2 + 20, y + H / 2, 15, col::accent);
                gfx.textMid(pill.label, px2 + 36, y + H / 2, col::text, f);
                px2 += pill.w + 10;
            }
        }
    }

    drawFooter(0, {{"L", tr("hints/previous")}, {"R", tr("hints/next")}, {"Y", tr("hints/status")},
                   {"X", tr(detailShiny ? "hints/regular_view" : "hints/shiny_view")}, {"B", tr("hints/back")}});
}

void App::inputDetail(uint32_t pressed) {
    const int n = static_cast<int>(dex::list(region).size());
    const int before = detailIndex;
    if (pressed & (BTN_L | BTN_LEFT)) {
        if (detailIndex > 0) detailIndex--;
    } else if (pressed & (BTN_R | BTN_RIGHT)) {
        if (detailIndex + 1 < n) detailIndex++;
    }
    if (detailIndex != before) detailLocRow = 0;
    if (pressed & BTN_UP) detailLocRow = std::max(0, detailLocRow - 1);
    if (pressed & BTN_DOWN) detailLocRow++;   // clamped when drawn
    if (pressed & BTN_Y) openCapture(region, detailIndex);
    if (pressed & BTN_X) detailShiny = !detailShiny;
    if (pressed & BTN_B) {
        page = Page::Dex;
        dexStates[region].cursor = detailIndex;
        ensureVisible();
    }
}
