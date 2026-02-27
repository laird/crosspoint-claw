#pragma once

namespace RssFeedSync {

/// Start background sync task. No-op if feed URL empty, WiFi not connected, or already running.
void startSync();

/// Returns true if a sync task is currently running.
bool isSyncing();

/// Returns true from the moment WiFi connects (startSync called) until sync fully completes.
/// Use this to show the FEED indicator — lit while pending/syncing, dark when done.
bool isFeedActive();

}  // namespace RssFeedSync
