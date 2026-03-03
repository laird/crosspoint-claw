#!/bin/bash
# reader-push-firmware.sh — Build, verify, and push firmware to reader(s)
#
# Usage:
#   scripts/reader-push-firmware.sh [--host laird|juliette|all] [--password <dz-password>] [--reboot] [--sync]
#
# Steps:
#   1. pio run -e default
#   2. python3 scripts/verify-routes.py
#   3. Copy firmware.bin to feed server content dir
#   4. Write version to feed server .version file
#   5. Upload firmware.bin + firmware.version to reader(s)
#   6. Optionally POST /api/reboot with DZ auth (--reboot flag)
#   7. Optionally POST /api/feed/sync (--sync flag)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
FIRMWARE="$REPO_DIR/.pio/build/default/firmware.bin"
FEED_FIRMWARE="$HOME/clawd/crosspoint-feed/firmware"

# Reader IPs
IP_LAIRD="192.168.0.234"
IP_JULIETTE="192.168.0.194"

# Defaults
HOST="laird"
DZ_PASSWORD=""
REBOOT=false
SYNC=false

while [[ $# -gt 0 ]]; do
  case "$1" in
    --host)     HOST="$2"; shift 2 ;;
    --password) DZ_PASSWORD="$2"; shift 2 ;;
    --reboot)   REBOOT=true; shift ;;
    --sync)     SYNC=true; shift ;;
    -h|--help)
      echo "Usage: $0 [--host laird|juliette|all] [--password <dz-password>] [--reboot] [--sync]"
      echo "  --host      Target reader (default: laird)"
      echo "  --password  Danger Zone password for reboot"
      echo "  --reboot    POST /api/reboot after upload (requires --password)"
      echo "  --sync      POST /api/feed/sync after upload"
      exit 0 ;;
    *) echo "Unknown option: $1"; exit 1 ;;
  esac
done

if [ "$REBOOT" = true ] && [ -z "$DZ_PASSWORD" ]; then
  echo "ERROR: --reboot requires --password <dz-password>"
  exit 1
fi

# Resolve target IPs
case "$HOST" in
  laird)    TARGETS=("$IP_LAIRD") ;;
  juliette) TARGETS=("$IP_JULIETTE") ;;
  all)      TARGETS=("$IP_LAIRD" "$IP_JULIETTE") ;;
  *)        echo "Unknown host: $HOST (use laird, juliette, or all)"; exit 1 ;;
esac

cd "$REPO_DIR"

# Step 1: Build
echo "==> Building firmware..."
pio run -e default 2>&1 | tail -5
echo ""

# Verify version embedded
VERSION=$(strings "$FIRMWARE" | grep -m1 "^1\.[0-9]\." || true)
if [ -z "$VERSION" ]; then
  echo "WARNING: Could not extract version string from firmware binary"
  # Fall back to git describe
  VERSION="$(git describe --always --dirty 2>/dev/null || git rev-parse --short HEAD)"
fi
echo "    Version: $VERSION"
echo ""

# Step 2: Route check
echo "==> Verifying routes..."
python3 scripts/verify-routes.py --verbose
echo ""

# Step 3: Copy to feed server
if [ -d "$FEED_FIRMWARE" ]; then
  echo "==> Publishing to feed server..."
  cp "$FIRMWARE" "$FEED_FIRMWARE/firmware.bin"
  echo "$VERSION" > "$FEED_FIRMWARE/.version"
  echo "    Copied to $FEED_FIRMWARE/firmware.bin"
  echo ""
fi

# Step 4: Write local version file for upload
VERSION_FILE=$(mktemp)
echo "$VERSION" > "$VERSION_FILE"

# Step 5: Upload to reader(s)
for IP in "${TARGETS[@]}"; do
  echo "==> Uploading to $IP..."

  RESULT=$(curl -s --connect-timeout 5 -X POST "http://$IP/upload?path=/" \
    -F "file=@$FIRMWARE" 2>&1) || {
    echo "    FAILED: reader at $IP is unreachable"
    continue
  }
  echo "    firmware.bin: $RESULT"

  # Upload companion version file
  curl -s --connect-timeout 5 -X POST "http://$IP/upload?path=/" \
    -F "file=@$VERSION_FILE;filename=firmware.version" 2>&1 || true

  # Trigger feed sync if requested
  if [ "$SYNC" = true ]; then
    echo "    Triggering feed sync..."
    curl -s --connect-timeout 5 -X POST "http://$IP/api/feed/sync" 2>&1 || true
  fi

  # Reboot via DZ if requested
  if [ "$REBOOT" = true ]; then
    echo "    Rebooting reader..."
    REBOOT_RESULT=$(curl -s --connect-timeout 5 -X POST "http://$IP/api/reboot" \
      -H "X-Danger-Zone-Password: $DZ_PASSWORD" 2>&1) || true
    echo "    $REBOOT_RESULT"
    echo "    Waiting 45s for boot-time OTA + reboot..."
    sleep 45
    NEW_VERSION=$(curl -s --connect-timeout 5 "http://$IP/api/status" \
      | python3 -c "import sys,json; print(json.load(sys.stdin).get('version','?'))" 2>/dev/null || echo "unreachable")
    echo "    Running version: $NEW_VERSION"
    if [[ "$NEW_VERSION" == *"$VERSION"* ]] || [[ "$NEW_VERSION" == *"$(git rev-parse --short HEAD)"* ]]; then
      echo "    ✓ Flash successful"
    else
      echo "    ✗ Version mismatch — flash may have rolled back"
    fi
  fi

  echo ""
done

rm -f "$VERSION_FILE"
if [ "$REBOOT" = false ]; then
  echo "Done. Sleep/wake the reader to trigger boot-time OTA flash."
else
  echo "Done."
fi
