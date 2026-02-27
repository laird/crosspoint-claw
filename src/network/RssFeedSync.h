#pragma once
#include <string>

namespace RssFeedSync {

enum class State {
  IDLE,        // Not active — pill hidden
  FETCHING,    // Requesting feed URL
  PARSING,     // Got response, parsing XML
  DOWNLOADING, // Downloading individual files (n of total)
  ERROR,       // Fetch or parse failed
  DONE,        // All files processed — briefly shown then → IDLE
};

/// Start background sync task. No-op if feed URL empty, WiFi not connected, or already running.
void startSync();

/// Current sync state.
State getState();

/// Short status label for the FEED pill (max ~6 chars): "FEED", "SYNC", "1/81", "ERR", "DONE"
const char* getStatusLabel();

/// Returns true while sync is in progress (any state except IDLE).
bool isFeedActive();

/// Returns true while actively downloading files.
bool isSyncing();

}  // namespace RssFeedSync
