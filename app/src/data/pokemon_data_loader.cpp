/*
    Copyright 2023

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "data/pokemon_data_loader.hpp"
#include <borealis/core/logger.hpp>
#include <tinyxml2.h>

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
        Pokemon pokemon = parsePokemonNode(pokemonElement);
        pokemons.push_back(pokemon);
    }
    
    return pokemons;
}

Pokemon PokemonDataLoader::loadPokemonById(const std::string& id)
{
    // Try to find the Pokemon in each region
    std::vector<std::string> regions = {"kanto", "johto", "hoenn"};
    
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
                return parsePokemonNode(pokemonElement);
            }
        }
    }
    
    // If not found, return a default Pokemon
    brls::Logger::error("Pokemon with ID {} not found", id);
    return Pokemon(id, "Unknown");
}

Pokemon PokemonDataLoader::parsePokemonNode(tinyxml2::XMLElement* pokemonElement)
{
    const char* id = pokemonElement->Attribute("id");
    const char* name = pokemonElement->Attribute("name");
    const char* regionalDexNumber = pokemonElement->Attribute("regionalDexNumber");
    const char* type = pokemonElement->Attribute("type");
    
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
    
    return Pokemon(
        id ? id : "",
        name ? name : "",
        regionalDexNumber ? regionalDexNumber : "",
        type ? type : "",
        evolution,
        exclusiveVersion,
        locations
    );
}