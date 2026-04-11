#!/usr/bin/env python3
"""Submit URLs from sitemap.xml to IndexNow (Bing/Yandex).

Usage:
  python3 Docs/submit_indexnow.py

Optional environment overrides:
  INDEXNOW_HOST           default: jennys-stm32-dsp-audio-engine.netlify.app
  INDEXNOW_KEY            default: 6f8d3c1ab9e24d479a1f6c50d3e8b742
  INDEXNOW_KEY_LOCATION   default: https://<host>/<key>.txt
  INDEXNOW_ENDPOINT       default: https://api.indexnow.org/indexnow
  INDEXNOW_SITEMAP        default: Docs/site/sitemap.xml
"""

from __future__ import annotations

import json
import os
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from urllib import request
from urllib.parse import urlparse
from urllib.error import HTTPError, URLError


def parse_sitemap_urls(sitemap_path: Path) -> list[str]:
    if not sitemap_path.exists():
        raise FileNotFoundError(f"Sitemap not found: {sitemap_path}")

    tree = ET.parse(sitemap_path)
    root = tree.getroot()

    ns_uri = ""
    if root.tag.startswith("{") and "}" in root.tag:
        ns_uri = root.tag[1 : root.tag.index("}")]

    if ns_uri:
        ns = {"sm": ns_uri}
        loc_nodes = root.findall("sm:url/sm:loc", ns)
    else:
        loc_nodes = root.findall("url/loc")

    urls = [node.text.strip() for node in loc_nodes if node.text and node.text.strip()]
    if not urls:
        raise ValueError("No URLs found in sitemap.xml")

    return urls


def submit_indexnow(endpoint: str, payload: dict[str, object]) -> tuple[int, str]:
    body = json.dumps(payload).encode("utf-8")
    req = request.Request(
        endpoint,
        data=body,
        headers={
            "Content-Type": "application/json; charset=utf-8",
            "User-Agent": "Doorchime-CR2-IndexNow/1.0",
        },
        method="POST",
    )

    with request.urlopen(req, timeout=20) as resp:
        status = resp.getcode()
        text = resp.read().decode("utf-8", errors="replace")
        return status, text


def infer_host_from_urls(urls: list[str]) -> str:
    for value in urls:
        parsed = urlparse(value)
        if parsed.netloc:
            return parsed.netloc.strip()
    return ""


def main() -> int:
    key = os.getenv("INDEXNOW_KEY", "6f8d3c1ab9e24d479a1f6c50d3e8b742").strip()
    endpoint = os.getenv("INDEXNOW_ENDPOINT", "https://api.indexnow.org/indexnow").strip()

    sitemap_env = os.getenv("INDEXNOW_SITEMAP", "Docs/site/sitemap.xml")
    repo_root = Path(__file__).resolve().parents[1]
    sitemap_path = (repo_root / sitemap_env).resolve()

    try:
        urls = parse_sitemap_urls(sitemap_path)
    except (FileNotFoundError, ET.ParseError, ValueError) as err:
        print(f"Failed to parse sitemap: {err}", file=sys.stderr)
        return 2

    host = os.getenv("INDEXNOW_HOST", "").strip() or infer_host_from_urls(urls)

    if not host or not key:
        print("INDEXNOW_HOST and INDEXNOW_KEY must be non-empty", file=sys.stderr)
        return 2

    default_key_location = f"https://{host}/{key}.txt"
    key_location = os.getenv("INDEXNOW_KEY_LOCATION", default_key_location).strip()

    payload = {
        "host": host,
        "key": key,
        "keyLocation": key_location,
        "urlList": urls,
    }

    try:
        status, response_text = submit_indexnow(endpoint, payload)
    except HTTPError as err:
        body = err.read().decode("utf-8", errors="replace")
        print(f"IndexNow HTTP error: {err.code} {err.reason}", file=sys.stderr)
        if body:
            print(body, file=sys.stderr)
        return 1
    except URLError as err:
        print(f"IndexNow connection error: {err}", file=sys.stderr)
        return 1

    print(f"Submitted {len(urls)} URL(s) to IndexNow.")
    print(f"Endpoint: {endpoint}")
    print(f"Status: {status}")
    if response_text:
        print(f"Response: {response_text}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
