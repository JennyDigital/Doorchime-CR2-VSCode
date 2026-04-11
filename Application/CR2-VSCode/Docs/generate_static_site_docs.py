#!/usr/bin/env python3
"""Generate crawlable static documentation pages for the site.

Usage:
  python3 Docs/generate_static_site_docs.py

Optional environment overrides:
  SITE_BASE_URL  default: https://jennys-stm32-dsp-audio-engine.netlify.app
"""

from __future__ import annotations

import html
import json
import os
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath
from typing import Iterable

import markdown
from bs4 import BeautifulSoup


BASE_DIR = Path(__file__).resolve().parent
SITE_DIR = BASE_DIR / "site"
SITE_DOCS_DIR = SITE_DIR / "docs"
SITE_ASSETS_DOCS_DIR = SITE_DIR / "assets" / "docs"
SITE_DOCS_MANIFEST_PATH = SITE_DIR / "assets" / "docs-manifest.js"
DEFAULT_SITE_BASE_URL = "https://jennys-stm32-dsp-audio-engine.netlify.app"
PRIMARY_IMAGE = "/assets/audio-engine-logo-flashy.svg"
LOCAL_STYLESHEET_LINK = '<link rel="stylesheet" href="../styles.css">'


@dataclass(frozen=True)
class DocPage:
    source_name: str
    slug: str
    title: str
    description: str
    section_kicker: str
    changefreq: str
    priority: str


DOC_PAGES: tuple[DocPage, ...] = (
    DocPage(
        source_name="README.md",
        slug="readme.html",
        title="Doorchime CR2 Overview and Quick Start",
        description="Project overview, STM32 audio engine features, quick start steps, filter setup, and documentation map.",
        section_kicker="PROJECT OVERVIEW",
        changefreq="weekly",
        priority="0.8",
    ),
    DocPage(
        source_name="AUDIO_ENGINE_MANUAL.md",
        slug="audio-engine-manual.html",
        title="Doorchime CR2 Audio Engine Manual",
        description="Complete STM32 audio engine manual covering architecture, playback flow, filter tuning, hardware integration, and troubleshooting.",
        section_kicker="TECHNICAL MANUAL",
        changefreq="weekly",
        priority="0.8",
    ),
    DocPage(
        source_name="API_REFERENCE.md",
        slug="api-reference.html",
        title="Doorchime CR2 API Reference",
        description="Function-by-function API reference for initialization, playback control, filter configuration, DMA callbacks, and runtime queries.",
        section_kicker="API REFERENCE",
        changefreq="weekly",
        priority="0.8",
    ),
    DocPage(
        source_name="QUICK_REFERENCE.md",
        slug="quick-reference.html",
        title="Doorchime CR2 Quick Reference",
        description="Fast reference for common setup patterns, playback calls, filter controls, and practical STM32 audio engine tasks.",
        section_kicker="QUICK REFERENCE",
        changefreq="weekly",
        priority="0.7",
    ),
    DocPage(
        source_name="FUNCTION_INDEX.md",
        slug="function-index.html",
        title="Doorchime CR2 Function Index",
        description="Browsable function index for the audio engine API, organized for fast discovery of playback, control, and configuration symbols.",
        section_kicker="FUNCTION INDEX",
        changefreq="weekly",
        priority="0.7",
    ),
    DocPage(
        source_name="PORTABILITY_REPORT.md",
        slug="portability-report.html",
        title="Doorchime CR2 Portability Report",
        description="Portability guidance for adapting the STM32 audio engine to other microcontrollers, toolchains, and hardware environments.",
        section_kicker="PORTABILITY REPORT",
        changefreq="monthly",
        priority="0.6",
    ),
)

LICENSE_PAGE = DocPage(
    source_name="LICENSE",
    slug="license.html",
    title="Doorchime CR2 MIT License",
    description="MIT License terms for using, modifying, and distributing the Doorchime CR2 audio engine.",
    section_kicker="OPEN SOURCE LICENSE",
    changefreq="yearly",
    priority="0.3",
)

DOC_PAGE_BY_SOURCE = {page.source_name: page for page in DOC_PAGES}
DOC_PAGE_BY_SOURCE[LICENSE_PAGE.source_name] = LICENSE_PAGE


def get_site_base_url() -> str:
    return os.getenv("SITE_BASE_URL", DEFAULT_SITE_BASE_URL).strip().rstrip("/")


def get_source_path(source_name: str) -> Path:
    docs_path = BASE_DIR / source_name
    if docs_path.exists():
        return docs_path

    repo_root_path = BASE_DIR.parent / source_name
    if repo_root_path.exists():
        return repo_root_path

    raise FileNotFoundError(f"Source document not found: {source_name}")


def markdown_converter() -> markdown.Markdown:
    return markdown.Markdown(
        extensions=["extra", "toc", "sane_lists"],
        output_format="html5",
    )


def resolve_doc_reference(current_doc: str, target: str) -> PurePosixPath:
    current_parent = PurePosixPath(current_doc).parent
    return (current_parent / target).as_posix().lstrip("/")


def relative_href(from_dir: Path, to_path: Path) -> str:
    return Path(os.path.relpath(to_path, from_dir)).as_posix()


def is_external_link(value: str) -> bool:
    lowered = value.lower()
    return lowered.startswith(("http://", "https://", "mailto:", "tel:", "data:"))


def rewrite_markdown_links(soup: BeautifulSoup, current_doc: str, output_file: Path) -> None:
    output_dir = output_file.parent

    for anchor in soup.select("a[href]"):
        href = (anchor.get("href") or "").strip()
        if not href or href.startswith("#") or is_external_link(href):
            continue

        if href == "site/index.html":
            anchor["href"] = relative_href(output_dir, SITE_DIR / "index.html")
            continue

        if href == "site/docs.html":
            anchor["href"] = relative_href(output_dir, SITE_DIR / "docs.html")
            continue

        resolved = resolve_doc_reference(current_doc, href)
        source_name = PurePosixPath(resolved).name

        if href == "LICENSE" or source_name == "LICENSE":
            anchor["href"] = relative_href(output_dir, SITE_DOCS_DIR / LICENSE_PAGE.slug)
            continue

        suffix = PurePosixPath(source_name).suffix.lower()
        if suffix == ".md":
            target_page = DOC_PAGE_BY_SOURCE.get(source_name)
            if target_page is not None:
                anchor["href"] = relative_href(output_dir, SITE_DOCS_DIR / target_page.slug)
            else:
                anchor["href"] = relative_href(output_dir, SITE_ASSETS_DOCS_DIR / source_name)
            continue

        if suffix in {".pdf", ".png", ".jpg", ".jpeg", ".gif", ".webp", ".svg"}:
            anchor["href"] = relative_href(output_dir, SITE_ASSETS_DOCS_DIR / source_name)
            continue

        if suffix in {".h", ".c"}:
            anchor["href"] = relative_href(output_dir, SITE_DIR / "assets" / source_name)

    for image in soup.select("img[src]"):
        src = (image.get("src") or "").strip()
        if not src or is_external_link(src):
            continue

        resolved = resolve_doc_reference(current_doc, src)
        source_name = PurePosixPath(resolved).name
        image["src"] = relative_href(output_dir, SITE_ASSETS_DOCS_DIR / source_name)
        image["loading"] = "lazy"


def build_json_ld(page: DocPage, page_url: str, lastmod: str) -> str:
    payload = {
        "@context": "https://schema.org",
        "@type": "TechArticle" if page.source_name != "LICENSE" else "CreativeWork",
        "headline": page.title,
        "description": page.description,
        "url": page_url,
        "mainEntityOfPage": page_url,
        "dateModified": lastmod,
        "publisher": {
            "@type": "Organization",
            "name": "Doorchime CR2",
        },
        "about": [
            {"@type": "Thing", "name": "STM32"},
            {"@type": "Thing", "name": "Embedded audio"},
            {"@type": "Thing", "name": "Digital signal processing"},
        ],
    }
    return json.dumps(payload, indent=2)


def build_doc_page(page: DocPage, body_html: str, lastmod: str, site_base_url: str) -> str:
    page_url = f"{site_base_url}/docs/{page.slug}"
    og_image = f"{site_base_url}{PRIMARY_IMAGE}"
    json_ld = build_json_ld(page, page_url, lastmod)
    source_asset_href = f"../assets/docs/{page.source_name}"

    return f"""<!doctype html>
<html lang=\"en\">
<head>
  <meta charset=\"UTF-8\">
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">
  <title>{html.escape(page.title)} | Doorchime CR2</title>
  <meta name=\"description\" content=\"{html.escape(page.description)}\">
  <meta name=\"robots\" content=\"index,follow,max-image-preview:large,max-snippet:-1,max-video-preview:-1\">
  <meta name=\"googlebot\" content=\"index,follow,max-image-preview:large,max-snippet:-1,max-video-preview:-1\">
  <link rel=\"canonical\" href=\"{page_url}\">
  <meta property=\"og:type\" content=\"article\">
  <meta property=\"og:site_name\" content=\"Doorchime CR2\">
  <meta property=\"og:url\" content=\"{page_url}\">
  <meta property=\"og:title\" content=\"{html.escape(page.title)}\">
  <meta property=\"og:description\" content=\"{html.escape(page.description)}\">
  <meta property=\"og:image\" content=\"{og_image}\">
  <meta name=\"twitter:card\" content=\"summary_large_image\">
  <meta name=\"twitter:title\" content=\"{html.escape(page.title)}\">
  <meta name=\"twitter:description\" content=\"{html.escape(page.description)}\">
  <meta name=\"twitter:image\" content=\"{og_image}\">
    {LOCAL_STYLESHEET_LINK}
  <script type=\"application/ld+json\">{json_ld}</script>
</head>
<body>
  <div class=\"noise\"></div>

  <header class=\"hero docs-hero\">
    <nav class=\"topbar\">
      <div class=\"brand\">DOORCHIME CR2</div>
      <div class=\"actions\">
        <a class=\"btn ghost\" href=\"../index.html\">Home</a>
        <a class=\"btn ghost\" href=\"../docs.html\">Docs Hub</a>
      </div>
    </nav>

    <section class=\"panel docs-intro\">
      <p class=\"breadcrumbs\"><a href=\"../index.html\">Home</a> / <a href=\"../docs.html\">Docs</a> / <span>{html.escape(page.title)}</span></p>
      <p class=\"kicker\">{html.escape(page.section_kicker)}</p>
      <h1>{html.escape(page.title)}</h1>
      <p class=\"lead\">{html.escape(page.description)}</p>
      <div class=\"doc-meta\">Updated {html.escape(lastmod)} · Static HTML page for Google and Bing indexing</div>
      <div class=\"actions doc-actions\">
        <a class=\"btn\" href=\"{source_asset_href}\">Source File</a>
        <a class=\"btn ghost\" href=\"../docs-viewer.html?doc={page.source_name}\">Interactive Viewer</a>
      </div>
    </section>
  </header>

  <main>
    <article class=\"panel markdown-view\">{body_html}</article>
  </main>

  <footer>
    <p>Doorchime CR2 Documentation</p>
  </footer>
</body>
</html>
"""


def build_license_page(page: DocPage, text: str, lastmod: str, site_base_url: str) -> str:
    body = f"""
      <div class=\"license-shell\">
        <p class=\"license-kicker\">Open Source License</p>
        <h2>MIT License</h2>
        <pre class=\"license-text\">{html.escape(text)}</pre>
      </div>
    """
    return build_doc_page(page, body, lastmod, site_base_url)


def build_markdown_page(page: DocPage, source_path: Path, site_base_url: str) -> str:
    converter = markdown_converter()
    raw_markdown = source_path.read_text(encoding="utf-8")
    raw_html = converter.convert(raw_markdown)
    soup = BeautifulSoup(raw_html, "html.parser")
    output_path = SITE_DOCS_DIR / page.slug
    rewrite_markdown_links(soup, page.source_name, output_path)
    lastmod = datetime.fromtimestamp(source_path.stat().st_mtime, tz=timezone.utc).date().isoformat()
    return build_doc_page(page, str(soup), lastmod, site_base_url)


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text.rstrip() + "\n", encoding="utf-8")


def generate_docs_manifest() -> str:
    manifest: dict[str, str] = {}
    for doc_path in sorted(SITE_ASSETS_DOCS_DIR.iterdir()):
        if not doc_path.is_file():
            continue

        if doc_path.name == "LICENSE" or doc_path.suffix.lower() == ".md":
            manifest[doc_path.name] = doc_path.read_text(encoding="utf-8")

    payload = json.dumps(manifest, ensure_ascii=False, separators=(",", ":"))
    return f"window.__DOCS_MANIFEST__ = {payload};"


def generate_sitemap(site_base_url: str, pages: Iterable[DocPage]) -> str:
    static_entries = [
        (f"{site_base_url}/", datetime.now(timezone.utc).date().isoformat(), "weekly", "1.0"),
        (f"{site_base_url}/docs.html", datetime.now(timezone.utc).date().isoformat(), "weekly", "0.9"),
    ]

    for page in pages:
        source_path = get_source_path(page.source_name)
        lastmod = datetime.fromtimestamp(source_path.stat().st_mtime, tz=timezone.utc).date().isoformat()
        static_entries.append((f"{site_base_url}/docs/{page.slug}", lastmod, page.changefreq, page.priority))

    pdf_candidates = [
        (SITE_ASSETS_DOCS_DIR / "Filter_Report_Enhanced.pdf", "monthly", "0.5"),
        (SITE_ASSETS_DOCS_DIR / "Audio_Engine_Manual.pdf", "monthly", "0.5"),
    ]
    for pdf_path, changefreq, priority in pdf_candidates:
        if pdf_path.exists():
            lastmod = datetime.fromtimestamp(pdf_path.stat().st_mtime, tz=timezone.utc).date().isoformat()
            static_entries.append((f"{site_base_url}/{pdf_path.relative_to(SITE_DIR).as_posix()}", lastmod, changefreq, priority))

    lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        '<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">',
    ]
    for loc, lastmod, changefreq, priority in static_entries:
        lines.extend(
            [
                "  <url>",
                f"    <loc>{html.escape(loc)}</loc>",
                f"    <lastmod>{lastmod}</lastmod>",
                f"    <changefreq>{changefreq}</changefreq>",
                f"    <priority>{priority}</priority>",
                "  </url>",
            ]
        )
    lines.append("</urlset>")
    return "\n".join(lines)


def generate_robots(site_base_url: str) -> str:
    return "\n".join(
        [
            "User-agent: *",
            "Allow: /",
            "",
            f"Sitemap: {site_base_url}/sitemap.xml",
        ]
    )


def main() -> int:
    site_base_url = get_site_base_url()
    SITE_DOCS_DIR.mkdir(parents=True, exist_ok=True)

    for page in DOC_PAGES:
        source_path = get_source_path(page.source_name)
        html_page = build_markdown_page(page, source_path, site_base_url)
        write_text(SITE_DOCS_DIR / page.slug, html_page)

    license_path = get_source_path(LICENSE_PAGE.source_name)
    license_lastmod = datetime.fromtimestamp(license_path.stat().st_mtime, tz=timezone.utc).date().isoformat()
    write_text(
        SITE_DOCS_DIR / LICENSE_PAGE.slug,
        build_license_page(LICENSE_PAGE, license_path.read_text(encoding="utf-8"), license_lastmod, site_base_url),
    )

    write_text(SITE_DOCS_MANIFEST_PATH, generate_docs_manifest())
    write_text(SITE_DIR / "sitemap.xml", generate_sitemap(site_base_url, (*DOC_PAGES, LICENSE_PAGE)))
    write_text(SITE_DIR / "robots.txt", generate_robots(site_base_url))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())