// The popups ("Capture status", "Confirm reset", "Select
// region to reset", "Updater").

#include "app.h"
#include "i18n.h"
#include "theme.h"
#include "util.h"

#include <algorithm>

namespace {

constexpr Uint8 SHADE = 165;
const SDL_Color WHITE{255, 255, 255, 255};

void card(int x, int y, int w, int h, int radius = 22) {
    gfx.fillRounded(x - 6, y - 4, w + 12, h + 16, radius + 6, SDL_Color{0, 0, 0, 70});   // shadow
    gfx.fillRounded(x, y, w, h, radius, col::modal);
    gfx.strokeRounded(x, y, w, h, radius, 1, col::modalBorder);
}

std::string stripDevice(const std::string& path) {
    const size_t colon = path.find(':');
    return colon == std::string::npos ? path : path.substr(colon + 1);
}

} // anonymous namespace

// --- capture status ----------------------------------------------------------------

void CaptureModal::draw(App& app) {
    gfx.shade(SHADE);
    const auto& list = dex::list(region);
    if (index < 0 || index >= static_cast<int>(list.size())) return;
    const Pokemon& p = list[index];
    const std::vector<int> states = app.statesOf(region);
    const int k = static_cast<int>(states.size());
    const int w = 520, h = 170 + 64 * k;
    const int x = (SCREEN_W - w) / 2, y = (SCREEN_H - h) / 2;
    card(x, y, w, h);

    app.drawPokemonThumb(p, x + 24, y + 24, 52, 12);
    gfx.textMid(tr("capture/title"), x + 92, y + 40, col::text, gfx.font(24, true));
    Font* fSub = gfx.font(14);
    gfx.textMid(gfx.fit("#" + p.regional + " " + p.name + " · " + dex::game(region), fSub, w - 116), x + 92, y + 66,
                col::textDim, fSub);

    const Capture cap = tracker::get(dex::regions()[region].id, p.regional);
    Font* fLabel = gfx.font(17, true);
    Font* fState = gfx.font(13);
    for (int i = 0; i < k; i++) {
        const int ry = y + 94 + i * 64, rx = x + 24, rw = w - 48;
        const int cy = ry + 32;
        if (i == cursor) {
            app.drawFocusRing(rx, ry + 2, rw, 60, 12);
            gfx.fillRounded(rx, ry + 2, rw, 60, 12, col::rowSel);
        }
        app.drawStateBadge(states[i], rx + 12, cy - 18, 36);
        gfx.textMid(app.stateLabel(states[i]), rx + 62, cy, col::text, fLabel);
        const bool on = cap.get(states[i]);
        app.drawToggle(rx + rw - 60, cy, on);
        gfx.textRight(tr(on ? "common/on" : "common/off"), rx + rw - 72, cy, col::textDim, fState);
    }

    const int fy = y + h - 61;
    gfx.rect(x + 24, fy, w - 48, 1, col::rowSep);
    app.drawHints(x + w - 24, fy + 31, {{"A", tr("hints/toggle")}, {"B", tr("hints/done")}}, col::text);
}

void CaptureModal::input(App& app, uint32_t pressed) {
    const std::vector<int> states = app.statesOf(region);
    const int k = static_cast<int>(states.size());
    if (pressed & BTN_UP) cursor = (cursor + k - 1) % k;
    if (pressed & BTN_DOWN) cursor = (cursor + 1) % k;
    if (pressed & BTN_A) {
        const auto& list = dex::list(region);
        if (index >= 0 && index < static_cast<int>(list.size())) {
            const char* id = dex::regions()[region].id;
            Capture cap = tracker::get(id, list[index].regional);
            cap.set(states[cursor], !cap.get(states[cursor]));
            tracker::set(id, list[index].regional, cap);
        }
    }
    if (pressed & (BTN_B | BTN_Y)) closed = true;
}

// --- action list -------------------------------------------------------------------

ActionModal::ActionModal(std::string t, std::string s, std::vector<Item> i)
    : title(std::move(t)), subtitle(std::move(s)), items(std::move(i)) {
    cursor = 0;
    while (cursor < static_cast<int>(items.size()) && items[cursor].header) cursor++;
}

void ActionModal::draw(App& app) {
    gfx.shade(SHADE);
    constexpr int HEAD = 34, ROW = 48;
    int listH = 0;
    for (const Item& it : items) listH += it.header ? HEAD : ROW;
    const int w = 520, h = 96 + listH + 16 + 60;
    const int x = (SCREEN_W - w) / 2, y = std::max(10, (SCREEN_H - h) / 2);
    card(x, y, w, h);

    gfx.textMid(title, x + 28, y + 40, col::text, gfx.font(24, true));
    Font* fSub = gfx.font(14);
    gfx.textMid(gfx.fit(subtitle, fSub, w - 56), x + 28, y + 68, col::textDim, fSub);

    Font* fLabel = gfx.font(16, true);
    Font* fDetail = gfx.font(13);
    int ry = y + 92;
    for (size_t i = 0; i < items.size(); i++) {
        const Item& it = items[i];
        if (it.header) {
            app.drawEyebrow(it.label, x + 30, ry + 12, col::textMuted);
            ry += HEAD;
            continue;
        }
        const int rx = x + 20, rw = w - 40, cy = ry + ROW / 2;
        if (static_cast<int>(i) == cursor) {
            app.drawFocusRing(rx, ry + 2, rw, ROW - 4, 10);
            gfx.fillRounded(rx, ry + 2, rw, ROW - 4, 10, col::rowSel);
        }
        int lx = rx + 14;
        if (it.state >= 0) {
            app.drawStateBadge(it.state, lx, cy - 14, 28);
            lx += 40;
        }
        const int dw = it.detail.empty() ? 0 : gfx.textRight(it.detail, rx + rw - 14, cy, col::textDim, fDetail) + 12;
        gfx.textMid(gfx.fit(it.label, fLabel, rx + rw - 14 - dw - lx), lx, cy, col::text, fLabel);
        ry += ROW;
    }

    const int fy = y + h - 61;
    gfx.rect(x + 24, fy, w - 48, 1, col::rowSep);
    app.drawHints(x + w - 24, fy + 31, {{"B", tr("hints/back")}, {"A", tr("hints/apply")}}, col::text);
}

void ActionModal::input(App& app, uint32_t pressed) {
    const int n = static_cast<int>(items.size());
    auto step = [&](int dir) {
        for (int c = cursor + dir; c >= 0 && c < n; c += dir)
            if (!items[c].header) { cursor = c; return; }
    };
    if (pressed & BTN_UP) step(-1);
    if (pressed & BTN_DOWN) step(+1);
    if (pressed & BTN_B) { closed = true; return; }
    if ((pressed & BTN_A) && cursor >= 0 && cursor < n && items[cursor].run) {
        closed = true;
        auto run = items[cursor].run;   // a copy: the popup it opens may outlive this one
        run(app);
    }
}

// --- confirm -------------------------------------------------------------------------

void ConfirmModal::draw(App& app) {
    gfx.shade(SHADE);
    const int w = 480, pad = 28;
    Font* fTitle = gfx.font(24, true);
    Font* fBody = gfx.font(15);
    // With a text, the mockup's layout: icon, then title, then text. A bare
    // question ("Quit pkDex?") keeps its title beside the icon instead.
    const bool beside = body.empty();
    const int titleX = beside ? pad + 52 + 18 : pad;
    const auto titleLines = gfx.wrap(title, fTitle, w - titleX - pad, 3);
    const auto bodyLines = body.empty() ? std::vector<std::string>() : gfx.wrap(body, fBody, w - 2 * pad, 8);
    const int titleH = static_cast<int>(titleLines.size()) * 32;
    const int head = beside ? std::max(52, titleH) : 52 + 18 + titleH + 6 + static_cast<int>(bodyLines.size()) * 24;
    const int h = pad + head + 24 + 52 + pad;
    const int x = (SCREEN_W - w) / 2, y = (SCREEN_H - h) / 2;
    card(x, y, w, h);

    SDL_Color tint = col::accent;
    Icon icon = Icon::Warning;
    switch (kind) {
        case Warning: tint = col::accent; icon = Icon::Warning; break;
        case Info:    tint = col::textDim; icon = Icon::Info; break;
        case Success: tint = col::green; icon = Icon::Check; break;
        case Error:   tint = col::accent; icon = Icon::Cross; break;
    }
    gfx.fillRounded(x + pad, y + pad, 52, 52, 13, mix(col::modal, tint, 0.18f));
    gfx.icon(icon, x + pad + 26, y + pad + 26, 26, tint);

    int ty = beside ? y + pad + (52 - titleH) / 2 : y + pad + 52 + 18;
    for (const auto& l : titleLines) { gfx.text(l, x + titleX, ty, col::text, fTitle); ty += 32; }
    ty += 6;
    for (const auto& l : bodyLines) { gfx.text(l, x + pad, ty, col::textDim, fBody); ty += 24; }

    const int by = y + h - pad - 52;
    Font* fBtn = gfx.font(16, true);
    auto button = [&](int bx, int bw, const std::string& label, bool primary, bool focused, const char* key) {
        const SDL_Color bg = primary ? (danger ? col::dangerBtn : col::text) : col::button;
        const SDL_Color fg = primary ? (danger ? WHITE : col::bg) : col::text;
        if (focused) app.drawFocusRing(bx, by, bw, 52, 12);
        gfx.fillRounded(bx, by, bw, 52, 12, bg);
        const int kw = key ? app.keyWidth(key) + 10 : 0;
        const std::string l = gfx.fit(label, fBtn, bw - 24 - kw);
        const int lw = gfx.textW(l, fBtn);
        int lx = bx + (bw - lw - kw) / 2;
        if (key) lx += app.drawKey(key, lx, by + 26) + 10;
        gfx.textMid(l, lx, by + 26, fg, fBtn);
    };
    if (single) {
        button(x + pad, w - 2 * pad, confirmLabel, true, true, nullptr);
    } else {
        const int bw = (w - 2 * pad - 12) / 2;
        button(x + pad, bw, cancelLabel, false, focus == 0, "B");
        button(x + pad + bw + 12, bw, confirmLabel, true, focus == 1, nullptr);
    }
}

void ConfirmModal::input(App& app, uint32_t pressed) {
    if (!single) {
        if (pressed & BTN_LEFT) focus = 0;
        if (pressed & BTN_RIGHT) focus = 1;
    }
    const bool confirm = (pressed & BTN_A) && (single || focus == 1);
    const bool cancel = (pressed & BTN_B) || ((pressed & BTN_A) && !single && focus == 0);
    if (confirm) {
        closed = true;
        auto f = onConfirm;   // a copy: it may push the next popup
        if (f) f(app);
    } else if (cancel) {
        closed = true;
        auto f = single ? onConfirm : onCancel;
        if (f) f(app);
    }
}

// --- drawer --------------------------------------------------------------------------

void DrawerModal::draw(App& app) {
    gfx.shade(SHADE);
    const int x = 820, w = SCREEN_W - x;
    gfx.rect(x, 0, w, SCREEN_H, col::drawer);
    gfx.rect(x, 0, 1, SCREEN_H, col::modalBorder);
    app.drawEyebrow(eyebrow, x + 29, 28, col::textDim);
    Font* fTitle = gfx.font(26, true);
    gfx.text(gfx.fit(title, fTitle, w - 58), x + 29, 44, col::text, fTitle);

    constexpr int TOP = 90, ROW = 34, VISIBLE = (656 - TOP) / ROW;
    const int n = static_cast<int>(items.size());
    // The cursor in view, and the heading of its group with it.
    const int want = (cursor > 0 && items[cursor - 1].heading) ? cursor - 1 : cursor;
    if (want < scroll) scroll = want;
    if (cursor >= scroll + VISIBLE) scroll = cursor - VISIBLE + 1;
    Font* fHeading = gfx.font(15, true);
    Font* fTop = gfx.font(16, true);
    Font* fChild = gfx.font(15);
    Font* fTag = gfx.font(13);
    Font* fDlc = gfx.font(9, true);
    for (int i = scroll; i < n && i < scroll + VISIBLE; i++) {
        const Item& it = items[i];
        const int ry = TOP + (i - scroll) * ROW, cy = ry + ROW / 2;
        if (it.heading) {
            gfx.textMid(gfx.fit(it.label, fHeading, w - 58), x + 29, cy + 3, col::textDim, fHeading);
            continue;
        }
        const int rx = x + 20, rw = w - 40;
        if (i == cursor) {
            app.drawFocusRing(rx, ry + 1, rw, ROW - 2, 10);
            gfx.fillRounded(rx, ry + 1, rw, ROW - 2, 10, col::sideSel);
        }
        const int rcx = x + (it.child ? 58 : 44);
        if (i == selected) {
            gfx.ring(rcx, cy, 9, 2, col::accent);
            gfx.disc(rcx, cy, 4, col::accent);
        } else {
            gfx.ring(rcx, cy, 9, 2, SDL_Color{0x6a, 0x70, 0x7b, 255});
        }
        int right = x + w - 34;
        if (!it.tag.empty()) right -= gfx.textRight(it.tag, right, cy, col::textDim, fTag) + 12;
        if (it.dlc) {
            const std::string tag = tr("sidebar/dlc");
            const int tw = gfx.textW(tag, fDlc) + 12;
            right -= tw;
            gfx.fillRounded(right, cy - 8, tw, 16, 4, col::chip);
            gfx.textCenter(tag, right + tw / 2, cy, col::textDim, fDlc);
            right -= 12;
        }
        gfx.marquee(it.label, rcx + 21, cy, right - (rcx + 21), col::text, it.child ? fChild : fTop, i == cursor);
    }

    gfx.rect(x + 23, 667, w - 46, 1, col::modalBorder);
    app.drawHints(SCREEN_W - 28, 692, {{"B", tr("hints/back")}, {"A", tr("hints/choose")}}, col::text);
}

void DrawerModal::input(App& app, uint32_t pressed) {
    const int n = static_cast<int>(items.size());
    auto step = [&](int dir) {
        for (int c = cursor + dir; c >= 0 && c < n; c += dir)
            if (!items[c].heading) { cursor = c; return; }
    };
    if (n == 0) { closed = true; return; }
    if (pressed & BTN_UP) step(-1);
    if (pressed & BTN_DOWN) step(+1);
    if (pressed & BTN_B) { closed = true; return; }
    if ((pressed & BTN_A) && !items[cursor].heading) {
        selected = cursor;
        closed = true;
        const int value = items[cursor].value >= 0 ? items[cursor].value : cursor;
        auto f = onChoose;
        if (f) f(app, value);
    }
}

// --- progress ------------------------------------------------------------------------

void ProgressModal::update(App& app) {
    if (closed || !job || !job->finished.load()) return;
    closed = true;
    auto f = onDone;
    if (f) f(app, job->ok.load(), job->message());
}

void ProgressModal::draw(App& app) {
    gfx.shade(SHADE);
    const int w = 520, h = cancellable ? 220 : 180;
    const int x = (SCREEN_W - w) / 2, y = (SCREEN_H - h) / 2;
    card(x, y, w, h);

    gfx.fillRounded(x + 28, y + 28, 48, 48, 13, mix(col::modal, col::accent, 0.15f));
    app.drawSpinner(x + 52, y + 52, 12, col::accent);
    Font* fTitle = gfx.font(20, true);
    gfx.textMid(gfx.fit(title, fTitle, w - 120), x + 92, y + 52, col::text, fTitle);

    const float p = job ? job->progress.load() : -1.0f;
    const int by = y + 104, bw = w - 56;
    gfx.fillRounded(x + 28, by, bw, 8, 4, col::chip);
    if (p >= 0.0f) {
        gfx.fillRounded(x + 28, by, std::max(8, static_cast<int>(bw * std::clamp(p, 0.0f, 1.0f))), 8, 4, col::accent);
    } else {
        // Unknown length: a segment going back and forth.
        const float t = static_cast<float>(SDL_GetTicks() % 1600) / 1600.0f;
        const float k = t < 0.5f ? t * 2.0f : 2.0f - t * 2.0f;
        gfx.fillRounded(x + 28 + static_cast<int>((bw - 90) * k), by, 90, 8, 4, col::accent);
    }

    Font* f = gfx.font(14);
    std::string left = p >= 0.0f ? std::to_string(static_cast<int>(p * 100.0f + 0.5f)) + "%" : "";
    if (showBytes && job && job->total.load() > 0)
        left += (left.empty() ? "" : " · ") + util::formatMB(job->done.load()) + " / " + util::formatMB(job->total.load());
    else if (!showBytes && job && job->total.load() > 0)
        left += (left.empty() ? "" : " · ") + std::to_string(job->done.load()) + " / " + std::to_string(job->total.load());
    gfx.textMid(left, x + 28, by + 32, col::textDim, f);

    if (cancellable) {
        gfx.rect(x + 24, y + h - 61, w - 48, 1, col::rowSep);
        app.drawHints(x + w - 24, y + h - 30, {{"B", tr("common/cancel")}}, col::text);
    }
}

void ProgressModal::input(App&, uint32_t pressed) {
    if (cancellable && (pressed & BTN_B) && job) job->cancel = true;
}

// --- the updater ---------------------------------------------------------------------

void UpdateModal::update(App& app) {
    if (!job || !job->finished.load() || !job->ok.load()) return;
    const uint32_t now = SDL_GetTicks();
    if (doneAt == 0) doneAt = now ? now : 1;
    if (now - doneAt >= 5000) {
        Update::restartInto(Update::installPath());
        app.quit = true;
    }
}

void UpdateModal::draw(App& app) {
    gfx.rect(0, 0, SCREEN_W, SCREEN_H, SDL_Color{0x0b, 0x0d, 0x10, 255});
    gfx.dotGrid(SDL_Rect{0, 0, SCREEN_W, SCREEN_H}, col::dotGrid);

    const bool finished = job && job->finished.load();
    const bool ok = finished && job->ok.load();
    const int stage = job ? job->stage.load() : 0;
    const std::string version = Update::latestVersion();

    const int x = 330, y = 116, w = 620, h = 488;
    card(x, y, w, h, 24);
    app.drawLogo(x + 33, y + 33, 44);
    gfx.text(tr("update/screen_title"), x + 91, y + 30, col::text, gfx.font(22, true));
    const std::string sub = ok ? tr("update/sub_done") : finished ? tr("update/sub_failed")
                               : trf("update/sub_running", {{"version", version}});
    gfx.text(sub, x + 91, y + 62, col::textDim, gfx.font(14));

    static const char* labels[Update::StepCount] = {
        "update/step_download", "update/step_unpack", "update/step_verify", "update/step_backup", "update/step_install", "update/step_cleanup",
    };
    Font* fStep = gfx.font(15, true);
    Font* fSub = gfx.font(12);
    const int sx = x + 45, sy0 = y + 115, pitch = 50;
    for (int i = 0; i < Update::StepCount; i++) {
        const int cy = sy0 + i * pitch;
        const bool done = i < stage || ok;
        const bool failed = finished && !ok && i == stage;
        const bool active = !finished && i == stage;
        if (i + 1 < Update::StepCount)
            gfx.rect(sx, cy + 14, 1, pitch - 28, done ? mix(col::modal, col::green, 0.4f) : col::modalBorder);
        if (done) {
            gfx.disc(sx, cy, 12, col::green);
            gfx.icon(Icon::Check, sx, cy, 14, col::bg);
        } else if (failed) {
            gfx.disc(sx, cy, 12, col::accent);
            gfx.icon(Icon::Cross, sx, cy, 12, col::bg);
        } else if (active) {
            app.drawSpinner(sx, cy, 11, col::accent);
        } else {
            gfx.ring(sx, cy, 11, 2, col::textFaint);
        }

        std::string detail;
        switch (i) {
            case Update::StepDownload:
                if (active && job && job->total.load() > 0)
                    detail = util::formatMB(job->done.load()) + " / " + util::formatMB(job->total.load());
                else
                    detail = Update::assetName() + " · v" + version;
                break;
            case Update::StepUnpack:  detail = "pkDex.nro"; break;
            case Update::StepVerify:  detail = "NACP · v" + version; break;
            case Update::StepBackup:
                // Moving an old install: the copy running now is the backup.
                detail = stripDevice(Update::installsInPlace(app.exePath) ? Update::backupPath() : app.exePath);
                break;
            case Update::StepInstall: detail = stripDevice(Update::installPath()); break;
            default: break;
        }
        const SDL_Color lc = (done || active || failed) ? col::text : col::textMuted;
        if (detail.empty()) {
            gfx.textMid(tr(labels[i]), sx + 26, cy, lc, fStep);
        } else {
            gfx.textMid(tr(labels[i]), sx + 26, cy - 8, lc, fStep);
            gfx.textMid(gfx.fit(detail, fSub, w - 120), sx + 26, cy + 12, col::textMuted, fSub);
        }
    }

    // The result, or while it runs, the warning not to switch off.
    const int bx = x + 33, by = y + h - 95, bw = w - 66, bh = 66;
    Font* fMsg = gfx.font(15, true);
    if (ok) {
        gfx.fillRounded(bx, by, bw, bh, 14, mix(col::modal, col::green, 0.12f));
        gfx.textMid(tr("update/success"), bx + 18, by + 26, mix(col::green, WHITE, 0.2f), fMsg);
        const uint32_t elapsed = doneAt ? SDL_GetTicks() - doneAt : 0;
        const int left = std::max(0, 5 - static_cast<int>(elapsed / 1000));
        gfx.textRight(trf("update/restarting", {{"seconds", std::to_string(left)}}), bx + bw - 18, by + 26,
                      col::textDim, gfx.font(13));
        const float k = std::clamp(static_cast<float>(elapsed) / 5000.0f, 0.0f, 1.0f);
        gfx.fillRounded(bx + 18, by + 46, bw - 36, 4, 2, mix(col::modal, col::green, 0.3f));
        gfx.fillRounded(bx + 18, by + 46, std::max(4, static_cast<int>((bw - 36) * k)), 4, 2, col::green);
    } else if (finished) {
        gfx.fillRounded(bx, by, bw, bh, 14, mix(col::modal, col::accent, 0.12f));
        const int hw = app.hintsWidth({{"B", tr("common/close")}});
        gfx.textMid(tr("update/failure"), bx + 18, by + 22, col::danger, fMsg);
        Font* f = gfx.font(12);
        gfx.textMid(gfx.fit(job->message() + " · " + tr("update/intact"), f, bw - 54 - hw), bx + 18, by + 45,
                    col::textDim, f);
        app.drawHints(bx + bw - 18, by + bh / 2, {{"B", tr("common/close")}}, col::text);
    } else {
        gfx.fillRounded(bx, by, bw, bh, 14, col::chip);
        gfx.icon(Icon::Warning, bx + 30, by + bh / 2, 20, col::gold);
        gfx.textMid(gfx.fit(tr("update/keep_on"), gfx.font(14), bw - 160), bx + 54, by + bh / 2, col::text, gfx.font(14));
        const float p = job ? job->progress.load() : -1.0f;
        if (stage == Update::StepDownload && p >= 0.0f)
            gfx.textRight(std::to_string(static_cast<int>(p * 100.0f + 0.5f)) + "%", bx + bw - 18, by + bh / 2,
                          col::textDim, gfx.font(14, true));
    }
}

void UpdateModal::input(App& app, uint32_t pressed) {
    if (!job) { closed = true; return; }
    const bool finished = job->finished.load();
    if (finished && job->ok.load()) {
        if (pressed & BTN_A) {
            Update::restartInto(Update::installPath());
            app.quit = true;
        }
        return;
    }
    if (finished) {
        if (pressed & (BTN_A | BTN_B)) closed = true;
        return;
    }
    // Only the download can be stopped: after it, stopping is what would hurt.
    if ((pressed & BTN_B) && job->stage.load() == Update::StepDownload) job->cancel = true;
}
