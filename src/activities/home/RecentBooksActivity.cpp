#include "RecentBooksActivity.h"

#include <FsHelpers.h>

#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>

#include <algorithm>

#include "MappedInputManager.h"
#include "RecentBooksStore.h"
#include "components/UITheme.h"
#include "fontIds.h"




namespace {
constexpr unsigned long GO_HOME_MS = 1000;
}  // namespace

void RecentBooksActivity::loadRecentBooks() {
  recentBooks.clear();
  const auto& books = RECENT_BOOKS.getBooks();
  recentBooks.reserve(books.size());

  for (const auto& book : books) {
    // Skip if file no longer exists
    if (!Storage.exists(book.path.c_str())) continue;
    // Skip plain-text/log/system files — they don't belong in the reading history
    const auto slash = book.path.rfind('/');
    const std::string name = (slash == std::string::npos) ? book.path : book.path.substr(slash + 1);
    if (FsHelpers::hasTxtExtension(name) || FsHelpers::hasMarkdownExtension(name)) continue;
    recentBooks.push_back(book);
  }
  applySortMode();
}

const char* RecentBooksActivity::getSortModeLabel(SortMode mode) {
  switch (mode) {
    case SORT_UNREAD:
      return "Unread";
    case SORT_READ:
      return tr(STR_SORT_RECENT_READ);
    case SORT_LOAD:
      return tr(STR_SORT_DATE);
    case SORT_ALPHABETICAL:
      return tr(STR_SORT_NAME);
    default:
      return "Unread";
  }
}

void RecentBooksActivity::applySortMode() {
  switch (currentSortMode) {
    case SORT_UNREAD:
      // Sort unread books first (lastReadTime == 0), then by load order
      std::stable_sort(recentBooks.begin(), recentBooks.end(),
                       [](const RecentBook& a, const RecentBook& b) {
                         // Unread (0) sorts before read (>0)
                         bool a_unread = (a.lastReadTime == 0);
                         bool b_unread = (b.lastReadTime == 0);
                         return a_unread > b_unread;  // true > false, so unread first
                       });
      break;

    case SORT_READ:
      // Sort by lastReadTime (newest first)
      std::sort(recentBooks.begin(), recentBooks.end(),
                [](const RecentBook& a, const RecentBook& b) { return a.lastReadTime > b.lastReadTime; });
      break;

    case SORT_LOAD:
      // Sort by position in RecentBooksStore (insertion order = load order, newest first)
      // Already in load order from loadRecentBooks(), but keep explicit for clarity
      break;

    case SORT_ALPHABETICAL:
      // Sort alphabetically by title
      std::sort(recentBooks.begin(), recentBooks.end(),
                [](const RecentBook& a, const RecentBook& b) { return a.title < b.title; });
      break;
  }
  selectorIndex = 0;  // Reset to top after sort
}

void RecentBooksActivity::onEnter() {
  Activity::onEnter();

  // Load data
  loadRecentBooks();

  selectorIndex = 0;
  requestUpdate();
}

void RecentBooksActivity::onExit() {
  Activity::onExit();
  recentBooks.clear();
}

void RecentBooksActivity::loop() {
  const int pageItems = UITheme::getInstance().getNumberOfItemsPerPage(renderer, true, false, true, true);

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (!recentBooks.empty() && selectorIndex < static_cast<int>(recentBooks.size())) {
      LOG_DBG("RBA", "Selected recent book: %s", recentBooks[selectorIndex].path.c_str());
      onSelectBook(recentBooks[selectorIndex].path);
      return;
    }
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    onGoHome();
  }

  // PageBack cycles to previous sort mode (UNREAD → LOAD → READ → ALPHABETICAL → UNREAD)
  bool pageButtonPressed = false;
  if (mappedInput.wasReleased(MappedInputManager::Button::PageBack)) {
    currentSortMode = static_cast<SortMode>((static_cast<int>(currentSortMode) + 3) % 4);
    applySortMode();
    requestUpdate();
    pageButtonPressed = true;
  }

  // PageForward cycles to next sort mode (UNREAD → READ → LOAD → ALPHABETICAL → UNREAD)
  if (mappedInput.wasReleased(MappedInputManager::Button::PageForward)) {
    currentSortMode = static_cast<SortMode>((static_cast<int>(currentSortMode) + 1) % 4);
    applySortMode();
    requestUpdate();
    pageButtonPressed = true;
  }

  int listSize = static_cast<int>(recentBooks.size());

  // Only handle list navigation if page buttons weren't pressed
  if (!pageButtonPressed) {
    buttonNavigator.onNextRelease([this, listSize] {
      selectorIndex = ButtonNavigator::nextIndex(static_cast<int>(selectorIndex), listSize);
      requestUpdate();
    });

    buttonNavigator.onPreviousRelease([this, listSize] {
      selectorIndex = ButtonNavigator::previousIndex(static_cast<int>(selectorIndex), listSize);
      requestUpdate();
    });

    buttonNavigator.onNextContinuous([this, listSize, pageItems] {
      selectorIndex = ButtonNavigator::nextPageIndex(static_cast<int>(selectorIndex), listSize, pageItems);
      requestUpdate();
    });

    buttonNavigator.onPreviousContinuous([this, listSize, pageItems] {
      selectorIndex = ButtonNavigator::previousPageIndex(static_cast<int>(selectorIndex), listSize, pageItems);
      requestUpdate();
    });
  }
}

void RecentBooksActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  const auto& metrics = UITheme::getInstance().getMetrics();

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_MENU_RECENT_BOOKS));

  const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int contentHeight = pageHeight - contentTop - metrics.buttonHintsHeight - metrics.verticalSpacing;

  // Recent tab
  if (recentBooks.empty()) {
    renderer.drawText(PULSR_10_FONT_ID, metrics.contentSidePadding, contentTop + 20, tr(STR_NO_RECENT_BOOKS));
  } else {
    GUI.drawList(
        renderer, Rect{0, contentTop, pageWidth, contentHeight}, recentBooks.size(), selectorIndex,
        [this](int index) { return recentBooks[index].title; }, [this](int index) { return recentBooks[index].author; },
        [this](int index) { return UITheme::getFileIcon(recentBooks[index].path); });
  }

  // Help text — show sort mode labels on side buttons
  const SortMode prevMode = static_cast<SortMode>((static_cast<int>(currentSortMode) + 2) % 3);
  const SortMode nextMode = static_cast<SortMode>((static_cast<int>(currentSortMode) + 1) % 3);
  const auto labels = mappedInput.mapLabels(tr(STR_HOME), tr(STR_OPEN), getSortModeLabel(prevMode),
                                            getSortModeLabel(nextMode));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
