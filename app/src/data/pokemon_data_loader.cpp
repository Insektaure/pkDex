#include "data/pokemon_data_loader.hpp"
#include <borealis/core/logger.hpp>
#include <borealis/core/i18n.hpp>
#include <fstream>
#include "data/pokemon_tracker.hpp"

std::vector<Pokemon> PokemonDataLoader::loadPokemonFromRegion(const std::string& region)
{
    std::vector<Pokemon> pokemons;

    // Load the JSON file
    std::string filePath = "romfs:/data/" + region + ".json";
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        brls::Logger::error("Failed to load Pokemon data from {}", filePath);
        return pokemons;
    }

    nlohmann::json doc;
    try
    {
        file >> doc;
    }
    catch (const std::exception& e)
    {
        brls::Logger::error("Failed to parse Pokemon data from {}: {}", filePath, e.what());
        return pokemons;
    }

    // Iterate through all Pokemon entries
    if (!doc.contains("pokemon") || !doc["pokemon"].is_array())
    {
        brls::Logger::error("Invalid Pokemon data file format: {}", filePath);
        return pokemons;
    }

    for (const auto& entry : doc["pokemon"])
    {
        Pokemon pokemon = parsePokemonNode(entry, region);
        pokemons.push_back(pokemon);
    }

    return pokemons;
}

Pokemon PokemonDataLoader::loadPokemonById(const std::string& id)
{
    // Try to find the Pokemon in each region
    std::vector<std::string> regions = pkdex::PokemonTracker::getAllRegions();

    for (const std::string& region : regions)
    {
        // Load the JSON file
        std::string filePath = "romfs:/data/" + region + ".json";
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            brls::Logger::error("Failed to load Pokemon data from {}", filePath);
            continue;
        }

        nlohmann::json doc;
        try
        {
            file >> doc;
        }
        catch (const std::exception& e)
        {
            brls::Logger::error("Failed to parse Pokemon data from {}: {}", filePath, e.what());
            continue;
        }

        if (!doc.contains("pokemon") || !doc["pokemon"].is_array())
        {
            brls::Logger::error("Invalid Pokemon data file format: {}", filePath);
            continue;
        }

        for (const auto& entry : doc["pokemon"])
        {
            std::string pokemonId = entry.value("id", "");
            if (pokemonId == id)
            {
                return parsePokemonNode(entry, region);
            }
        }
    }

    // If not found, return a default Pokemon
    brls::Logger::error("Pokemon with ID {} not found", id);
    return Pokemon(id, "Unknown");
}

Pokemon PokemonDataLoader::parsePokemonNode(const nlohmann::json& j, const std::string& region)
{
    std::string id = j.value("id", "");
    std::string name = j.value("name", "");
    std::string regionalDexNumber = j.value("regionalDexNumber", "");
    std::string type = j.value("type", "");
    bool shinyLocked = j.value("shinyLocked", false);
    std::string evolution = j.value("evolution", "");
    std::string exclusiveVersion = j.value("exclusiveVersion", "");
    std::string locations = j.value("locations", "");

    // Apply i18n overrides if available
    auto makeKey = [&](const std::string& field) {
        return "data/" + region + "/" + regionalDexNumber + "/" + field;
    };

    if (!regionalDexNumber.empty())
    {
        std::string o;
        o = brls::getStr(makeKey("name"));
        if (o != makeKey("name")) name = o;
        o = brls::getStr(makeKey("type"));
        if (o != makeKey("type")) type = o;
        o = brls::getStr(makeKey("evolution"));
        if (o != makeKey("evolution")) evolution = o;
        o = brls::getStr(makeKey("exclusiveVersion"));
        if (o != makeKey("exclusiveVersion")) exclusiveVersion = o;
        o = brls::getStr(makeKey("locations"));
        if (o != makeKey("locations")) locations = o;
    }

    return Pokemon(
        id,
        name,
        regionalDexNumber,
        type,
        evolution,
        exclusiveVersion,
        locations,
        shinyLocked
    );
}
