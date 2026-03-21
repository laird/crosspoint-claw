#pragma once
#include <ctime>
#include <string>
#include <vector>

/// Metadata for a recently-opened book stored in RecentBooksStore.
///
/// Tracks essential book metadata (path, title, author, cover) plus the timestamp
/// of when the book was last opened. Used for sorting file browser by read recency.
struct RecentBook {
  std::string path;            ///< Full path to the book file (e.g., "/Books/chip/story.epub")
  std::string title;           ///< Book title (extracted from EPUB metadata)
  std::string author;          ///< Author name (extracted from EPUB metadata)
  std::string coverBmpPath;    ///< Path to cached cover image (BMP format)
  time_t lastReadTime = 0;     ///< Unix timestamp of last open (0 if never read)

  bool operator==(const RecentBook& other) const { return path == other.path; }
};

class RecentBooksStore;
namespace JsonSettingsIO {
bool loadRecentBooks(RecentBooksStore& store, const char* json);
}  // namespace JsonSettingsIO

class RecentBooksStore {
  // Static instance
  static RecentBooksStore instance;

  std::vector<RecentBook> recentBooks;

  friend bool JsonSettingsIO::loadRecentBooks(RecentBooksStore&, const char*);

 public:
  ~RecentBooksStore() = default;

  // Get singleton instance
  static RecentBooksStore& getInstance() { return instance; }

  // Add a book to the recent list (moves to front if already exists)
  void addBook(const std::string& path, const std::string& title, const std::string& author,
               const std::string& coverBmpPath);

  void updateBook(const std::string& path, const std::string& title, const std::string& author,
                  const std::string& coverBmpPath);

  // Get the list of recent books (most recent first)
  const std::vector<RecentBook>& getBooks() const { return recentBooks; }

  // Get the count of recent books
  int getCount() const { return static_cast<int>(recentBooks.size()); }

  bool saveToFile() const;

  bool loadFromFile();
  RecentBook getDataFromBook(std::string path) const;

  /// Retrieves the last-read timestamp for the book at the given path.
  ///
  /// Searches the recent books list for a book matching the provided path
  /// and returns its lastReadTime (Unix timestamp). If the book is not found
  /// in the recent list, returns 0 (never read).
  ///
  /// @param path Full path to the book file (e.g., "/Books/chip/story.epub")
  /// @return Unix timestamp of last read (time_t), or 0 if not found
  time_t getLastReadTime(const std::string& path) const;

 private:
  bool loadFromBinaryFile();
};

// Helper macro to access recent books store
#define RECENT_BOOKS RecentBooksStore::getInstance()
