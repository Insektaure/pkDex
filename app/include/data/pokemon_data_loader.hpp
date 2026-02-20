#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "view/pokemon_view.hpp"

class PokemonDataLoader
{
public:
    // Load all Pokemon from a specific region (kanto, johto, hoenn)
    static std::vector<Pokemon> loadPokemonFromRegion(const std::string& region);

    // Load a specific Pokemon by ID
    static Pokemon loadPokemonById(const std::string& id);

private:
    // Helper method to parse a Pokemon JSON object
    static Pokemon parsePokemonNode(const nlohmann::json& j, const std::string& region);
};
