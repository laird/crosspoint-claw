# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

CrossPoint Reader is open-source replacement firmware for the **Xteink X4** e-paper reader, targeting an **ESP32-C3** microcontroller with a 480×800 e-ink display. Built with **PlatformIO + Arduino framework** (C++20).

Key constraints: ~380KB usable RAM; aggressive SD card caching to compensate; no dynamic allocation in hot paths.

## Build Commands

```bash
# Build firmware
pio run

# Flash via USB
pio run --target upload

# Build and copy to SD card (SD card OTA install)
# The SD card device letter changes between reconnections (sda, sdb, etc.) — always use lsblk to find it.
# Always verify the file landed on the card before ejecting.
SDCARD=$(lsblk -rno NAME,MOUNTPOINT | awk '/70B0-0D19/{print $2}') && \
  cp .pio/build/default/firmware.bin "$SDCARD/firmware.bin" && \
  ls -lh "$SDCARD/firmware.bin" && \
  udisksctl unmount -b /dev/$(lsblk -rno NAME,MOUNTPOINT | awk '/70B0-0D19/{print $1}')

# Run i18n code generator (required after editing any translation YAML)
python3 scripts/gen_i18n.py lib/I18n/translations lib/I18n/

# Serial debug monitor
python3 scripts/debugging_monitor.py          # Linux
python3 scripts/debugging_monitor.py /dev/cu.usbmodem2101  # macOS
```

There are no unit tests — the `test/` directory contains only EPUB fixtures and a hyphenation evaluator. Testing is done on-device.

## Architecture

### Activity System

The UI is structured as a stack of **Activities** (`src/activities/`). Each Activity runs a FreeRTOS render task loop and handles input events. `main.cpp` manages the stack via `enterNewActivity()` / `exitActivity()`.

Activity categories: `boot_sleep/`, `browser/`, `home/`, `network/`, `reader/`, `settings/`, `util/`.

The base class (`src/activities/Activity.h`) provides:
- A FreeRTOS task for rendering (`renderTaskLoop`)
- A render mutex (RAII `RenderLock`) to prevent deletion mid-render
- `onEnter()` / `onExit()` / `loop()` lifecycle hooks
- `requestUpdate()` / `requestUpdateAndWait()` to trigger redraws

### Rendering Pipeline

`GfxRenderer` (`lib/GfxRenderer/`) wraps the e-ink HAL and provides all drawing primitives:
- `fillRect`, `fillRoundedRect` (with per-corner radius control)
- `drawText(fontId, x, y, text, black, style)` — `black=false` renders white text
- `displayBuffer(RefreshMode)` — flushes to screen (FAST_REFRESH or FULL_REFRESH)

**Color enum** (4-bit dithered): `White=0x01`, `LightGray=0x05`, `DarkGray=0x0A`, `Black=0x10`

**Font IDs** are defined in `src/fontIds.h` (e.g. `UI_10_FONT_ID`, `SMALL_FONT_ID`).

The HAL lives in the git submodule `open-x4-sdk/` (SDCardManager, InputManager, BatteryMonitor, EInkDisplay).

### Theme System

`UITheme` singleton (`src/components/UITheme.h`, accessed via `GUI` macro) selects between themes based on `CrossPointSettings::UI_THEME`. Themes inherit from `BaseTheme` (`src/components/themes/BaseTheme.h`).

To add a new theme:
1. Create `src/components/themes/<name>/` with `<Name>Theme.h` and `<Name>Theme.cpp`
2. Define a `<Name>Metrics::values` constexpr `ThemeMetrics` struct
3. Override `BaseTheme` virtual methods as needed
4. Add enum value to `CrossPointSettings::UI_THEME` in `src/CrossPointSettings.h`
5. Add `case` in `UITheme::setTheme()` in `src/components/UITheme.cpp`
6. Add `StrId::STR_THEME_<NAME>` to the enum options in `src/SettingsList.h`
7. Add the string key to `lib/I18n/translations/english.yaml` and run `gen_i18n.py`

See `LyraTheme` for the canonical example.

### Settings System

`CrossPointSettings` (`src/CrossPointSettings.h`) is a singleton with typed enums for every setting. `src/SettingsList.h` declares `getSettingsList()` — a shared list used by both the on-device settings UI and the web API. Each entry is a `SettingInfo::Enum`, `SettingInfo::Toggle`, or `SettingInfo::Action`.

### Internationalization (I18N)

**Do not edit** `lib/I18n/I18nKeys.h`, `I18nStrings.h`, or `I18nStrings.cpp` — they are auto-generated.

Workflow for new strings:
1. Add key to `lib/I18n/translations/english.yaml`
2. Run `python3 scripts/gen_i18n.py lib/I18n/translations lib/I18n/`
3. Use `tr(STR_MY_KEY)` macro in C++ code

Missing translations fall back to English automatically. `gen_i18n.py` also runs automatically as a PlatformIO pre-build script.

### SD Card OTA

On boot, `main.cpp` checks for `/firmware.bin` on the SD card. If found, it flashes it using the ESP32 `Update` library (with a progress bar displayed), deletes the file, and restarts. This is the primary development flash workflow when USB is unavailable.

**Important:** The SD card's Linux device letter (`sda`, `sdb`, etc.) changes unpredictably between reconnections. Always run `lsblk` to find the current mount point before copying. After copying, always verify with `ls -lh` that the file is actually on the card — a silently wrong path means the firmware never lands there and the device won't update. OTA only runs at boot/deep-sleep-wake (`setup()`), not while the device is running.

### Data Storage

All persistent data lives under `/.crosspoint/` on the SD card:
- `epub_<hash>/` — per-book cache (cover BMP, parsed chapter `.bin` files, reading progress)
- `language.bin` — language preference
- Settings stored via `CrossPointSettings`

The `.crosspoint` cache directory can be deleted to force a full re-parse.
