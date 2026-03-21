#pragma once

#include <functional>
#include <string>
#include <vector>

#include "../Activity.h"
#include "RecentBooksStore.h"
#include "util/ButtonNavigator.h"

class FileBrowserActivity final : public Activity {
 public:
  /// Sort modes for the file browser list view.
  ///
  /// SORT_ALPHABETICAL: Natural sort A-Z (default, case-insensitive with numeric awareness)
  /// SORT_RECENT_RECEIVED: Newest files first by modification time (when synced to reader)
  /// SORT_RECENT_READ: Newest reads first by last-opened timestamp (from RecentBooksStore)
  ///
  /// Use Left/Right buttons to cycle through modes. Directories always sort first.
  enum SortMode { SORT_ALPHABETICAL, SORT_RECENT_RECEIVED, SORT_RECENT_READ };
  static constexpr int SORT_MODE_COUNT = 3;

 private:
  // Deletion
  void clearFileMetadata(const std::string& fullPath);

  ButtonNavigator buttonNavigator;

  size_t selectorIndex = 0;
  SortMode currentSortMode = SORT_ALPHABETICAL;

  // Files state
  std::string basepath = "/";
  std::vector<std::string> files;

  // Data loading
  void loadFiles();
  size_t findEntry(const std::string& name) const;

  /// Applies the current sort mode to the files list.
  ///
  /// Sorts the files list according to currentSortMode:
  /// - SORT_ALPHABETICAL: Natural sort (default, existing behavior)
  /// - SORT_RECENT_RECEIVED: Sort by file modification time (newest first)
  /// - SORT_RECENT_READ: Sort by last-opened timestamp from RecentBooksStore (newest first)
  ///
  /// Directories always sort first in all modes to maintain consistent navigation.
  /// Unread books (not in RecentBooksStore) sort to bottom in RECENT_READ mode.
  /// Resets selectorIndex to 0 after sorting completes.
  void applySortMode();

  /// Gets the human-readable label for a sort mode.
  ///
  /// Maps SortMode enum values to translatable UI string identifiers
  /// for display in button hints and UI labels.
  ///
  /// @param mode The sort mode to get a label for
  /// @return Pointer to translated string (via tr() macro)
  static const char* getSortModeLabel(SortMode mode);

 public:
  explicit FileBrowserActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string initialPath = "/")
      : Activity("FileBrowser", renderer, mappedInput), basepath(initialPath.empty() ? "/" : std::move(initialPath)) {}
  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
};
