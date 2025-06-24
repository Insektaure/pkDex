#pragma once

#include <borealis.hpp>

class Pokemon
{
  public:
    std::string id;
    std::string name;
    std::string regionalDexNumber;
    std::string type;
    std::string evolution;
    std::string exclusiveVersion;
    std::string locations;
    bool shinyLocked;

    Pokemon(std::string id, std::string name, std::string regionalDexNumber = "", std::string type = "", std::string evolution = "", std::string exclusiveVersion="", std::string locations = "", bool shinyLocked = false)
        : id(id)
        , name(name)
        , regionalDexNumber(regionalDexNumber)
        , type(type)
        , evolution(evolution)
        , exclusiveVersion(exclusiveVersion)
        , locations(locations)
        , shinyLocked(shinyLocked)
    {
    }
};

class PokemonView : public brls::Box
{
  public:
    PokemonView(Pokemon pokemon, int pokemonIndex = -1);
    PokemonView()
        : PokemonView(Pokemon("001", "Bulbasaur"), -1)
    {
    }

    static brls::View* create();

  private:
    Pokemon pokemon;
    int currentIndex; // Index of the current Pokemon in the global pokemons vector

    // Methods for navigating between Pokemon
    bool navigateToPreviousPokemon(brls::View* view);
    bool navigateToNextPokemon(brls::View* view);
    void loadPokemon(const Pokemon& pokemon);
    void loadHighResImage(brls::Image* image, const std::string& path, const std::string& id);
    BRLS_BIND(brls::Label, standard_label, "standard_label");
    BRLS_BIND(brls::Image, standard_image, "standard_image");
    BRLS_BIND(brls::Label, shiny_label, "shiny_label");
    BRLS_BIND(brls::Image, shiny_image, "shiny_image");
    BRLS_BIND(brls::Label, national_dex, "national_dex");
    BRLS_BIND(brls::Label, regional_dex, "regional_dex");
    BRLS_BIND(brls::Label, type, "type");
    BRLS_BIND(brls::Label, evolution, "evolution");
    BRLS_BIND(brls::Label, exclusive_version, "exclusive_version");
    BRLS_BIND(brls::Label, locations, "locations");
    BRLS_BIND(brls::Label, shiny_locked, "shiny_locked");
    BRLS_BIND(brls::Button, close_button, "close_button");
    BRLS_BIND(brls::Box, info_box, "info_box");
};
