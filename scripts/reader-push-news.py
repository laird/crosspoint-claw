#!/usr/bin/env python3
"""
reader-push-news.py — Push a news item to the CrossPoint feed server.

Usage:
    python3 scripts/reader-push-news.py --title "..." --body "..." [--sync] [--host laird|juliette|all]
    echo "body content" | python3 scripts/reader-push-news.py --title "..." --body - [--sync]

The news item is written to feed-server/content/news/ as a markdown file with YAML frontmatter.
If --sync is passed, triggers /api/feed/sync on the specified reader(s) immediately.
"""

import argparse
import json
import os
import sys
import urllib.request
import urllib.error
from datetime import datetime

FEED_SERVER_NEWS_DIR = os.path.expanduser("~/src/crosspoint-claw/feed-server/content/news")

HOSTS = {
    "laird": "192.168.0.234",
    "juliette": "192.168.0.194",
}


def sanitize_filename(title: str, max_len: int = 200) -> str:
    """Convert title to a safe filename."""
    safe = "".join(c if c.isalnum() or c in "-_" else "_" for c in title.lower())
    return safe[:max_len] + ".md"


def write_news_item(title: str, body: str) -> str:
    """Write a news item to the feed server."""
    os.makedirs(FEED_SERVER_NEWS_DIR, exist_ok=True)
    
    filename = sanitize_filename(title)
    filepath = os.path.join(FEED_SERVER_NEWS_DIR, filename)
    
    now = datetime.now().isoformat()
    content = f"""---
title: {title}
date: {now}
---

{body}
"""
    
    with open(filepath, 'w') as f:
        f.write(content)
    
    return filepath


def trigger_feed_sync(host: str) -> bool:
    """POST /api/feed/sync on the reader."""
    if host not in HOSTS:
        print(f"ERROR: Unknown host '{host}'", file=sys.stderr)
        return False
    
    ip = HOSTS[host]
    url = f"http://{ip}/api/feed/sync"
    
    try:
        req = urllib.request.Request(url, method="POST")
        urllib.request.urlopen(req, timeout=10)
        return True
    except urllib.error.URLError as e:
        print(f"ERROR: Could not trigger sync on {host} ({ip}): {e}", file=sys.stderr)
        return False


def main():
    parser = argparse.ArgumentParser(description="Push a news item to CrossPoint feed server")
    parser.add_argument("--title", required=True, help="News title")
    parser.add_argument("--body", required=True, help="News body (or '-' to read from stdin)")
    parser.add_argument("--sync", action="store_true", help="Trigger /api/feed/sync after push")
    parser.add_argument("--host", default="laird", help="Target reader (laird|juliette|all)")
    
    args = parser.parse_args()
    
    # Read body from stdin if needed
    if args.body == "-":
        args.body = sys.stdin.read()
    
    # Write news item
    try:
        filepath = write_news_item(args.title, args.body)
        print(f"News item written: {filepath}")
    except Exception as e:
        print(f"ERROR: Failed to write news item: {e}", file=sys.stderr)
        sys.exit(1)
    
    # Trigger sync if requested
    if args.sync:
        hosts = ["laird", "juliette"] if args.host == "all" else [args.host]
        for host in hosts:
            if trigger_feed_sync(host):
                print(f"Feed sync triggered on {host}")
            else:
                print(f"Failed to trigger sync on {host} (reader may be offline)", file=sys.stderr)


if __name__ == "__main__":
    main()
