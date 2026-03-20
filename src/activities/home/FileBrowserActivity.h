#pragma once

#include <functional>
#include <string>
#include <vector>

#include "../Activity.h"
#include "RecentBooksStore.h"
#include "util/ButtonNavigator.h"

class FileBrowserActivity final : public Activity {
 public:
  enum SortMode { SORT_ALPHABETICAL, SORT_RECENT_RECEIVED, SORT_RECENT_READ };

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

  // Sorting
  void applySortMode();
  static const char* getSortModeLabel(SortMode mode);

 public:
  explicit FileBrowserActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string initialPath = "/")
      : Activity("FileBrowser", renderer, mappedInput), basepath(initialPath.empty() ? "/" : std::move(initialPath)) {}
  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
};
