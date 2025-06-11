/*
    Copyright 2020-2021 natinusala

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
#include <borealis/views/button.hpp>
#include <map>

class RecyclerHeader
    : public brls::RecyclerHeader
{
};

class RecyclerCell
    : public brls::RecyclerCell
{
  public:
    RecyclerCell();

    BRLS_BIND(brls::Rectangle, accent, "brls/sidebar/item_accent");
    BRLS_BIND(brls::Label, label, "title");
    BRLS_BIND(brls::Image, image, "image");

    static RecyclerCell* create();
};

class DataSource
    : public brls::RecyclerDataSource
{
  public:
    int numberOfSections(brls::RecyclerFrame* recycler) override;
    int numberOfRows(brls::RecyclerFrame* recycler, int section) override;
    brls::RecyclerCell* cellForRow(brls::RecyclerFrame* recycler, brls::IndexPath index) override;
    void didSelectRowAt(brls::RecyclerFrame* recycler, brls::IndexPath indexPath) override;
    std::string titleForHeader(brls::RecyclerFrame* recycler, int section) override;
};

class RecyclingListTab : public brls::Box
{
  public:
    RecyclingListTab();
    RecyclingListTab(const std::string& region);

    // Factory method for XML engine (default region)
    static brls::View* create();

    // Factory method with specific region
    static brls::View* create(const std::string& region);

    // Factory methods for specific regions
    static brls::View* createKanto();
    static brls::View* createJohto();
    static brls::View* createHoenn();
    static brls::View* createSinnoh();
    static brls::View* createGalar();
    static brls::View* createPaldea();

    // Load Pokemon data from a file
    void loadPokemonData(const std::string& region);

  private:
    BRLS_BIND(brls::RecyclerFrame, recycler, "recycler");
};
