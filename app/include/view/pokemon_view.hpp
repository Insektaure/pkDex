/*
    Copyright 2021 XITRIX

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

    Pokemon(std::string id, std::string name, std::string regionalDexNumber = "", std::string type = "", std::string evolution = "", std::string exclusiveVersion="", std::string locations = "")
        : id(id)
        , name(name)
        , regionalDexNumber(regionalDexNumber)
        , type(type)
        , evolution(evolution)
        , exclusiveVersion(exclusiveVersion)
        , locations(locations)
    {
    }
};

class PokemonView : public brls::Box
{
  public:
    PokemonView(Pokemon pokemon);
    PokemonView()
        : PokemonView(Pokemon("001", "Bulbasaur"))
    {
    }

    static brls::View* create();

  private:
    Pokemon pokemon;
    void loadHighResImage(brls::Image* image, const std::string& path, const std::string& id);
    BRLS_BIND(brls::Image, standard_image, "standard_image");
    BRLS_BIND(brls::Image, shiny_image, "shiny_image");
    BRLS_BIND(brls::Label, national_dex, "national_dex");
    BRLS_BIND(brls::Label, regional_dex, "regional_dex");
    BRLS_BIND(brls::Label, type, "type");
    BRLS_BIND(brls::Label, evolution, "evolution");
    BRLS_BIND(brls::Label, exclusive_version, "exclusive_version");
    BRLS_BIND(brls::Label, locations, "locations");
};
