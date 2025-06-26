#include "view/pokemon_view.hpp"
#include "data/pokemon_tracker.hpp"
#include "tab/recycling_list_tab.hpp"

#include <borealis/core/i18n.hpp>
#include <borealis/core/logger.hpp>
#include <unistd.h>

using namespace brls::literals;

// Reference to the global pokemons vector defined in recycling_list_tab.cpp
extern std::vector<Pokemon> pokemons;

bool dismissView(brls::View* view, PokemonView* pock)
{
    return true;
}

PokemonView::PokemonView(Pokemon pokemon, int pokemonIndex, const std::string& region)
    : pokemon(pokemon), currentIndex(pokemonIndex), region(region)
{
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/views/pokemon.xml");

    auto dismissAction = [this](View* view) {
        this->dismiss();
        return true;
    };

    brls::Label* label = new brls::Label();
    label->setText(brls::Hint::getKeyIcon(brls::ControllerButton::BUTTON_RB) + " Close");
    label->setFontSize(24);
    label->setMargins(0, 12, 0, 12);

    brls::Box* holder = new brls::Box();
    holder->addView(label);
    holder->setFocusable(true);
    holder->addGestureRecognizer(new brls::TapGestureRecognizer(holder));

    holder->registerClickAction(dismissAction);
    holder->registerAction("Close", brls::ControllerButton::BUTTON_RB, dismissAction, true);
    registerAction("Close", brls::ControllerButton::BUTTON_RB, dismissAction, true);

    // Register ZL and ZR button actions for navigating between Pokemon
    this->registerAction("Previous Pokemon", brls::BUTTON_LT, 
        std::bind(&PokemonView::navigateToPreviousPokemon, this, std::placeholders::_1), false, true);
    this->registerAction("Next Pokemon", brls::BUTTON_RT, 
        std::bind(&PokemonView::navigateToNextPokemon, this, std::placeholders::_1), false, true);

    // Load the Pokemon data
    loadPokemon(pokemon);

    // Set up close button action
    close_button->registerClickAction([this](brls::View* view) {
        this->dismiss();
        return true;
    });
}

void PokemonView::loadPokemon(const Pokemon& newPokemon)
{
    // Update the current Pokemon
    this->pokemon = newPokemon;

    // Use the stored region instead of trying to get it from the parent
    // The region is set in the constructor

    // Check if the Pokemon is captured
    bool isCaptured = pkdex::PokemonTracker::isCaptured(region, pokemon.regionalDexNumber);

    // Update the applet frame with a checkmark if the Pokemon is captured
    std::string title = pokemon.name + " | N°" + pokemon.regionalDexNumber;
    if (isCaptured) {
        title = title + " [Captured]"; // Use brackets instead of emoji for better compatibility
    }

    getAppletFrameItem()->title = title;
    getAppletFrameItem()->setIconFromRes("img/pokemon/icons/" + pokemon.id + ".png");
    // Update the UI with the new title and icon
    updateAppletFrameItem();

    // Set images
    loadHighResImage(standard_image, "img/pokemon/full", pokemon.id);
    loadHighResImage(shiny_image, "img/pokemon/shiny", pokemon.id);

    // Set label texts
    standard_label->setText("");
    shiny_label->setText("");

    // Set text fields
    national_dex->setText(pokemon.id);
    regional_dex->setText(pokemon.regionalDexNumber);
    type->setText(pokemon.type);
    evolution->setText(pokemon.evolution);
    exclusive_version->setText(pokemon.exclusiveVersion);
    locations->setText(pokemon.locations);

    if (pokemon.shinyLocked) {
        shiny_locked->setText("Locked");
        shiny_locked->setTextColor(nvgRGB(255, 85, 85)); // Red color for emphasis
    } else {
        shiny_locked->setText("Available");
        shiny_locked->setTextColor(nvgRGB(85, 255, 85)); // Green color for available
    }
}

void PokemonView::loadHighResImage(brls::Image* image, const std::string& path, const std::string& id)
{
    std::string sdPath = "/switch/pkDex/resources/" + path + "/" + id + ".png";
    if (access(sdPath.c_str(), F_OK) != -1) {
        // File exists on SD card
        //brls::Logger::info("Loading high-res image from SD card: {}", sdPath);
        image->setImageFromFile(sdPath);
    } else {
        // Fall back to embedded resource
        //brls::Logger::info("High-res image not found on SD card, using embedded resource: {}", path + "/" + id + ".png");
        image->setImageFromRes(path + "/" + id + ".png");
    }
}

bool PokemonView::navigateToPreviousPokemon(brls::View* view)
{
    // If we don't have a valid index or there are no Pokemon, do nothing
    if (currentIndex < 0 || pokemons.empty())
        return false;

    // Calculate the previous index, wrapping around if necessary
    int previousIndex = (currentIndex > 0) ? currentIndex - 1 : pokemons.size() - 1;

    // Update the current index
    currentIndex = previousIndex;

    // Load the previous Pokemon
    loadPokemon(pokemons[currentIndex]);

    return true;
}

bool PokemonView::navigateToNextPokemon(brls::View* view)
{
    // If we don't have a valid index or there are no Pokemon, do nothing
    if (currentIndex < 0 || pokemons.empty())
        return false;

    // Calculate the next index, wrapping around if necessary
    int nextIndex = (currentIndex < pokemons.size() - 1) ? currentIndex + 1 : 0;

    // Update the current index
    currentIndex = nextIndex;

    // Load the next Pokemon
    loadPokemon(pokemons[currentIndex]);

    return true;
}

brls::View* PokemonView::create()
{
    // Called by the XML engine to create a new ComponentsTab
    return new PokemonView();
}
