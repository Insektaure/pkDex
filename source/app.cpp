// The main loop, the sidebar, the footer and what every screen shares.

#include "app.h"
#include "config.h"
#include "i18n.h"
#include "net.h"
#include "pack.h"
#include "theme.h"
#include "util.h"

#include <switch.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>

using namespace layout;

namespace {

// Startup failed before anything could be drawn: say so on the libnx text
// console, which needs no font, and wait for +.
void showStartupError(const std::string& reason) {
    consoleInit(nullptr);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);
    std::printf("\n  pkDex could not start.\n\n  %s\n\n  Press + to exit.\n",
                reason.empty() ? "The display could not be initialised." : reason.c_str());
    consoleUpdate(nullptr);
    while (appletMainLoop()) {
        padUpdate(&pad);
        if (padGetButtonsDown(&pad) & HidNpadButton_Plus) break;
        consoleUpdate(nullptr);
    }
    consoleExit(nullptr);
}

const char* const LOGO_PATH = "romfs:/img/pkdex_256.png";

const char* stateKey(int s) {
    switch (s) {
        case Capture::Shiny:      return "shiny";
        case Capture::Alpha:      return "alpha";
        case Capture::ShinyAlpha: return "shiny_alpha";
        default:                  return "regular";
    }
}

// A dex by its game in the reset texts ("FireRed & LeafGreen"), where the two
// Kantos must not read alike.
std::string regionLabel(int r) { return dex::sidebar(r); }

} // anonymous namespace

// --- lifetime --------------------------------------------------------------------

int App::run(int argc, char** argv) {
    exePath = (argc > 0 && argv[0] && argv[0][0]) ? argv[0] : "sdmc:/switch/pkDex/pkDex.nro";

    config::load();
    std::string locale = config::getString(config::LOCALE, "");
    if (locale.empty()) locale = i18n::systemLocale();
    i18n::init(locale);

    std::string error;
    if (!gfx.init(error)) {
        gfx.shutdown();
        showStartupError(error);
        return 1;
    }
    input_.init();
    // Now, while the RomFS is surely mounted.
    gfx.pinned(LOGO_PATH);
    for (const char* p : {"romfs:/img/states/shiny.png", "romfs:/img/states/alpha.png", "romfs:/img/states/shiny_alpha.png"})
        gfx.pinned(p);

    dexStates.assign(dex::count(), DexState{});
    loadData();
    Update::cleanupLegacy(exePath);

    if (config::getBool(config::CHECK_ON_LAUNCH, true))
        checkForUpdates(false);

    while (!quit) {
        if (!input_.poll()) break;
        handleInput(input_.pressed());
        watchUpdate();
        for (size_t i = 0; i < modals_.size(); i++) modals_[i]->update(*this);
        modals_.erase(std::remove_if(modals_.begin(), modals_.end(),
                                     [](const std::unique_ptr<Modal>& m) { return m->closed; }),
                      modals_.end());
        frame();
    }

    if (job) job->cancel = true;   // a download stops; an extraction runs out
    worker.join();
    Update::shutdown();
    net::stop();
    modals_.clear();
    input_.shutdown();
    gfx.shutdown();
    return 0;
}

void App::loadData() {
    drawLoading(0.0f);
    dex::load([this](float p) { drawLoading(p); });
    countsVersion_ = 0;   // recounted on next use
    for (int r = 0; r < dex::count(); r++) {
        DexState& s = dexStates[r];
        const int n = static_cast<int>(dex::list(r).size());
        s.cursor = std::clamp(s.cursor, 0, std::max(0, n - 1));
    }
}

void App::drawLoading(float progress) {
    gfx.beginFrame(col::bg);
    gfx.dotGrid(SDL_Rect{0, 0, SCREEN_W, SCREEN_H}, col::dotGrid);
    const int cx = SCREEN_W / 2;
    drawLogo(cx - 36, 260, 72);
    gfx.textCenter("pkDex", cx, 372, col::text, gfx.font(30, true));
    gfx.textCenter(tr("common/loading"), cx, 410, col::textDim, gfx.font(15));
    const int w = 240, x = cx - w / 2, y = 440;
    gfx.fillRounded(x, y, w, 6, 3, col::chip);
    gfx.fillRounded(x, y, std::max(6, static_cast<int>(w * std::clamp(progress, 0.0f, 1.0f))), 6, 3, col::accent);
    gfx.present();
}

void App::confirmQuit() {
    auto c = std::make_unique<ConfirmModal>();
    c->kind = ConfirmModal::Info;
    c->title = tr("quit/title");
    // A running download stops with the app; an extraction runs out first.
    if (worker.busy()) c->body = tr("quit/busy");
    c->cancelLabel = tr("common/cancel");
    c->confirmLabel = tr("hints/quit");
    c->focus = 1;
    c->onConfirm = [](App& app) { app.quit = true; };
    push(std::move(c));
}

void App::push(std::unique_ptr<Modal> m) { modals_.push_back(std::move(m)); }

void App::toast(const std::string& text, bool error) { notify(text, std::string(), error); }

void App::notify(const std::string& title, const std::string& body, bool error) {
    toast_ = Toast{title, body, error, SDL_GetTicks()};
    toastShown_ = true;
}

// --- the frame ---------------------------------------------------------------------

void App::handleInput(uint32_t pressed) {
    if (!pressed) return;
    if (!modals_.empty()) {
        // By pointer: the input may push another popup, which can move the
        // vector's storage but never the popup itself.
        Modal* top = modals_.back().get();
        top->input(*this, pressed);
        return;
    }
    if (pressed & BTN_PLUS) { confirmQuit(); return; }
    if (page == Page::Detail) { inputDetail(pressed); return; }
    if (sidebarFocus) { inputSidebar(pressed); return; }
    switch (page) {
        case Page::Dex:       inputDex(pressed); break;
        case Page::Settings:  inputSettings(pressed); break;
        case Page::Changelog: inputChangelog(pressed); break;
        case Page::About:
            if (pressed & (BTN_B | BTN_LEFT)) sidebarFocus = true;
            break;
        default: break;
    }
}

void App::frame() {
    gfx.beginFrame(col::bg);

    // Only what the last full-screen popup leaves visible is drawn.
    size_t first = 0;
    for (size_t i = 0; i < modals_.size(); i++)
        if (modals_[i]->fullscreen()) first = i;
    const bool covered = !modals_.empty() && modals_[first]->fullscreen();

    if (!covered) {
        if (page == Page::Detail) {
            drawDetail();
        } else {
            switch (page) {
                case Page::Dex:       drawDex(); break;
                case Page::Settings:  drawSettings(); break;
                case Page::About:     drawAbout(); break;
                case Page::Changelog: drawChangelog(); break;
                default: break;
            }
            drawSidebar();
        }
    }
    for (size_t i = covered ? first : 0; i < modals_.size(); i++) modals_[i]->draw(*this);
    drawToasts();
    gfx.present();
}

void App::watchUpdate() {
    const Update::State s = Update::state();
    if (s == lastUpdateState_) return;
    lastUpdateState_ = s;
    if (s == Update::State::Available) {
        if (manualCheck_) {
            offerUpdate();
        } else if (!updateToastShown_) {
            toast(trf("update/toast_available", {{"version", Update::latestVersion()}}));
            updateToastShown_ = true;
        }
    } else if (manualCheck_ && s == Update::State::UpToDate) {
        toast(trf("update/latest", {{"version", APP_VERSION}}));
    } else if (manualCheck_ && s == Update::State::Failed) {
        notify(tr("update/check_failed"), Update::lastError(), true);
    }
    if (s != Update::State::Checking) manualCheck_ = false;
}

void App::drawToasts() {
    if (!toastShown_) return;
    // nx-plaza's notice, at this screen's scale: top right, 480 wide, slides
    // down into place over 0.28 s, stays, and fades out over its last 0.6 s.
    constexpr uint32_t LIFE = 6000, APPEAR = 280, FADE = 600;
    const uint32_t age = SDL_GetTicks() - toast_.start;
    if (age >= LIFE) { toastShown_ = false; return; }
    const float appear = std::min(1.0f, static_cast<float>(age) / APPEAR);
    const float fade = std::min(1.0f, static_cast<float>(LIFE - age) / FADE);
    const float alpha = appear * fade;
    auto a = [alpha](SDL_Color c, float k = 1.0f) {
        return withAlpha(c, static_cast<Uint8>(c.a * k * alpha + 0.5f));
    };

    constexpr int W = 480, PAD = 16, EDGE = 43, TOP = 32;
    Font* fEyebrow = gfx.font(11, true);
    Font* fTitle = gfx.font(20, true);
    Font* fBody = gfx.font(14);
    const int textW = W - 2 * PAD;
    const auto titleLines = gfx.wrap(toast_.title, fTitle, textW, 2);
    const auto bodyLines = toast_.body.empty() ? std::vector<std::string>() : gfx.wrap(toast_.body, fBody, textW, 2);
    const int eyebrowH = 16;
    const int h = PAD + eyebrowH + 6 + static_cast<int>(titleLines.size()) * 26 +
                  (bodyLines.empty() ? 0 : 6 + static_cast<int>(bodyLines.size()) * 20) + PAD;
    const int x = SCREEN_W - EDGE - W;
    const int y = TOP - static_cast<int>((1.0f - appear) * 16.0f);

    gfx.fillRounded(x - 4, y - 2, W + 8, h + 10, 16, SDL_Color{0, 0, 0, static_cast<Uint8>(90 * alpha)});
    gfx.fillRounded(x, y, W, h, 12, a(col::modal, 0.92f));
    gfx.strokeRounded(x, y, W, h, 12, 1, a(col::modalBorder));

    // The eyebrow: a dot (red for a failure) and the app's name.
    const int ey = y + PAD + eyebrowH / 2;
    gfx.disc(x + PAD + 7, ey, 7, a(toast_.error ? col::accent : col::green));
    gfx.tracked("PKDEX", x + PAD + 14 + 8, ey - fEyebrow->height / 2, a(col::accent), fEyebrow, 2);

    int ty = y + PAD + eyebrowH + 6;
    for (const auto& l : titleLines) { gfx.text(l, x + PAD, ty, a(col::text), fTitle); ty += 26; }
    if (!bodyLines.empty()) {
        ty += 6;
        for (const auto& l : bodyLines) { gfx.text(l, x + PAD, ty, a(col::textDim), fBody); ty += 20; }
    }
}

// --- the sidebar -------------------------------------------------------------------

namespace {

SDL_Rect navRect(int i) { return SDL_Rect{14 + i * 89, 654, 82, 52}; }

// The region list: a heading per region, then a row per game or DLC. Labels
// that do not fit scroll (Gfx::marquee) rather than being cut short.
constexpr int SIDE_TOP = 110, HEADING_H = 28, ROW_H = 30;
constexpr int LABEL_X = 34, DLC_X = 50, COUNT_R = 265;

} // anonymous namespace

void App::drawSidebar() {
    gfx.rect(0, 0, SIDEBAR_W, SCREEN_H, col::sidebar);
    gfx.rect(SIDEBAR_W - 1, 0, 1, SCREEN_H, col::divider);

    drawLogo(20, 25, 44);
    gfx.text("pkDex", 70, 23, col::text, gfx.font(22, true));
    gfx.text(gfx.fit(tr("app/tagline"), gfx.font(12), 200), 70, 50, col::textDim, gfx.font(12));

    int regionsCount = 0;
    for (const Region& r : dex::regions()) regionsCount += r.child ? 0 : 1;
    drawEyebrow(tr("sidebar/regions"), 22, 89, col::textMuted);
    gfx.textRight(std::to_string(regionsCount), COUNT_R, 95, col::textMuted, gfx.font(12));

    Font* fHeading = gfx.font(15, true);
    Font* fLabel = gfx.font(13, true);
    Font* fCount = gfx.font(12);
    Font* fTag = gfx.font(9, true);
    const bool onDex = page == Page::Dex || page == Page::Detail;
    int y = SIDE_TOP;
    for (int i = 0; i < dex::count(); i++) {
        if (!dex::regions()[i].child) {
            gfx.textMid(gfx.fit(dex::name(i), fHeading, COUNT_R - 22), 22, y + 16, col::text, fHeading);
            y += HEADING_H;
        }

        const int cy = y + ROW_H / 2;
        const bool active = onDex && region == i;
        if (active) gfx.fillRounded(10, y, 267, ROW_H, 8, col::sideSel);
        if (sidebarFocus && sideCursor == i) drawFocusRing(10, y, 267, ROW_H, 8);

        const tracker::Counts& c = counts(i);
        const int total = static_cast<int>(dex::list(i).size());
        const bool complete = total > 0 && c.any >= total;
        const std::string count = std::to_string(c.any) + "/" + std::to_string(total);
        const SDL_Color cc = complete ? col::green : active ? col::accent : col::textMuted;
        int right = COUNT_R - gfx.textRight(count, COUNT_R, cy, cc, fCount);
        if (complete) {
            gfx.icon(Icon::Check, right - 9, cy, 12, col::green);
            right -= 16;
        }
        if (dex::regions()[i].dlc) {
            const std::string tag = tr("sidebar/dlc");
            const int tw = gfx.textW(tag, fTag) + 12;
            right -= 8 + tw;
            gfx.fillRounded(right, cy - 8, tw, 16, 4, col::chip);
            gfx.textCenter(tag, right + tw / 2, cy, col::textDim, fTag);
        }
        // A DLC hangs off its game, as the 1.x sidebar drew it: └ from the row
        // above, and ├ down to the next when another DLC follows.
        int labelX = LABEL_X;
        if (dex::regions()[i].dlc) {
            const bool more = i + 1 < dex::count() && dex::regions()[i + 1].dlc;
            gfx.rect(LABEL_X + 4, y - 4, 1, (more ? ROW_H : ROW_H / 2) + 4, col::textFaint);
            gfx.rect(LABEL_X + 4, cy, 8, 1, col::textFaint);
            labelX = DLC_X;
        }
        gfx.marquee(dex::sidebar(i), labelX, cy, right - 10 - labelX, active ? col::text : col::textDim, fLabel,
                    sidebarFocus && sideCursor == i);
        y += ROW_H;
    }

    gfx.rect(0, 643, SIDEBAR_W - 1, 1, col::divider);
    static const Icon icons[NAV_COUNT] = {Icon::Gear, Icon::Info, Icon::List};
    static const char* labels[NAV_COUNT] = {"nav/settings", "nav/about", "nav/changelog"};
    static const Page pages[NAV_COUNT] = {Page::Settings, Page::About, Page::Changelog};
    Font* fNav = gfx.font(11);
    for (int i = 0; i < NAV_COUNT; i++) {
        const SDL_Rect r = navRect(i);
        const bool active = page == pages[i];
        if (active) gfx.fillRounded(r.x, r.y, r.w, r.h, 10, col::sideSel);
        if (sidebarFocus && sideCursor == dex::count() + i) drawFocusRing(r.x, r.y, r.w, r.h, 10);
        const SDL_Color c = active ? col::text : col::textDim;
        gfx.icon(icons[i], r.x + r.w / 2, r.y + 17, 16, c);
        gfx.textCenter(gfx.fit(tr(labels[i]), fNav, r.w - 6), r.x + r.w / 2, r.y + 38, c, fNav);
    }

    if (sidebarFocus) {
        // The footer belongs to the page; with the sidebar focused it says
        // what the sidebar does instead.
        drawFooter(SIDEBAR_W, {{"+", tr("hints/quit")}, {"A", tr("hints/open")}});
    }
}

void App::selectSide(int index) {
    const int n = dex::count();
    sideCursor = std::clamp(index, 0, n + NAV_COUNT - 1);
    if (sideCursor < n) {
        region = sideCursor;
        page = Page::Dex;
    } else {
        static const Page pages[NAV_COUNT] = {Page::Settings, Page::About, Page::Changelog};
        page = pages[sideCursor - n];
        if (page == Page::Changelog) changelogScroll = 0;
    }
}

void App::inputSidebar(uint32_t pressed) {
    const int n = dex::count();
    const bool inNav = sideCursor >= n;
    if (pressed & BTN_UP) {
        if (inNav) selectSide(n - 1);
        else if (sideCursor > 0) selectSide(sideCursor - 1);
    } else if (pressed & BTN_DOWN) {
        if (!inNav) selectSide(sideCursor + 1);
    } else if (pressed & BTN_LEFT) {
        if (inNav && sideCursor > n) selectSide(sideCursor - 1);
    } else if (pressed & BTN_RIGHT) {
        if (inNav && sideCursor < n + NAV_COUNT - 1) selectSide(sideCursor + 1);
        else if (!inNav) sidebarFocus = false;
    }
    if (pressed & BTN_A) {
        // Into the page: nothing to go into on the About page.
        if (page != Page::About) sidebarFocus = false;
    }
}

// --- the footer --------------------------------------------------------------------

int App::keyWidth(const std::string& key) {
    const auto parts = util::split(key, " ");
    int w = 0;
    for (const std::string& k : parts) {
        if (k.empty()) continue;
        if (w) w += 4;
        w += k.size() == 1 ? 22 : gfx.textW(k, gfx.font(11, true)) + 14;
    }
    return w;
}

int App::drawKey(const std::string& key, int x, int cy, bool dark) {
    const SDL_Color bg = dark ? col::chip : col::keyCap;
    const SDL_Color fg = dark ? col::textDim : col::keyCapText;
    Font* f = gfx.font(11, true);
    const int x0 = x;
    for (const std::string& k : util::split(key, " ")) {
        if (k.empty()) continue;
        if (x != x0) x += 4;
        if (k.size() == 1) {
            if (dark) gfx.fillRounded(x, cy - 11, 22, 22, 6, bg);
            else gfx.disc(x + 11, cy, 11, bg);
            gfx.textCenter(k, x + 11, cy, fg, f);
            x += 22;
        } else {
            const int w = gfx.textW(k, f) + 14;
            gfx.fillRounded(x, cy - 11, w, 22, 6, bg);
            gfx.textCenter(k, x + w / 2, cy, fg, f);
            x += w;
        }
    }
    return x - x0;
}

int App::hintsWidth(const std::vector<Hint>& hints) {
    Font* f = gfx.font(14);
    int w = 0;
    for (const Hint& h : hints) {
        if (w) w += 22;
        w += keyWidth(h.key) + 8 + gfx.textW(h.label, f);
    }
    return w;
}

void App::drawHints(int right, int cy, const std::vector<Hint>& hints, SDL_Color label) {
    Font* f = gfx.font(14);
    int x = right - hintsWidth(hints);
    for (const Hint& h : hints) {
        x += drawKey(h.key, x, cy) + 8;
        x += gfx.textMid(h.label, x, cy, label, f) + 22;
    }
}

void App::drawFooter(int left, const std::vector<Hint>& hints) {
    if (config::getBool(config::HIDE_FOOTER, false)) return;
    gfx.rect(left, FOOTER_Y, SCREEN_W - left, FOOTER_H, col::bg);
    gfx.rect(left, FOOTER_Y, SCREEN_W - left, 1, col::divider);
    const int cy = FOOTER_Y + FOOTER_H / 2;

    const uint32_t now = SDL_GetTicks();
    if (batteryAt_ == 0 || now - batteryAt_ > 10000) {
        batteryAt_ = now ? now : 1;
        u32 pct = 0;
        if (R_SUCCEEDED(psmGetBatteryChargePercentage(&pct))) battery_ = static_cast<int>(pct);
        PsmChargerType type = PsmChargerType_Unconnected;
        charging_ = R_SUCCEEDED(psmGetChargerType(&type)) && type != PsmChargerType_Unconnected;
    }
    const int bx = left + 40;
    gfx.battery(bx, cy, battery_, charging_, col::textDim);
    char clock[8] = "--:--";
    const time_t t = time(nullptr);
    if (const struct tm* lt = localtime(&t)) strftime(clock, sizeof(clock), "%H:%M", lt);
    gfx.textMid(clock, bx + 38, cy, col::textDim, gfx.font(14));

    drawHints(SCREEN_W - 32, cy, hints, col::text);
}

// --- shared pieces -------------------------------------------------------------------

void App::drawToggle(int x, int cy, bool on) {
    gfx.fillRounded(x, cy - 13, 44, 26, 13, on ? col::accent : col::toggleOff);
    gfx.disc(on ? x + 31 : x + 13, cy, 10, on ? SDL_Color{255, 255, 255, 255} : col::knobOff);
}

void App::drawLogo(int x, int y, int size) {
    if (SDL_Texture* t = gfx.pinned(LOGO_PATH)) {
        // The emblem without the transparent margin around it.
        const SDL_Rect src{23, 22, 211, 212};
        const SDL_Rect dst{x, y, size, size};
        SDL_SetTextureAlphaMod(t, 255);
        SDL_RenderCopy(gfx.renderer(), t, &src, &dst);
        return;
    }
    // Only if the image is missing from the RomFS.
    gfx.fillRounded(x, y, size, size, size * 10 / 44, col::accent);
    gfx.icon(Icon::Logo, x + size / 2, y + size / 2, size * 6 / 10, SDL_Color{0x1a, 0x0d, 0x0c, 255});
}

void App::drawEyebrow(const std::string& s, int x, int y, SDL_Color c) {
    gfx.tracked(util::toUpper(s), x, y, c, gfx.font(11, true), 1);
}

void App::drawFocusRing(int x, int y, int w, int h, int radius) {
    gfx.strokeRounded(x - 5, y - 5, w + 10, h + 10, radius + 5, 3, withAlpha(col::accent, 45));
    gfx.strokeRounded(x - 2, y - 2, w + 4, h + 4, radius + 2, 2, col::accent);
}

void App::drawStateIcon(int state, int cx, int cy, int size) {
    if (state == Capture::Regular) {
        gfx.disc(cx, cy, size / 2, col::green);
        gfx.icon(Icon::Check, cx, cy, size * 7 / 10, col::bg);
        return;
    }
    // pkHouse's icons. The sparkle is white and tinted gold, as pkHouse does.
    static const char* const paths[Capture::Count] = {
        nullptr, "romfs:/img/states/shiny.png", "romfs:/img/states/alpha.png", "romfs:/img/states/shiny_alpha.png",
    };
    if (state < 0 || state >= Capture::Count) return;
    SDL_Texture* t = gfx.pinned(paths[state]);
    if (!t) {
        // Only if the icons are missing from the RomFS.
        gfx.icon(state == Capture::Shiny ? Icon::Sparkle : Icon::Alpha, cx, cy, size,
                 state == Capture::Shiny ? col::gold : state == Capture::Alpha ? col::alpha : col::shinyAlpha);
        return;
    }
    if (state == Capture::Shiny) SDL_SetTextureColorMod(t, col::gold.r, col::gold.g, col::gold.b);
    gfx.blitFit(t, cx, cy, size);
    SDL_SetTextureColorMod(t, 255, 255, 255);
}

void App::drawStateBadge(int state, int x, int y, int box) {
    static const SDL_Color colors[Capture::Count] = {col::green, col::gold, col::alpha, col::shinyAlpha};
    const SDL_Color c = colors[std::clamp(state, 0, Capture::Count - 1)];
    gfx.fillRounded(x, y, box, box, box / 4, mix(col::modal, c, 0.18f));
    drawStateIcon(state, x + box / 2, y + box / 2, box / 2);
}

void App::drawPokemonThumb(const Pokemon& p, int x, int y, int box, int radius) {
    const SDL_Color tc = p.typesEn.empty() ? col::knobOff : typeColor(p.typesEn[0]);
    gfx.fillRounded(x, y, box, box, radius, mix(col::card, tc, 0.16f));
    int w = 0, h = 0;
    if (SDL_Texture* t = gfx.image(dex::iconPath(p), &w, &h)) {
        // Whole multiples only: the sprites are pixel art.
        const int k = std::max(1, std::min((box - 6) / std::max(1, w), (box - 6) / std::max(1, h)));
        gfx.blit(t, x + (box - w * k) / 2, y + (box - h * k) / 2, w * k, h * k);
    } else {
        gfx.icon(Icon::Image, x + box / 2, y + box / 2, box * 4 / 10, withAlpha(tc, 200));
    }
}

void App::drawSpinner(int cx, int cy, int radius, SDL_Color c) {
    gfx.ring(cx, cy, radius, 3, withAlpha(c, 60));
    const float a = static_cast<float>(SDL_GetTicks() % 1000) / 1000.0f * 6.2831853f;
    gfx.arc(cx, cy, radius - 1, a, a + 1.7f, 3.0f, c);
}

const tracker::Counts& App::counts(int r) {
    if (countsVersion_ != tracker::version() || counts_.size() != static_cast<size_t>(dex::count())) {
        counts_.assign(dex::count(), tracker::Counts{});
        for (int i = 0; i < dex::count(); i++)
            counts_[i] = tracker::counts(dex::regions()[i].id, dex::list(i));
        countsVersion_ = tracker::version();
    }
    return counts_[r];
}

std::string App::stateLabel(int state) const { return tr(std::string("states/") + stateKey(state)); }

std::vector<int> App::statesOf(int r) const {
    std::vector<int> s = {Capture::Regular, Capture::Shiny};
    if (dex::regions()[r].alpha) { s.push_back(Capture::Alpha); s.push_back(Capture::ShinyAlpha); }
    return s;
}

// --- actions ---------------------------------------------------------------------------

void App::openCapture(int r, int index) {
    if (index < 0 || index >= static_cast<int>(dex::list(r).size())) return;
    push(std::make_unique<CaptureModal>(r, index));
}

namespace {

// Mark or clear one state on a set of Pokemon, then say how many.
void applyBulk(App& app, int r, const std::vector<std::string>& regionals, int state, bool value) {
    tracker::bulkSet(dex::regions()[r].id, regionals, state, value);
    app.toast(trf("bulk/done", {{"count", std::to_string(regionals.size())}}));
}

std::vector<ActionModal::Item> bulkItems(App& app, int r, std::function<void(App&, int, bool)> act,
                                         const std::string& detail) {
    std::vector<ActionModal::Item> items;
    for (int pass = 0; pass < 2; pass++) {
        const bool value = pass == 0;
        ActionModal::Item header;
        header.label = tr(value ? "bulk/mark" : "bulk/clear");
        header.header = true;
        items.push_back(header);
        for (int s : app.statesOf(r)) {
            ActionModal::Item it;
            it.label = tr(std::string(value ? "bulk/mark_" : "bulk/clear_") + stateKey(s));
            it.detail = detail;
            it.state = s;
            it.run = [act, s, value](App& a) { act(a, s, value); };
            items.push_back(it);
        }
    }
    return items;
}

} // anonymous namespace

void App::openBulk() {
    const int r = region;
    const auto& list = dex::list(r);
    if (list.empty()) return;
    const std::string count = std::to_string(list.size());
    auto act = [r, count](App& app, int s, bool value) {
        auto c = std::make_unique<ConfirmModal>();
        c->kind = ConfirmModal::Warning;
        c->title = trf("bulk/confirm_title", {{"count", count}});
        c->body = trf(value ? "bulk/confirm_mark_body" : "bulk/confirm_clear_body",
                      {{"state", app.stateLabel(s)}, {"region", dex::name(r)}, {"count", count}});
        c->cancelLabel = tr("common/cancel");
        c->confirmLabel = tr("bulk/apply");
        c->danger = !value;
        c->onConfirm = [r, s, value](App& a) {
            std::vector<std::string> regionals;
            for (const auto& p : dex::list(r)) regionals.push_back(p.regional);
            applyBulk(a, r, regionals, s, value);
        };
        app.push(std::move(c));
    };
    push(std::make_unique<ActionModal>(tr("bulk/title"),
                                       trf("bulk/subtitle", {{"region", dex::name(r)}, {"count", count}}),
                                       bulkItems(*this, r, act, trf("common/count_pokemon", {{"count", count}}))));
}

void App::openMultiApply() {
    const int r = region;
    const DexState& st = dexStates[r];
    if (st.selected.empty()) { toast(tr("multi/none")); return; }
    const std::string count = std::to_string(st.selected.size());
    auto act = [r](App& app, int s, bool value) {
        std::vector<std::string> regionals;
        const auto& list = dex::list(r);
        for (int i : app.dexStates[r].selected)
            if (i >= 0 && i < static_cast<int>(list.size())) regionals.push_back(list[i].regional);
        // The selection stays, so several states can be applied in a row.
        applyBulk(app, r, regionals, s, value);
    };
    push(std::make_unique<ActionModal>(tr("multi/title"),
                                       trf("multi/subtitle", {{"region", dex::name(r)}, {"count", count}}),
                                       bulkItems(*this, r, act, trf("common/count_pokemon", {{"count", count}}))));
}

void App::openRegionReset() {
    auto d = std::make_unique<DrawerModal>();
    d->eyebrow = tr("settings/group/pokemon_data");
    d->title = tr("reset/drawer_title");
    // As in the sidebar: a heading per region, a choice per game. The value
    // kept is 0 for all regions, else the dex's place in the list plus one.
    const int current = std::clamp(config::getInt(config::RESET_REGION, 0), 0, dex::count());
    DrawerModal::Item all;
    all.label = tr("reset/all_regions");
    all.value = 0;
    d->items.push_back(all);
    for (int i = 0; i < dex::count(); i++) {
        if (!dex::regions()[i].child) {
            DrawerModal::Item h;
            h.label = dex::name(i);
            h.heading = true;
            d->items.push_back(h);
        }
        DrawerModal::Item it;
        it.label = dex::sidebar(i);
        it.child = true;
        it.dlc = dex::regions()[i].dlc;
        it.value = i + 1;
        if (it.value == current) d->selected = static_cast<int>(d->items.size());
        d->items.push_back(it);
    }
    d->cursor = d->selected;
    d->onChoose = [](App&, int index) { config::setInt(config::RESET_REGION, index); };
    push(std::move(d));
}

void App::confirmReset() {
    const int index = std::clamp(config::getInt(config::RESET_REGION, 0), 0, dex::count());
    const bool all = index == 0;
    const int r = index - 1;
    auto c = std::make_unique<ConfirmModal>();
    c->kind = ConfirmModal::Warning;
    c->danger = true;
    c->title = all ? tr("reset/all_title") : trf("reset/region_title", {{"region", regionLabel(r)}});
    c->body = all ? tr("reset/all_body") : trf("reset/region_body", {{"region", regionLabel(r)}});
    c->cancelLabel = tr("common/cancel");
    c->confirmLabel = tr(all ? "reset/all_button" : "reset/region_button");
    c->onConfirm = [all, r](App& app) {
        const bool ok = all ? tracker::resetAll() : tracker::resetRegion(dex::regions()[r].id);
        if (!ok) app.toast(tr("reset/failure"), true);
        else if (all) app.toast(tr("reset/success_all"));
        else app.toast(trf("reset/success_region", {{"region", regionLabel(r)}}));
    };
    push(std::move(c));
}

void App::openLanguage() {
    auto d = std::make_unique<DrawerModal>();
    d->eyebrow = tr("settings/group/language");
    d->title = tr("language/title");
    const auto& locs = i18n::locales();
    for (size_t i = 0; i < locs.size(); i++) {
        d->items.push_back({locs[i].label, locs[i].code, false});
        if (i18n::locale() == locs[i].code) d->selected = static_cast<int>(i);
    }
    d->cursor = d->selected;
    d->onChoose = [](App& app, int index) {
        const auto& l = i18n::locales();
        if (index >= 0 && index < static_cast<int>(l.size()) && i18n::locale() != l[index].code)
            app.setLanguage(l[index].code);
    };
    push(std::move(d));
}

void App::setLanguage(const std::string& code) {
    config::setString(config::LOCALE, code);
    // Applied at once: the strings and the translated data are reloaded, no restart needed.
    i18n::init(code);
    loadData();
    changelogBuilt_ = false;   // its fallback text and wrapping follow the language
    toast(tr("language/changed"));
}

void App::checkForUpdates(bool manual) {
    if (Update::state() == Update::State::Checking) return;
    if (!net::online()) {
        if (manual) toast(tr("net/offline"), true);
        return;
    }
    manualCheck_ = manual;
    Update::beginCheck();
}

void App::offerUpdate() {
    auto c = std::make_unique<ConfirmModal>();
    c->kind = ConfirmModal::Info;
    c->title = trf("update/available_title", {{"version", Update::latestVersion()}});
    c->body = trf("update/available_body", {{"version", Update::latestVersion()}, {"current", APP_VERSION}});
    c->cancelLabel = tr("common/later");
    c->confirmLabel = tr("update/install");
    c->focus = 1;
    c->onConfirm = [](App& app) { app.startUpdate(); };
    push(std::move(c));
}

bool App::runJob(std::function<void(Job&)> fn) {
    if (worker.busy()) { toast(tr("common/busy"), true); return false; }
    auto j = std::make_shared<Job>();
    if (!worker.start([j, fn]() { fn(*j); })) { toast(tr("common/busy"), true); return false; }
    job = j;
    return true;
}

void App::startUpdate() {
    if (Update::state() != Update::State::Available) { toast(tr("update/none")); return; }
    if (!net::start()) { toast(tr("net/offline"), true); return; }
    const std::string exe = exePath;
    if (!runJob([exe](Job& j) { Update::install(exe, j); })) return;
    push(std::make_unique<UpdateModal>(job));
}

void App::startPackDownload(bool confirmed) {
    if (!net::online()) { toast(tr("net/offline"), true); return; }
    if (!confirmed && pack::zipExists()) {
        auto c = std::make_unique<ConfirmModal>();
        c->kind = ConfirmModal::Info;
        c->title = tr("pack/exists_title");
        c->body = tr("pack/exists_body");
        c->cancelLabel = tr("common/cancel");
        c->confirmLabel = tr("pack/redownload");
        c->onConfirm = [](App& app) { app.startPackDownload(true); };
        push(std::move(c));
        return;
    }
    if (!net::start()) { toast(tr("net/offline"), true); return; }
    if (!runJob([](Job& j) { pack::download(j); })) return;
    auto m = std::make_unique<ProgressModal>();
    m->title = tr("pack/downloading");
    m->job = job;
    m->cancellable = true;
    m->showBytes = true;
    m->onDone = [](App& app, bool ok, const std::string& msg) {
        if (ok) {
            auto c = std::make_unique<ConfirmModal>();
            c->kind = ConfirmModal::Success;
            c->title = tr("pack/downloaded_title");
            c->body = tr("pack/downloaded_body");
            c->cancelLabel = tr("common/later");
            c->confirmLabel = tr("pack/extract");
            c->focus = 1;
            c->onConfirm = [](App& a) { a.startPackExtract(true); };
            app.push(std::move(c));
        } else if (msg == "cancelled") {
            app.toast(tr("pack/cancelled"));
        } else {
            app.toast(trf("pack/download_failed", {{"error", msg}}), true);
        }
    };
    push(std::move(m));
}

void App::startPackExtract(bool confirmed) {
    if (!pack::zipExists()) { toast(tr("pack/missing"), true); return; }
    if (!confirmed) {
        auto c = std::make_unique<ConfirmModal>();
        c->kind = ConfirmModal::Info;
        c->title = tr("pack/extract_title");
        c->body = tr("pack/extract_body");
        c->cancelLabel = tr("common/cancel");
        c->confirmLabel = tr("pack/extract");
        c->focus = 1;
        c->onConfirm = [](App& app) { app.startPackExtract(true); };
        push(std::move(c));
        return;
    }
    if (!runJob([](Job& j) { pack::extract(j); })) return;
    auto m = std::make_unique<ProgressModal>();
    m->title = tr("pack/extracting");
    m->job = job;
    m->onDone = [](App& app, bool ok, const std::string& msg) {
        if (!ok) {
            auto c = std::make_unique<ConfirmModal>();
            c->kind = ConfirmModal::Error;
            c->single = true;
            c->title = tr("pack/extract_failed");
            c->body = msg;
            c->confirmLabel = tr("common/ok");
            app.push(std::move(c));
            return;
        }
        // The new sprites replace the small ones from here on.
        dex::forgetSpritePaths();
        gfx.forgetImages();
        auto c = std::make_unique<ConfirmModal>();
        c->kind = ConfirmModal::Success;
        c->title = tr("pack/extracted_title");
        c->body = tr("pack/extracted_body");
        c->cancelLabel = tr("pack/keep");
        c->confirmLabel = tr("pack/delete");
        c->onCancel = [](App& a) { a.toast(tr("pack/zip_kept")); };
        c->onConfirm = [](App& a) {
            if (remove(pack::ZIP) == 0) a.toast(tr("pack/zip_deleted"));
            else a.toast(tr("pack/zip_delete_failed"), true);
        };
        app.push(std::move(c));
    };
    push(std::move(m));
}
