#pragma once
#include "dex.h"
#include "gfx.h"
#include "input.h"
#include "job.h"
#include "tracker.h"
#include "update.h"

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

class App;

// A button and what it does, for the footer and the popups' own hint rows.
// The key is a button name ("A", "ZL", "+"), or two separated by a space
// ("L R") for a pair of caps.
struct Hint { std::string key; std::string label; };

// Anything drawn over the page: a dialog, a drawer, the updater. The top one
// gets the input; all of them are drawn, bottom first.
struct Modal {
    virtual ~Modal() = default;
    virtual void update(App&) {}
    virtual void draw(App&) = 0;
    virtual void input(App&, uint32_t pressed) = 0;
    // Covers the whole screen: nothing under it is drawn.
    virtual bool fullscreen() const { return false; }
    bool closed = false;
};

enum class Page { Dex, Detail, Settings, About, Changelog };

// Layout of the main screen.
namespace layout {
constexpr int SIDEBAR_W = 288;
constexpr int CONTENT_X = 328;
constexpr int CONTENT_R = 1240;
constexpr int FOOTER_Y  = 668;
constexpr int FOOTER_H  = SCREEN_H - FOOTER_Y;
constexpr int NAV_COUNT = 3;   // Settings, About, Changelog under the regions
} // namespace layout

class App {
public:
    int run(int argc, char** argv);

    // --- for the pages and the popups --------------------------------------------
    void push(std::unique_ptr<Modal> m);
    void toast(const std::string& text, bool error = false);

    void drawFooter(int left, const std::vector<Hint>& hints);
    int hintsWidth(const std::vector<Hint>& hints);
    void drawHints(int right, int cy, const std::vector<Hint>& hints, SDL_Color label);
    int keyWidth(const std::string& key);
    int drawKey(const std::string& key, int x, int cy, bool dark = false);   // returns the width
    void drawToggle(int x, int cy, bool on);
    void drawLogo(int x, int y, int size);
    void drawEyebrow(const std::string& s, int x, int y, SDL_Color c);
    void drawFocusRing(int x, int y, int w, int h, int radius);
    void drawStateIcon(int state, int cx, int cy, int size);
    void drawStateBadge(int state, int x, int y, int box);
    void drawPokemonThumb(const Pokemon& p, int x, int y, int box, int radius);
    void drawSpinner(int cx, int cy, int radius, SDL_Color c);

    const tracker::Counts& counts(int region);
    std::string stateLabel(int state) const;
    std::vector<int> statesOf(int region) const;   // Regular and Shiny, plus the Alpha ones

    // --- actions the popups lead to ----------------------------------------------
    void confirmQuit();
    void openCapture(int region, int index);
    void openBulk();
    void openMultiApply();
    void openRegionReset();
    void confirmReset();
    void openLanguage();
    void setLanguage(const std::string& code);
    void checkForUpdates(bool manual);
    void offerUpdate();
    void startUpdate();
    void startPackDownload(bool confirmed);
    void startPackExtract(bool confirmed);
    bool runJob(std::function<void(Job&)> fn);

    // --- state ----------------------------------------------------------------------
    Page page = Page::Dex;
    bool sidebarFocus = false;
    int sideCursor = 0;   // regions, then the three nav buttons
    int region = 0;

    struct DexState { int cursor = 0; int scroll = 0; bool multi = false; std::set<int> selected; };
    std::vector<DexState> dexStates;

    int detailIndex = 0;
    bool detailShiny = false;
    int detailLocRow = 0;   // first visible row of location pills (Up/Down scroll them)

    int settingsCursor = 0;
    int changelogScroll = 0;

    std::string exePath;
    bool quit = false;

    Worker worker;
    std::shared_ptr<Job> job;

private:
    void loadData();
    void drawLoading(float progress);
    void frame();
    void handleInput(uint32_t pressed);
    void watchUpdate();

    void drawSidebar();
    void inputSidebar(uint32_t pressed);
    void selectSide(int index);

    void drawDex();
    void inputDex(uint32_t pressed);
    void ensureVisible();

    void drawDetail();
    void inputDetail(uint32_t pressed);

    void drawSettings();
    void inputSettings(uint32_t pressed);
    void activateSetting(int row);

    void drawAbout();
    void drawChangelog();
    void inputChangelog(uint32_t pressed);
    void buildChangelog();

    void drawToasts();

    struct Toast { std::string text; bool error; uint32_t until; };
    std::vector<Toast> toasts_;
    std::vector<std::unique_ptr<Modal>> modals_;

    std::vector<tracker::Counts> counts_;
    unsigned countsVersion_ = 0;

    struct ChangeLine { int kind; std::string text; int indent; };   // kind: 0 text, 1 heading, 2 bullet, 3 gap
    std::vector<ChangeLine> changelog_;
    bool changelogBuilt_ = false;

    Update::State lastUpdateState_ = Update::State::Idle;
    bool manualCheck_ = false;
    bool updateToastShown_ = false;

    int battery_ = 100;
    bool charging_ = false;
    uint32_t batteryAt_ = 0;

    Input input_;
};

// --- the popups -------------------------------------------------------------------

// The capture states of one Pokemon, toggled in place ("Capture status").
struct CaptureModal : Modal {
    CaptureModal(int region, int index) : region(region), index(index) {}
    void draw(App& app) override;
    void input(App& app, uint32_t pressed) override;
    int region, index, cursor = 0;
};

// A list of actions, in groups: bulk actions, the multi-select actions.
struct ActionModal : Modal {
    struct Item {
        std::string label;
        std::string detail;
        int state = -1;          // capture state icon, or -1
        bool header = false;     // a group title, not selectable
        std::function<void(App&)> run;
    };
    ActionModal(std::string title, std::string subtitle, std::vector<Item> items);
    void draw(App& app) override;
    void input(App& app, uint32_t pressed) override;
    std::string title, subtitle;
    std::vector<Item> items;
    int cursor = 0, scroll = 0;
};

// A question with a cancel and a confirm button, or a message with one.
struct ConfirmModal : Modal {
    enum Kind { Warning, Info, Success, Error };
    Kind kind = Warning;
    std::string title, body;
    std::string cancelLabel, confirmLabel;
    bool danger = false;    // the confirm button in red
    bool single = false;    // only the confirm button (a message)
    int focus = 0;          // 0 cancel, 1 confirm
    std::function<void(App&)> onConfirm;
    std::function<void(App&)> onCancel;
    void draw(App& app) override;
    void input(App& app, uint32_t pressed) override;
};

// A list of choices sliding in from the right ("Select region to reset"), one of them marked as current.
struct DrawerModal : Modal {
    struct Item { std::string label, tag; bool child = false; };
    std::string eyebrow, title;
    std::vector<Item> items;
    int selected = 0, cursor = 0, scroll = 0;
    std::function<void(App&, int)> onChoose;
    void draw(App& app) override;
    void input(App& app, uint32_t pressed) override;
};

// A worker's progress: the image pack's download or extraction.
struct ProgressModal : Modal {
    std::string title;
    std::shared_ptr<Job> job;
    bool cancellable = false;
    bool showBytes = false;
    std::function<void(App&, bool ok, const std::string& message)> onDone;
    void update(App& app) override;
    void draw(App& app) override;
    void input(App& app, uint32_t pressed) override;
};

// The self-update, step by step ("Updater").
struct UpdateModal : Modal {
    explicit UpdateModal(std::shared_ptr<Job> job) : job(std::move(job)) {}
    void update(App& app) override;
    void draw(App& app) override;
    void input(App& app, uint32_t pressed) override;
    bool fullscreen() const override { return true; }
    std::shared_ptr<Job> job;
    uint32_t doneAt = 0;
};
