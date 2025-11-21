#include "data/pokemon_data_loader.hpp"
#include <borealis/core/logger.hpp>
#include <borealis/core/i18n.hpp>
#include <tinyxml2.h>
#include "data/pokemon_tracker.hpp"

std::vector<Pokemon> PokemonDataLoader::loadPokemonFromRegion(const std::string& region)
{
    std::vector<Pokemon> pokemons;

    // Load the XML file
    tinyxml2::XMLDocument doc;
    std::string filePath = "romfs:/data/" + region + ".xml";
    tinyxml2::XMLError error = doc.LoadFile(filePath.c_str());

    if (error != tinyxml2::XML_SUCCESS)
    {
        brls::Logger::error("Failed to load Pokemon data from {}: {}", filePath, doc.ErrorStr());
        return pokemons;
    }

    // Get the root element
    tinyxml2::XMLElement* root = doc.FirstChildElement("pokedex");
    if (!root)
    {
        brls::Logger::error("Invalid Pokemon data file format: {}", filePath);
        return pokemons;
    }

    // Iterate through all Pokemon elements
    for (tinyxml2::XMLElement* pokemonElement = root->FirstChildElement("pokemon");
         pokemonElement;
         pokemonElement = pokemonElement->NextSiblingElement("pokemon"))
    {
        Pokemon pokemon = parsePokemonNode(pokemonElement, region);
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
        // Load the XML file
        tinyxml2::XMLDocument doc;
        std::string filePath = "romfs:/data/" + region + ".xml";
        tinyxml2::XMLError error = doc.LoadFile(filePath.c_str());

        if (error != tinyxml2::XML_SUCCESS)
        {
            brls::Logger::error("Failed to load Pokemon data from {}: {}", filePath, doc.ErrorStr());
            continue;
        }

        // Get the root element
        tinyxml2::XMLElement* root = doc.FirstChildElement("pokedex");
        if (!root)
        {
            brls::Logger::error("Invalid Pokemon data file format: {}", filePath);
            continue;
        }

        // Iterate through all Pokemon elements
        for (tinyxml2::XMLElement* pokemonElement = root->FirstChildElement("pokemon");
             pokemonElement;
             pokemonElement = pokemonElement->NextSiblingElement("pokemon"))
        {
            const char* pokemonId = pokemonElement->Attribute("id");
            if (pokemonId && id == pokemonId)
            {
                return parsePokemonNode(pokemonElement, region);
            }
        }
    }

    // If not found, return a default Pokemon
    brls::Logger::error("Pokemon with ID {} not found", id);
    return Pokemon(id, "Unknown");
}

Pokemon PokemonDataLoader::parsePokemonNode(tinyxml2::XMLElement* pokemonElement, const std::string& region)
{
    const char* id = pokemonElement->Attribute("id");
    const char* name = pokemonElement->Attribute("name");
    const char* regionalDexNumber = pokemonElement->Attribute("regionalDexNumber");
    const char* type = pokemonElement->Attribute("type");
    const char* shinyLockedStr = pokemonElement->Attribute("shinyLocked");
    bool shinyLocked = (shinyLockedStr && strcmp(shinyLockedStr, "true") == 0);

    std::string evolution;
    tinyxml2::XMLElement* evolutionElement = pokemonElement->FirstChildElement("evolution");
    if (evolutionElement && evolutionElement->GetText())
    {
        evolution = evolutionElement->GetText();
    }

    std::string exclusiveVersion;
    tinyxml2::XMLElement* exclusiveVersionElement = pokemonElement->FirstChildElement("exclusiveVersion");
    if (exclusiveVersionElement && exclusiveVersionElement->GetText())
    {
        exclusiveVersion = exclusiveVersionElement->GetText();
    }

    std::string locations;
    tinyxml2::XMLElement* locationsElement = pokemonElement->FirstChildElement("locations");
    if (locationsElement && locationsElement->GetText())
    {
        locations = locationsElement->GetText();
    }

    // Apply i18n overrides if available
    std::string regionalId = regionalDexNumber ? regionalDexNumber : "";
    auto makeKey = [&](const std::string& field) {
        return "data/" + region + "/" + regionalId + "/" + field;
    };

    // name and attributes may be overridden
    std::string nameStr = name ? name : "";
    std::string typeStr = type ? type : "";
    std::string evolutionStr = evolution;
    std::string exclusiveVersionStr = exclusiveVersion;
    std::string locationsStr = locations;

    if (!regionalId.empty())
    {
        std::string o;
        o = brls::getStr(makeKey("name"));
        if (o != makeKey("name")) nameStr = o;
        o = brls::getStr(makeKey("type"));
        if (o != makeKey("type")) typeStr = o;
        o = brls::getStr(makeKey("evolution"));
        if (o != makeKey("evolution")) evolutionStr = o;
        o = brls::getStr(makeKey("exclusiveVersion"));
        if (o != makeKey("exclusiveVersion")) exclusiveVersionStr = o;
        o = brls::getStr(makeKey("locations"));
        if (o != makeKey("locations")) locationsStr = o;
    }

    return Pokemon(
        id ? id : "",
        nameStr,
        //regionalDexNumber ? regionalDexNumber : "",
        regionalId,
        typeStr,
        evolutionStr,
        exclusiveVersionStr,
        locationsStr,
        shinyLocked
    );
}
