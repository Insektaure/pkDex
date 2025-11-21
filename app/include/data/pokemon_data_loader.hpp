#pragma once

#include <string>
#include <vector>
#include "view/pokemon_view.hpp"

class PokemonDataLoader
{
public:
    // Load all Pokemon from a specific region (kanto, johto, hoenn)
    static std::vector<Pokemon> loadPokemonFromRegion(const std::string& region);
    
    // Load a specific Pokemon by ID
    static Pokemon loadPokemonById(const std::string& id);
    
private:
    // Helper method to parse a Pokemon XML node
    static Pokemon parsePokemonNode(tinyxml2::XMLElement* pokemonElement, const std::string& region);
};