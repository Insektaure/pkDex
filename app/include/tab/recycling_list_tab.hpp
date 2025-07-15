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
    DataSource();
    int numberOfSections(brls::RecyclerFrame* recycler) override;
    int numberOfRows(brls::RecyclerFrame* recycler, int section) override;
    brls::RecyclerCell* cellForRow(brls::RecyclerFrame* recycler, brls::IndexPath index) override;
    void didSelectRowAt(brls::RecyclerFrame* recycler, brls::IndexPath indexPath) override;
    std::string titleForHeader(brls::RecyclerFrame* recycler, int section) override;

    // Get the currently selected index path
    brls::IndexPath getCurrentSelection() const { return currentSelection; }

    // Set the currently selected index path
    void setCurrentSelection(const brls::IndexPath& indexPath) { currentSelection = indexPath; }

  private:
    brls::IndexPath currentSelection;
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
    static brls::View* createRegionTab(const std::string& region);
    static brls::View* createKanto();
    //static brls::View* createJohto();
    //static brls::View* createHoenn();
    static brls::View* createSinnoh();
    static brls::View* createSinnohArceus();
    static brls::View* createGalar();
    static brls::View* createIsleArmor();
    static brls::View* createCrownTundra();
    static brls::View* createPaldea();
    static brls::View* createKitakami();
    static brls::View* createBlueberry();

    // Load Pokemon data from a file
    void loadPokemonData(const std::string& region);

    // Get the current region
    std::string getCurrentRegion() const { return currentRegion; }

  private:
    // Current region
    std::string currentRegion;

    // Toggle capture status of the currently selected Pokemon
    bool toggleCaptureStatus(brls::View* view);

    // Jump to previous/next page (30 rows) using L/R buttons
    bool jumpToPreviousPage(brls::View* view);
    bool jumpToNextPage(brls::View* view);

    // Ensure the cell at the given index path is fully visible
    void ensureCellVisible(const brls::IndexPath& indexPath);

    BRLS_BIND(brls::RecyclerFrame, recycler, "recycler");
    DataSource* dataSource;
};
