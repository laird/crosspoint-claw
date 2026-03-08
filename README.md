# CrossPoint OpenClaw — AI-Curated E-Reader

This is a fork of [CrossPoint Reader](CROSSPOINT-README.md) that extends it with **AI-driven content delivery**: an RSS feed pipeline that lets a personal AI agent (OpenClaw/Chip) automatically generate, package, and push content — news digests, EPUBs, markdown articles, sleep screen art, and firmware updates — directly to the device over WiFi.

![Home screen](docs/images/screenshots/home.png)
*The home screen — recent books surface automatically as new content arrives.*

---

## What this fork adds

### RSS Feed Sync

The device polls an RSS feed server on every WiFi connect and automatically downloads new content to the SD card. Supported content types:

| Type | Extension | Notes |
|------|-----------|-------|
| EPUB books | `.epub` | Full e-reader support |
| Articles / news | `.md`, `.txt` | Rendered natively |
| Sleep screen art | `.bmp` | Displayed on sleep screen |
| Firmware updates | `.bin` | Flashed on next boot |

The feed server runs anywhere on your local network (or internet). A simple Python HTTP server with an RSS XML feed is all that's needed. See `docs/rss-content-feeds.md` for the full feed format spec.

**Smart deduplication:** Items are deduplicated by timestamp + file-exists + size check — not just GUID. This means the device never re-downloads files that are already on the SD card, even after a firmware reflash wipes GUID history.

**Skip sync:** Hold Up or Down at WiFi connect to suppress the sync for that session — useful when you want to connect without triggering a 70-file download.

---

### OTA Safety — Never Overwrite the Running Firmware

This fork is strict about OTA partition safety:

- **Always writes to the inactive partition** — the currently-running firmware is never touched
- **Firmware.bin deleted from SD after any flash attempt** (success or failure) — no flash loops
- **SHA-based GUID check** — the device won't re-flash firmware it's already running
- **ESP-IDF rollback** — if new firmware crashes before calling `markValid()`, the bootloader automatically rolls back to the previous partition

The result: there is always a known-good firmware in the inactive partition as a fallback.

---

### Pulsr Theme

A custom theme (upstream only includes Classic and Lyra) with:

- Clean top bar showing WiFi/Feed status pills and battery
- Feed sync progress states: FEED → SYNC → n/total → DONE / ERR
- **Version string right-aligned in the top bar** — `1.1.1-claw (44a2001)` visible on every screen
- File transfer screen live-updates as content arrives from the feed

---

### Hidden System Logs

System log files are stored in `/.crosspoint/` on the SD card — invisible in the library browser (which filters dot-prefixed names) but still accessible via the web API:

| File | Purpose | API endpoint |
|------|---------|--------------|
| `/.crosspoint/boot.log` | Boot events and OTA flash results | `GET /api/boot-log` |
| `/.crosspoint/ota_error.log` | OTA error details | `GET /api/boot-log` |
| `/.crosspoint/feed_sync_time.txt` | Timestamp of last successful sync | internal |

---

### Danger Zone — Remote OTA & Auto-Connect

The **Danger Zone** (enabled in Settings → SYST) unlocks remote management:

- **Auto-connect on boot**: device rejoins known WiFi automatically, no manual interaction needed
- **Background web server**: file upload, EPUB management, and feed sync all accessible via browser while reading
- **Remote OTA flash**: push new firmware to the SD card and it flashes on next sleep/wake
- **QR code on boot**: device lands on the file transfer screen after auto-connect, ready to receive content

| | |
|---|---|
| ![File transfer screen with QR code](docs/images/screenshots/file_transfer.png) | ![System settings — Danger Zone toggle](docs/images/screenshots/settings_syst.png) |
| *On boot with Danger Zone enabled, the device auto-connects and shows the QR code screen* | *Enable Danger Zone in Settings → SYST, then set a password* |

---

### Web Interface

Once connected, the full web interface is accessible from any browser on the network — upload EPUBs, browse files, trigger syncs, and push firmware.

| | |
|---|---|
| ![Web interface — file manager](docs/images/wifi/webserver_files.png) | ![Web interface — upload](docs/images/wifi/webserver_upload.png) |
| *Browser-based file manager* | *Drag-and-drop EPUB upload* |

---

### Reading

![Reader](docs/images/screenshots/reader.png)
*Reading an EPUB — delivered automatically via the RSS feed.*

---

### Network Setup

| | |
|---|---|
| ![Network mode](docs/images/screenshots/network_mode.png) | ![WiFi scan](docs/images/screenshots/wifi_scan.png) |
| *Choose between joining a network or creating a hotspot* | *Select your WiFi network* |

---

### OpenClaw / Chip Integration

**OpenClaw** is a personal AI agent (Claude Sonnet, running as `claude-code`) that acts as the content brain for the device. It:

- Generates daily news digests as markdown files and pushes them via RSS
- Summarises and packages articles into readable `.md` or `.epub` files
- Creates sleep screen artwork (`.bmp`) tailored to the reader's preferences
- Monitors the feed server and triggers firmware updates autonomously
- Runs the full build → deploy → flash → verify loop without human intervention

The reader becomes a **passive delivery target**: Chip generates content on a schedule, the device syncs on boot, and new reading material appears automatically.

---

## How it works end to end

```
OpenClaw/Chip (AI agent)
    │
    │  generates content, builds firmware
    ▼
Feed Server (Python HTTP, local network)
    │  serves RSS feed + files at http://192.168.x.x:8090/feed.xml
    ▼
CrossPoint Device (ESP32-C3, Xteink X4)
    │  syncs on WiFi connect, downloads new items to SD card
    │  flashes firmware on next boot if firmware.bin present (version-checked)
    ▼
Reader (you)
    opens new articles, books, and art — automatically delivered
```

---

## Getting Started

There are two paths depending on what you want to do:

| Goal | Path |
|------|------|
| Get a reader receiving AI-curated content | **Loop 1: Reader Setup** (below) |
| Build or modify the firmware itself | **Loop 2: Firmware Dev** (see [Development](#development)) |

Most people only ever need Loop 1.

---

## Loop 1: Reader Setup + OpenClaw Content Delivery

### Step 1 — Flash the firmware (one-time, human step)

You need a USB-C cable and Chrome or Edge browser.

1. Download the latest `.bin` from the [Releases page](https://github.com/laird/crosspoint-claw/releases)
2. Open the web flasher: **https://xteink.dve.al/**
3. Click **Connect** → select the reader's USB serial port
4. Upload the `.bin` and click **Flash**

That's it. The flasher handles everything. After this, all future updates happen wirelessly — no USB or browser needed again.

### Step 2 — Enable Danger Zone on the device

On the device:
1. Settings → SYST tab → toggle **Danger Zone** ON
2. Tap **Danger Zone Password** → set a password
3. Reboot

After reboot, the device auto-connects to WiFi. The web API is now live — OpenClaw can push content and firmware updates wirelessly from here on.

### Step 3 — Point OpenClaw at the reader

Give your OpenClaw instance this prompt:

> You are managing a CrossPoint e-ink reader. Read the README at `/home/laird/src/crosspoint-claw/README.md` (or `https://github.com/laird/crosspoint-claw`) — specifically the **AI Agent Operations** section — for the full reference on pushing books, news, and firmware. The reader is at `192.168.0.234` (or your reader's IP). Danger Zone password is `1814` (or whatever you set).

OpenClaw will read the README and know exactly what to do: push EPUBs, sync articles, deliver news digests, and optionally push firmware updates — all over WiFi, on a schedule, automatically.

### Step 4 — Start a feed server (optional, for scheduled content)

```bash
# Minimal feed server (Python)
cd /path/to/your/feed/content
python3 -m http.server 8090
```

See `docs/rss-content-feeds.md` for the full feed format spec and `scripts/crosspoint-feed-server.py` for a production-ready feed server with directory auto-scanning.

Configure the feed URL on the device: Settings → NETW → Feed URL → `http://192.168.x.x:8090/feed.xml`

---

## Loop 2: Firmware Development

See [Development](#development) below. This requires PlatformIO, the source repo, and comfort with embedded C++. OpenClaw can assist with the build → flash → verify cycle once you're set up, but the initial dev environment is a human setup step.

---

## Development

### Prerequisites

- **PlatformIO** (VS Code extension or CLI: `pip install platformio`)
- **Python 3** (for i18n code generation and flash scripts)
- **USB cable** (for initial flash only — after that, everything is OTA)

### Clone and Build

```bash
git clone https://github.com/laird/crosspoint-claw.git
cd crosspoint-claw
git submodule update --init --recursive

# Build firmware
pio run
```

### Safe USB Flash

> ⚠️ **Always commit before flashing.** The feed server identifies firmware by SHA. If you flash uncommitted code, the feed re-downloads the old firmware on next sync and overwrites your work. `flash-crosspoint.sh` enforces this with a pre-flight check.

Use `scripts/crosspoint-flash.py` — an OTA-aware flash script that always writes to the **inactive** partition and preserves the running firmware as a rollback fallback:

```bash
# Prerequisites: esptool in a venv
pip install esptool

# Check partition state without flashing
python3 scripts/crosspoint-flash.py --status --port /dev/cu.usbmodem101

# Build, commit-check, update feed, and flash safely (wrapper script)
scripts/flash-crosspoint.sh

# Or flash a pre-built binary directly
python3 scripts/crosspoint-flash.py .pio/build/default/firmware.bin --port /dev/cu.usbmodem101
```

**What the script does:**
1. Reads `otadata` to determine which partition (app0/app1) is currently active
2. Flashes new firmware to the **inactive** partition only
3. Updates `otadata` to boot from the new partition on next reset
4. Resets the device

**Rollback safety:** If the new firmware crashes before calling `esp_ota_mark_app_valid_cancel_rollback()`, the ESP-IDF bootloader automatically reverts to the previous partition. The old firmware is never overwritten.

### OTA via Feed (Preferred After First Flash)

Once the device is running this firmware, USB is rarely needed. Push a new build via the feed server instead:

```bash
# Build
pio run

# Copy firmware to feed server content dir
cp .pio/build/default/firmware.bin ~/crosspoint-feed/firmware/firmware.bin

# Restart feed server so it picks up the new file
systemctl --user restart crosspoint-feed

# Trigger a sync on the device
curl -X POST http://192.168.x.x/api/feed/sync
```

The device downloads `firmware.bin`, writes it to SD card, and flashes on next boot. After a successful flash, `firmware.bin` is automatically deleted from the SD card to prevent re-flashing.

### Iterative Development Loop

```
Write code → commit → pio run → copy firmware.bin to feed server
    → device syncs on next WiFi connect (or trigger manually)
    → check /.crosspoint/boot.log for flash result
    → monitor Serial for runtime logs
    → repeat
```

No USB flashing after the first setup. No physical access to the device needed.

### Serial Debugging

```bash
# Linux/Mac
python3 scripts/debugging_monitor.py

# macOS (adjust port)
python3 scripts/debugging_monitor.py /dev/cu.usbmodem101
```

RSS sync logs all activity to Serial in real time. System log files on the SD card (`/.crosspoint/boot.log`, `/.crosspoint/ota_error.log`) are also accessible via `GET /api/boot-log`.

### Screen Captures

```bash
# Capture current screen as PNG
curl http://192.168.x.x/api/screenshot > screen.png
```

Useful for verifying UI changes, attaching to bug reports, and documenting UI states.

### GitHub Issues

Always use `--repo laird/crosspoint-claw` when filing issues with `gh`:

```bash
gh issue create --repo laird/crosspoint-claw --title "..." --body "..."
```

Without `--repo`, `gh` defaults to the upstream repo (`crosspoint-reader/crosspoint-reader`).

---

## AI Agent Operations (OpenClaw / Chip)

This section is a complete reference for an AI agent managing these readers. Copy it as a system prompt or context block for any OpenClaw instance.

**Two modes of use:**
- **Content delivery** (most users): The firmware is already installed. OpenClaw pushes books, news, and articles to the reader over WiFi. No build tools or firmware knowledge needed.
- **Firmware development**: OpenClaw builds and flashes new firmware wirelessly. Requires PlatformIO installed and the source repo cloned.

> **First-time firmware install** is a human step done once via browser + USB (see [Setup](#setup) above). After that, OpenClaw handles everything wirelessly.

### Reader Network Info

| Reader | LAN IP | Purpose |
|--------|--------|---------|
| Laird's | 192.168.0.234 | Primary |
| Juliette's | 192.168.0.194 | Secondary |

Danger Zone endpoints require `?password=1814`. All others are open HTTP.

### Check Reader Status

```bash
curl -s http://192.168.0.234/api/status | python3 -m json.tool
# Key fields: version, uptime, freeHeap, dangerZoneEnabled, ip
```

If offline (curl fails), the reader is asleep and needs a physical wake (tap screen or press a button).

### Push Books

**Via crosspoint-feed (preferred — auto-syncs on reader connect):**

```bash
cp my-story.epub ~/clawd/crosspoint-feed/books/chip/     # AI stories
cp article.md    ~/clawd/crosspoint-feed/thought/         # Articles (.md works natively)
cp story.epub    ~/clawd/crosspoint-feed/books/erotic/    # AO3 / erotica
curl -X POST http://192.168.0.234/api/feed/sync           # trigger immediate sync
```

**Via direct HTTP upload (immediate, one file):**

```bash
curl -X POST "http://192.168.0.234/upload?path=/Books/chip/" \
  -F "file=@my-book.epub"
```

**Via push-epubs.py (mirror-aware, skips already-sent files):**

```bash
/home/laird/clawd/venvs/epub-tools/bin/python3 \
  /home/laird/clawd/scripts/push-epubs.py [--host laird|juliette] [--dry-run]
```

**Convert Markdown → EPUB:**

```bash
/home/laird/clawd/venvs/epub-tools/bin/python3 \
  /home/laird/clawd/scripts/story-to-epub.py my-story.md \
  --out-dir ~/Documents/epub/Books/chip/
```

Markdown should have YAML frontmatter: `title`, `author`.

**SD card directories:**

| Path | Content |
|------|---------|
| `/Books/chip/` | AI-written stories |
| `/Books/erotic/` | AO3 downloads |
| `/Books/humble/` | Purchased books |
| `/Thought/` | Articles |
| `/trips/` | Road trip guides |
| `/News/` | News briefings |

### Push News / Briefings

```bash
cat > ~/clawd/crosspoint-feed/news/digest.json << 'EOF'
{"title": "AI Digest — March 8", "date": "2026-03-08", "body": "Content here..."}
EOF
curl -X POST http://192.168.0.234/api/feed/sync
```

### Flash Firmware

**⚠️ Always bump `version` in `platformio.ini` between iterative flashes.** The reader compares the version string in `firmware.bin` to the running version and skips the flash if they match.

```bash
# 1. Edit platformio.ini → bump version (e.g. 1.3.33-claw → 1.3.34-claw)
# 2. Build
cd /home/laird/src/crosspoint-claw && pio run
# 3. Upload to reader SD card
curl -X POST "http://192.168.0.234/upload?path=/" \
  -F "file=@.pio/build/default/firmware.bin;filename=firmware.bin"
# 4. Reboot to flash (~30s offline, shows "Updating firmware..." on screen)
curl -X POST "http://192.168.0.234/api/reboot?password=1814"
# 5. Verify
sleep 35 && curl -s http://192.168.0.234/api/status | \
  python3 -c "import json,sys; print(json.load(sys.stdin)['version'])"
```

Rollback is enabled — if the new firmware crashes before calling `markValid()`, the bootloader silently reverts to the previous version.

### Screenshot Tour (Verify UI Changes)

```bash
curl -X POST "http://192.168.0.234/api/screenshot-tour?password=1814"
# WiFi disconnects during tour (~35s), then auto-reconnects
sleep 40
# Download a screenshot
curl -s "http://192.168.0.234/download?path=/screencap/home.bmp" -o home.bmp
ffmpeg -i home.bmp home.png
```

Screens: home, settings, browse, recents, opds, file_transfer, network_mode, wifi_scan, keyboard, reader

### List / Delete Files

```bash
# List root
curl -s "http://192.168.0.234/api/files?path=/" | python3 -m json.tool
# Delete a file
curl -X POST "http://192.168.0.234/delete?path=/Books/chip/old.epub&type=file"
```

### Known Gotchas

| Issue | Cause | Fix |
|-------|-------|-----|
| Flash skipped, old version keeps running | Version string in `platformio.ini` matches running firmware | Bump version before building |
| Icons render garbled | Icon bitmap is 32×32 but `drawIcon()` renders at 24×24 (row-width mismatch) | Regenerate icons as 24×24 (72 bytes) |
| Reader stays offline after screenshot tour | `dangerZoneAutoConnect()` skipped when DZ disabled | `reconnectWifiAfterTour()` helper in `main.cpp` handles this |
| New firmware boots then rolls back | Firmware crashes before `markValid()` | Check crash_report.txt at SD root; fix the bug |
| OPDS shows WiFi scan screen | Reader is offline — OPDS redirects to WiFi selection | Expected; title fix only applies when connected |
| Reader won't wake for USB flash | Port disappears when screen sleeps | Tap screen first, port returns within seconds |

---

## Troubleshooting

| Symptom | Where to look |
|---------|---------------|
| Sync not starting | Check feed URL is set in Settings → NETW |
| Feed fetched but no files downloaded | Check `/.crosspoint/feed_sync_time.txt`; pull SD and inspect boot.log |
| Firmware not flashing | Confirm `firmware.bin` is in SD root; check `/.crosspoint/ota_error.log` |
| Device stuck in flash loop | Remove `firmware.bin` from SD card root; check GUID dedup in feed |
| Firmware re-flashing same version | GUID in feed must change between releases |
| Device booting old firmware after USB flash | otadata points to wrong partition — use https://xteink.dve.al/debug → "Swap boot partition" |
| Feed sync not triggering after WiFi connect | Hold Up/Down suppresses sync for that session — reconnect without holding |

---

## Original CrossPoint documentation

For hardware specs, standard EPUB reader features, development setup, contributing guidelines, and the full internals writeup, see **[CROSSPOINT-README.md](CROSSPOINT-README.md)**.

---

*CrossPoint OpenClaw is not affiliated with Xteink. Built on top of the open-source [CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader) project.*
