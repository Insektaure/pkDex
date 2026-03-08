#pragma once

#include <borealis.hpp>
#include <borealis/views/button.hpp>
#include <map>
#include <set>

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
  BRLS_BIND(brls::Rectangle, selectIndicator, "selectIndicator");
  BRLS_BIND(brls::Label, label, "title");
  BRLS_BIND(brls::Image, image, "image");
  BRLS_BIND(brls::Image, rightIcon, "rightIcon");
  BRLS_BIND(brls::Image, rightIcon2, "rightIcon2");
  BRLS_BIND(brls::Image, rightIcon3, "rightIcon3");
  BRLS_BIND(brls::Image, rightIcon4, "rightIcon4");

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
    static brls::View* createKanto();
    static brls::View* createKantoFrlg();
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
    static brls::View* createKalosLza();
    static brls::View* createHyperspaceLumiose();

    // Load Pokemon data from a file
    void loadPokemonData(const std::string& region);

    // Get the current region
    std::string getCurrentRegion() const { return currentRegion; }

    // Multi-select mode
    bool isMultiSelectMode() const { return multiSelectMode; }
    bool isIndexSelected(int index) const { return selectedIndices.count(index) > 0; }
    void toggleIndexSelection(int index);
    void exitMultiSelectMode();

    // Refresh the recycler and restore focus to the given index path
    void refreshRecycler(const brls::IndexPath& sel);

  private:
    // Current region
    std::string currentRegion;

    // Multi-select state
    bool multiSelectMode = false;
    std::set<int> selectedIndices;

    // Toggle multi-select mode on/off
    bool toggleMultiSelectMode(brls::View* view);

    // Toggle capture status of the currently selected Pokemon (or multi-selected)
    bool toggleCaptureStatus(brls::View* view);

    // Open bulk actions dialog for the current region
    bool openBulkActionsDialog(brls::View* view);

    // Jump to previous/next page (30 rows) using L/R buttons
    bool jumpToPreviousPage(brls::View* view);
    bool jumpToNextPage(brls::View* view);

    // Get the index path of the currently focused cell
    brls::IndexPath getFocusedIndexPath();

    // Ensure the cell at the given index path is fully visible
    void ensureCellVisible(const brls::IndexPath& indexPath);

    BRLS_BIND(brls::RecyclerFrame, recycler, "recycler");
    DataSource* dataSource;
};
