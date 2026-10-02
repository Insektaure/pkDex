// Settings, About and Changelog (external/new_ui "Settings").

#include "app.h"
#include "config.h"
#include "i18n.h"
#include "pack.h"
#include "theme.h"
#include "util.h"

#include <algorithm>
#include <cstdlib>

using namespace layout;

namespace {

// --- settings layout -----------------------------------------------------------------

enum Row {
    CheckOnLaunch, CheckNow, InstallUpdate, RegionToReset, ResetCapture,
    HideFooter, Language, DownloadPack, ExtractPack, RowCount,
};

constexpr int COL_W = 444;
constexpr int LEFT_X = CONTENT_X, RIGHT_X = CONTENT_X + COL_W + 24;
constexpr int ROW_H = 55;

struct Group { const char* label; int x, y; int first, count; };

// Each group: its label's top, then a card holding its rows.
const Group GROUPS[] = {
    {"settings/group/updates",        LEFT_X,  118, CheckOnLaunch, 3},
    {"settings/group/pokemon_data",   LEFT_X,  337, RegionToReset, 2},
    {"settings/group/user_interface", RIGHT_X, 118, HideFooter,    1},
    {"settings/group/language",       RIGHT_X, 228, Language,      1},
    {"settings/group/resources",      RIGHT_X, 338, DownloadPack,  2},
};

const Group& groupOf(int row) {
    for (const Group& g : GROUPS)
        if (row >= g.first && row < g.first + g.count) return g;
    return GROUPS[0];
}

int rowTop(int row) {
    const Group& g = groupOf(row);
    return g.y + 22 + 4 + (row - g.first) * ROW_H;
}

bool inLeft(int row) { return groupOf(row).x == LEFT_X; }

std::string resetLabel() {
    const int index = std::clamp(config::getInt(config::RESET_REGION, 0), 0, dex::count());
    if (index == 0) return tr("reset/all_regions");
    return dex::sidebar(index - 1);
}

// Asked of the SD card once a second at most, not every frame.
bool zipThere() {
    static bool there = false;
    static uint32_t at = 0;
    const uint32_t now = SDL_GetTicks();
    if (at == 0 || now - at > 1000) {
        there = pack::zipExists();
        at = now ? now : 1;
    }
    return there;
}

std::string localeLabel() {
    for (const auto& l : i18n::locales())
        if (i18n::locale() == l.code) return l.label;
    return i18n::locale();
}

} // anonymous namespace

void App::drawSettings() {
    drawEyebrow(tr("settings/eyebrow"), CONTENT_X, 27, col::accent);
    gfx.text(tr("settings/title"), CONTENT_X, 40, col::text, gfx.font(40, true));
    gfx.textRight(std::string("pkDex v") + APP_VERSION, CONTENT_R, 76, col::textMuted, gfx.font(13));

    for (const Group& g : GROUPS) {
        drawEyebrow(tr(g.label), g.x + 4, g.y, col::textDim);
        const int cy = g.y + 22, ch = 8 + g.count * ROW_H;
        gfx.fillRounded(g.x, cy, COL_W, ch, 14, col::card);
        gfx.strokeRounded(g.x, cy, COL_W, ch, 14, 1, col::cardBorder);
        for (int i = 1; i < g.count; i++)
            gfx.rect(g.x + 16, cy + 4 + i * ROW_H, COL_W - 32, 1, col::rowSep);
    }

    const Update::State us = Update::state();
    Font* fLabel = gfx.font(16);
    Font* fValue = gfx.font(14);
    for (int row = 0; row < RowCount; row++) {
        const Group& g = groupOf(row);
        const int top = rowTop(row), cy = top + ROW_H / 2;
        const int x = g.x, right = g.x + COL_W - 18;
        const bool focused = !sidebarFocus && settingsCursor == row;
        if (focused) {
            drawFocusRing(x + 4, top + 1, COL_W - 8, ROW_H - 2, 10);
            gfx.fillRounded(x + 4, top + 1, COL_W - 8, ROW_H - 2, 10, col::cardFocus);
        }

        std::string label, value;
        SDL_Color labelColor = col::text, valueColor = col::textDim;
        enum { Chevron, Toggle, Download, Spinner, None } trailing = Chevron;
        bool toggleOn = false;
        switch (row) {
            case CheckOnLaunch:
                label = tr("settings/check_on_launch");
                trailing = Toggle;
                toggleOn = config::getBool(config::CHECK_ON_LAUNCH, true);
                break;
            case CheckNow:
                label = tr("settings/check_now");
                if (us == Update::State::Checking) { value = tr("settings/value/checking"); trailing = Spinner; }
                else if (us == Update::State::UpToDate) value = tr("settings/value/up_to_date");
                else if (us == Update::State::Failed) value = tr("settings/value/check_failed");
                break;
            case InstallUpdate:
                label = tr("settings/install_update");
                trailing = Download;
                if (us == Update::State::Available) {
                    value = "v" + Update::latestVersion();
                    valueColor = col::accent;
                } else {
                    value = tr("settings/value/no_update");
                }
                break;
            case RegionToReset:
                label = tr("settings/region_to_reset");
                value = resetLabel();
                break;
            case ResetCapture:
                label = tr("settings/reset_capture");
                labelColor = col::danger;
                break;
            case HideFooter:
                label = tr("settings/hide_footer");
                trailing = Toggle;
                toggleOn = config::getBool(config::HIDE_FOOTER, false);
                break;
            case Language:
                label = tr("settings/language");
                value = localeLabel();
                break;
            case DownloadPack:
                label = tr("settings/download_pack");
                value = zipThere() ? tr("settings/value/downloaded") : tr("settings/value/pack_size");
                trailing = Download;
                break;
            case ExtractPack:
                label = tr("settings/extract_pack");
                if (!zipThere()) value = tr("settings/value/not_downloaded");
                break;
        }

        int valueRight = right;
        switch (trailing) {
            case Toggle:   drawToggle(right - 44, cy, toggleOn); valueRight = right - 56; break;
            case Chevron:  gfx.icon(Icon::Chevron, right - 6, cy, 16, col::textMuted); valueRight = right - 22; break;
            case Download: gfx.icon(Icon::Download, right - 6, cy, 15, col::textMuted); valueRight = right - 22; break;
            case Spinner:  drawSpinner(right - 6, cy, 7, col::textDim); valueRight = right - 22; break;
            case None:     break;
        }
        int valueW = 0;
        if (!value.empty()) {
            // Right-aligned; a value too long for its half of the row scrolls.
            const int room = COL_W / 2 - 20;
            const int vw = std::min(gfx.textW(value, fValue), room);
            gfx.marquee(value, valueRight - vw, cy, vw, valueColor, fValue, focused);
            valueW = vw + 12;
        }
        gfx.textMid(gfx.fit(label, fLabel, valueRight - valueW - (x + 19)), x + 19, cy, labelColor, fLabel);
    }

    if (!sidebarFocus) drawFooter(SIDEBAR_W, {{"B", tr("hints/back")}, {"A", tr("hints/select")}});
}

void App::inputSettings(uint32_t pressed) {
    const int row = settingsCursor;
    auto nearestIn = [](bool left, int y) {
        int best = -1, bestD = 1 << 30;
        for (int r = 0; r < RowCount; r++) {
            if (inLeft(r) != left) continue;
            const int d = std::abs(rowTop(r) - y);
            if (d < bestD) { bestD = d; best = r; }
        }
        return best;
    };
    if (pressed & BTN_UP) {
        for (int r = row - 1; r >= 0; r--)
            if (inLeft(r) == inLeft(row)) { settingsCursor = r; break; }
    } else if (pressed & BTN_DOWN) {
        for (int r = row + 1; r < RowCount; r++)
            if (inLeft(r) == inLeft(row)) { settingsCursor = r; break; }
    } else if (pressed & BTN_LEFT) {
        if (inLeft(row)) { sidebarFocus = true; sideCursor = dex::count(); return; }
        settingsCursor = nearestIn(true, rowTop(row));
    } else if (pressed & BTN_RIGHT) {
        if (inLeft(row)) settingsCursor = nearestIn(false, rowTop(row));
    }
    if (pressed & BTN_A) activateSetting(settingsCursor);
    if (pressed & BTN_B) { sidebarFocus = true; sideCursor = dex::count(); }
}

void App::activateSetting(int row) {
    switch (row) {
        case CheckOnLaunch: {
            const bool on = !config::getBool(config::CHECK_ON_LAUNCH, true);
            config::setBool(config::CHECK_ON_LAUNCH, on);
            break;
        }
        case CheckNow:
            checkForUpdates(true);
            break;
        case InstallUpdate:
            if (Update::state() == Update::State::Available) offerUpdate();
            else checkForUpdates(true);
            break;
        case RegionToReset:
            openRegionReset();
            break;
        case ResetCapture:
            confirmReset();
            break;
        case HideFooter:
            config::setBool(config::HIDE_FOOTER, !config::getBool(config::HIDE_FOOTER, false));
            break;
        case Language:
            openLanguage();
            break;
        case DownloadPack:
            startPackDownload(false);
            break;
        case ExtractPack:
            startPackExtract(false);
            break;
    }
}

// --- about ---------------------------------------------------------------------------

void App::drawAbout() {
    drawEyebrow(tr("about/eyebrow"), CONTENT_X, 27, col::accent);
    gfx.text(tr("about/title"), CONTENT_X, 40, col::text, gfx.font(40, true));

    const int w = CONTENT_R - CONTENT_X;
    {
        const int y = 124, h = 200;
        gfx.fillRounded(CONTENT_X, y, w, h, 16, col::card);
        gfx.strokeRounded(CONTENT_X, y, w, h, 16, 1, col::cardBorder);
        if (SDL_Texture* t = gfx.image("romfs:/img/pkdex_256.png")) gfx.blitFit(t, CONTENT_X + 24 + 76, y + h / 2, 152);
        else drawLogo(CONTENT_X + 24 + 36, y + h / 2 - 40, 80);
        const int tx = CONTENT_X + 200;
        gfx.text("pkDex", tx, y + 26, col::text, gfx.font(32, true));
        gfx.text(trf("about/version", {{"version", APP_VERSION}, {"author", APP_AUTHOR}}), tx, y + 70, col::textDim,
                 gfx.font(14));
        Font* f = gfx.font(15);
        int ly = y + 104;
        for (const auto& l : gfx.wrap(tr("about/description"), f, CONTENT_R - 24 - tx, 3)) {
            gfx.text(l, tx, ly, col::text, f);
            ly += 23;
        }
    }

    {
        struct Fact { const char* label; const char* value; };
        static const Fact facts[] = {
            {"about/source_label",  "about/source"},
            {"about/license_label", "about/license"},
            {"about/data_label",    "about/data"},
            {"about/logo_label",    "about/logo"},
        };
        const int y = 340, h = 116, gap = 16;
        const int cw = (w - 3 * gap) / 4;
        for (int i = 0; i < 4; i++) {
            const int x = CONTENT_X + i * (cw + gap);
            gfx.fillRounded(x, y, cw, h, 14, col::card);
            gfx.strokeRounded(x, y, cw, h, 14, 1, col::cardBorder);
            drawEyebrow(tr(facts[i].label), x + 18, y + 18, col::textDim);
            Font* f = gfx.font(14);
            int ly = y + 42;
            for (const auto& l : gfx.wrap(tr(facts[i].value), f, cw - 36, 3)) {
                gfx.text(l, x + 18, ly, col::text, f);
                ly += 21;
            }
        }
    }

    {
        Font* f = gfx.font(13);
        int ly = 480;
        for (const auto& l : gfx.wrap(tr("about/disclaimer"), f, w, 6)) {
            gfx.text(l, CONTENT_X, ly, col::textMuted, f);
            ly += 20;
        }
    }

    if (!sidebarFocus) drawFooter(SIDEBAR_W, {{"B", tr("hints/back")}});
}

// --- changelog -----------------------------------------------------------------------

namespace {

constexpr int LOG_X = CONTENT_X, LOG_Y = 124, LOG_W = CONTENT_R - CONTENT_X, LOG_H = 520;
constexpr int LOG_PAD = 28;

enum LineKind { Text = 0, Heading = 1, Bullet = 2, Gap = 3, Continuation = 4 };

int lineHeight(int kind) {
    switch (kind) {
        case Heading: return 44;
        case Gap:     return 10;
        default:      return 24;
    }
}

} // anonymous namespace

void App::buildChangelog() {
    changelog_.clear();
    changelogBuilt_ = true;
    std::string text;
    if (!util::readFile("sdmc:/switch/pkDex/changelog.txt", text) && !util::readFile("romfs:/changelog.txt", text)) {
        changelog_.push_back({Text, tr("changelog/missing"), 0});
        return;
    }
    Font* fText = gfx.font(15);
    Font* fHead = gfx.font(19, true);
    const int width = LOG_W - 2 * LOG_PAD;
    bool lastGap = true;
    for (const std::string& raw : util::split(text, "\n")) {
        std::string line = raw;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        size_t lead = 0;
        while (lead < line.size() && line[lead] == ' ') lead++;
        const std::string body = line.substr(lead);
        if (body.empty()) {
            if (!lastGap) changelog_.push_back({Gap, "", 0});
            lastGap = true;
            continue;
        }
        lastGap = false;
        if (body.rfind("#", 0) == 0) {
            size_t h = 0;
            while (h < body.size() && body[h] == '#') h++;
            for (const auto& l : gfx.wrap(util::trim(body.substr(h)), fHead, width))
                changelog_.push_back({Heading, l, 0});
        } else if (body.rfind("- ", 0) == 0 || body.rfind("* ", 0) == 0) {
            const int indent = static_cast<int>(lead / 4) + 1;
            const auto lines = gfx.wrap(body.substr(2), fText, width - indent * 22);
            for (size_t i = 0; i < lines.size(); i++)
                changelog_.push_back({i == 0 ? Bullet : Continuation, lines[i], indent});
        } else {
            const int indent = static_cast<int>(lead / 4);
            for (const auto& l : gfx.wrap(body, fText, width - indent * 22))
                changelog_.push_back({Text, l, indent});
        }
    }
}

void App::drawChangelog() {
    if (!changelogBuilt_) buildChangelog();
    drawEyebrow(tr("changelog/eyebrow"), CONTENT_X, 27, col::accent);
    gfx.text(tr("changelog/title"), CONTENT_X, 40, col::text, gfx.font(40, true));

    gfx.fillRounded(LOG_X, LOG_Y, LOG_W, LOG_H, 16, col::card);
    gfx.strokeRounded(LOG_X, LOG_Y, LOG_W, LOG_H, 16, 1, col::cardBorder);
    const SDL_Rect clip{LOG_X + 1, LOG_Y + 12, LOG_W - 2, LOG_H - 24};
    gfx.clip(&clip);

    Font* fText = gfx.font(15);
    Font* fHead = gfx.font(19, true);
    const int n = static_cast<int>(changelog_.size());
    changelogScroll = std::clamp(changelogScroll, 0, std::max(0, n - 1));
    int y = LOG_Y + 18;
    for (int i = changelogScroll; i < n && y < LOG_Y + LOG_H; i++) {
        const ChangeLine& l = changelog_[i];
        const int x = LOG_X + LOG_PAD + l.indent * 22;
        switch (l.kind) {
            case Heading:
                gfx.text(l.text, LOG_X + LOG_PAD, y + 12, l.text.find("Version") == 0 ? col::text : col::textDim, fHead);
                break;
            case Bullet:
                gfx.disc(x - 12, y + fText->height / 2, 2, l.indent > 1 ? col::textMuted : col::accent);
                gfx.text(l.text, x, y, col::text, fText);
                break;
            case Continuation:
            case Text:
                gfx.text(l.text, x, y, l.kind == Text ? col::textDim : col::text, fText);
                break;
            default:
                break;
        }
        y += lineHeight(l.kind);
    }
    gfx.clip(nullptr);

    if (n > 1) {
        const int trackY = LOG_Y + 16, trackH = LOG_H - 32;
        const int thumbH = std::max(30, trackH / 6);
        const int thumbY = trackY + (trackH - thumbH) * changelogScroll / std::max(1, n - 1);
        gfx.fillRounded(LOG_X + LOG_W - 12, trackY, 3, trackH, 1, col::divider);
        gfx.fillRounded(LOG_X + LOG_W - 12, thumbY, 3, thumbH, 1, col::textMuted);
    }

    if (!sidebarFocus)
        drawFooter(SIDEBAR_W, {{"L R", tr("hints/page")}, {"B", tr("hints/back")}});
}

void App::inputChangelog(uint32_t pressed) {
    const int n = static_cast<int>(changelog_.size());
    // How far down it can go: the last lines filling the card, no further.
    int maxScroll = n - 1, used = 0;
    for (int i = n - 1; i >= 0; i--) {
        used += lineHeight(changelog_[i].kind);
        if (used > LOG_H - 36) { maxScroll = i + 1; break; }
        maxScroll = i;
    }
    maxScroll = std::max(0, maxScroll);
    if (pressed & BTN_UP) changelogScroll--;
    if (pressed & BTN_DOWN) changelogScroll++;
    if (pressed & BTN_L) changelogScroll -= 15;
    if (pressed & BTN_R) changelogScroll += 15;
    changelogScroll = std::clamp(changelogScroll, 0, maxScroll);
    if (pressed & (BTN_B | BTN_LEFT)) { sidebarFocus = true; sideCursor = dex::count() + 2; }
}
