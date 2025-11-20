#if defined(ANDROID) || defined(IOS)
#include <SDL2/SDL_main.h>
#endif

#include <borealis.hpp>
#include <cstdlib>
#include <string>

#include "update_checker.hpp"
#include "config.hpp"
#include "view/pokemon_view.hpp"
#include "tab/recycling_list_tab.hpp"
#include "tab/settings_tab.hpp"
#include "tab/changelog_tab.hpp"
#include "activity/main_activity.hpp"
#include "i18n.hpp"

#if defined(__PSV__) && defined(BOREALIS_USE_OPENGL)
// Needed for the OpenGL driver to work
extern "C" unsigned int sceLibcHeapSize = 2 * 1024 * 1024;
#endif

using namespace brls::literals; // for _i18n

// Set this to true to enable logging to file, false to disable
const bool ENABLE_FILE_LOGGING = true;

int main(int argc, char* argv[])
{
    // Set up logging to file if enabled
    if (ENABLE_FILE_LOGGING) {
        const char* logPath = "/switch/pkDex.log";
        FILE* logFile = std::fopen(logPath, "w+");
        if (logFile) {
            brls::Logger::setLogOutput(logFile);
            brls::Logger::info("Log file opened: {}", logPath);
        } else {
            brls::Logger::error("Failed to open log file: {}", logPath);
        }
    }

    // Set log level based on command line arguments
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "-d") == 0) { // Set log level
            brls::Logger::setLogLevel(brls::LogLevel::LOG_DEBUG);
        } else if (std::strcmp(argv[i], "-o") == 0) {
            // Skip the -o option since file logging is handled by ENABLE_FILE_LOGGING
            if (i + 1 < argc) i++; // Skip the next argument which would be the path
        } else if (std::strcmp(argv[i], "-v") == 0) {
            brls::Application::enableDebuggingView(true);
        }
    }

    // force initialize the locale for i18n - defaults to en-US if not set
    brls::Platform::APP_LOCALE_DEFAULT = pkdex::Config::getString("i18n_locale", "en-US");

    // Init the app and i18n
    if (!brls::Application::init())
    {
        brls::Logger::error("Unable to init Borealis application");
        return EXIT_FAILURE;
    }

    brls::Application::createWindow("pkdex/title"_i18n);

    brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);

    // Have the application register an action on every activity that will quit when you press BUTTON_START
    brls::Application::setGlobalQuit(false);

    // Register custom views (including tabs, which are views)
    brls::Application::registerXMLView("RecyclingListTab", static_cast<brls::View*(*)(void)>(RecyclingListTab::create));

    // Register region-specific factory methods
    brls::Application::registerXMLView("KantoTab", RecyclingListTab::createKanto);
    brls::Application::registerXMLView("SinnohTab", RecyclingListTab::createSinnoh);
    brls::Application::registerXMLView("SinnohArceusTab", RecyclingListTab::createSinnohArceus);
    brls::Application::registerXMLView("GalarTab", RecyclingListTab::createGalar);
    brls::Application::registerXMLView("IsleArmorTab", RecyclingListTab::createIsleArmor);
    brls::Application::registerXMLView("CrownTundraTab", RecyclingListTab::createCrownTundra);
    brls::Application::registerXMLView("PaldeaTab", RecyclingListTab::createPaldea);
    brls::Application::registerXMLView("KitakamiTab", RecyclingListTab::createKitakami);
    brls::Application::registerXMLView("BlueberryTab", RecyclingListTab::createBlueberry);
    brls::Application::registerXMLView("KalosLzaTab", RecyclingListTab::createKalosLza);

    brls::Application::registerXMLView("PokemonView", PokemonView::create);
    brls::Application::registerXMLView("SettingsTab", SettingsTab::create);
    brls::Application::registerXMLView("ChangelogTab", ChangelogTab::create);

    // Add custom values to the style
    brls::getStyle().addMetric("about/padding_top_bottom", 50);
    brls::getStyle().addMetric("about/padding_sides", 75);
    brls::getStyle().addMetric("about/description_margin", 50);

    // Apply the bottom bar visibility setting from config
    bool hideBottomBar = pkdex::Config::getBool("toggle_hide_bottom_bar", false);
    brls::AppletFrame::HIDE_BOTTOM_BAR = hideBottomBar;

    // Create and push the main activity to the stack
    brls::Application::pushActivity(new MainActivity());

    // Check for updates once at app launch, but after a delay to ensure the app is fully loaded
    brls::delay(5000, []() {
        // Check if version checking on launch is enabled
        if (pkdex::Config::getBool("toggle_check_version_on_launch", true)) {
            // Check for updates and notify if a new version is available
            checkForUpdatesAndNotify();
        }
    });

    // Run the app
    while (brls::Application::mainLoop())
        ;

    // Exit
    return EXIT_SUCCESS;
}

#ifdef __WINRT__
#include <borealis/core/main.hpp>
#endif
