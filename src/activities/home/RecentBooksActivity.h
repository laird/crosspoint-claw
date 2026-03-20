#pragma once
#include <I18n.h>

#include <functional>
#include <string>
#include <vector>

#include "../Activity.h"
#include "RecentBooksStore.h"
#include "util/ButtonNavigator.h"

class RecentBooksActivity final : public Activity {
 public:
  /// Sort modes for recent books display.
  /// READ: Most recently opened books first
  /// LOAD: Most recently synced to reader first
  /// ALPHABETICAL: A-Z alphabetical order
  enum SortMode { SORT_READ, SORT_LOAD, SORT_ALPHABETICAL };

 private:
  ButtonNavigator buttonNavigator;

  size_t selectorIndex = 0;
  SortMode currentSortMode = SORT_READ;  // Default: most recent reads first

  // Recent tab state
  std::vector<RecentBook> recentBooks;

  // Data loading and sorting
  void loadRecentBooks();
  void applySortMode();
  static const char* getSortModeLabel(SortMode mode);

 public:
  explicit RecentBooksActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("RecentBooks", renderer, mappedInput) {}
  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
};
