#pragma once

#include <string>
#include <vector>

#include "../Activity.h"

class CoverBrowserActivity final : public Activity {
 private:
  static constexpr int GRID_COLS = 3;
  static constexpr int GRID_ROWS = 3;
  static constexpr int GRID_SIZE = GRID_COLS * GRID_ROWS;
  static constexpr int COVER_THUMB_H = 185;
  static constexpr int CELL_PADDING = 8;
  static constexpr int SELECTION_BORDER = 2;

  std::string basepath;
  std::vector<std::string> files;  // EPUB/XTC filenames only

  int selectorIndex = 0;
  int pageOffset = 0;  // index of first item on current page

  // Framebuffer caching (follows Lyra3CoversTheme pattern)
  bool coverRendered = false;
  bool coverBufferStored = false;
  bool bufferRestored = false;
  bool needFullRefresh = true;

  void loadFiles();
  std::string getServerThumbPath(const std::string& epubFullPath) const;
  std::string findThumbPath(const std::string& epubFullPath) const;
  void drawGrid();
  void drawCoverCell(int cellIndex, int gridX, int gridY, int cellW, int cellH);
  void drawSelectionHighlight(int cellIndex, int gridX, int gridY, int cellW, int cellH);

  int totalPages() const;
  int currentPage() const;

 public:
  explicit CoverBrowserActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string path)
      : Activity("CoverBrowser", renderer, mappedInput), basepath(std::move(path)) {}
  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
};
