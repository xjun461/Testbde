# Copyright 2026 Bloomberg Finance L.P.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

"""
Playwright-based scraper backend.

FIFA's site uses aggressive bot detection:
  - Checks navigator.webdriver (patched by playwright-stealth)
  - Checks TLS fingerprint  (full browser fixes this)
  - Requires JS to render ticket cards

Run `playwright install chromium` once before using this backend.
"""

import json
import logging
import re
from typing import List
from urllib.parse import urljoin

import config as cfg
from notifier import TicketListing

logger = logging.getLogger(__name__)

# Optional stealth patch — suppresses navigator.webdriver and other bot signals
try:
    from playwright_stealth import stealth_sync
    _HAS_STEALTH = True
except ImportError:
    _HAS_STEALTH = False
    logger.debug("playwright-stealth not installed; proceeding without stealth patches")


def _is_nynj_venue(text: str) -> bool:
    lower = text.lower()
    return any(kw in lower for kw in cfg.TARGET_VENUE_KEYWORDS)


def _parse_listing_from_element(el, page_url: str) -> "TicketListing | None":
    """Extract a TicketListing from a Playwright ElementHandle."""
    try:
        text = el.inner_text()
    except Exception:
        return None

    if not _is_nynj_venue(text):
        return None

    try:
        link = el.query_selector("a[href]")
        url = link.get_attribute("href") if link else page_url
        if url and not url.startswith("http"):
            url = urljoin(cfg.FIFA_TICKETS_BASE_URL, url)
    except Exception:
        url = page_url

    price_match = re.search(r"\$[\d,]+(?:\.\d{2})?", text)
    price = price_match.group(0) if price_match else "see link"

    date_match = re.search(
        r"\b(?:Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\w*\s+\d{1,2},?\s+\d{4}",
        text, re.I,
    )
    date_str = date_match.group(0) if date_match else "TBD"

    first_line = text.strip().split("\n")[0][:80]

    return TicketListing(
        match_name=first_line or "FIFA 2026 Match",
        venue="MetLife Stadium, NJ",
        match_date=date_str,
        category="",
        price=price,
        url=url or page_url,
    )


def scrape_with_playwright(
    cookies: "list[dict] | None" = None,
    headless: bool = True,
) -> List[TicketListing]:
    """
    Open the FIFA ticketing page in a real Chromium browser, wait for JS to
    render, then extract NY/NJ listings.

    Args:
        cookies: Optional list of cookie dicts (see config.BROWSER_COOKIES).
                 Each dict must have at least 'name', 'value', 'domain'.
        headless: Set False for debugging (opens a visible browser window).
    """
    try:
        from playwright.sync_api import sync_playwright, TimeoutError as PWTimeout
    except ImportError:
        logger.error(
            "playwright not installed. Run: pip install playwright && playwright install chromium"
        )
        return []

    url = f"{cfg.FIFA_TICKETS_BASE_URL}/en/tickets"
    results: List[TicketListing] = []

    with sync_playwright() as pw:
        browser = pw.chromium.launch(
            headless=headless,
            args=[
                "--disable-blink-features=AutomationControlled",
                "--no-sandbox",
                "--disable-dev-shm-usage",
            ],
        )

        context = browser.new_context(
            viewport={"width": 1280, "height": 900},
            user_agent=cfg.REQUEST_HEADERS["User-Agent"],
            locale="en-US",
            timezone_id="America/New_York",
            java_script_enabled=True,
        )

        # Inject any saved browser cookies (bypasses login walls / consent gates)
        if cookies:
            context.add_cookies(cookies)

        page = context.new_page()

        # Apply stealth patches if available
        if _HAS_STEALTH:
            stealth_sync(page)

        # Extra JS to hide automation signals
        page.add_init_script("""
            Object.defineProperty(navigator, 'webdriver', { get: () => undefined });
            Object.defineProperty(navigator, 'plugins', { get: () => [1, 2, 3] });
            Object.defineProperty(navigator, 'languages', { get: () => ['en-US', 'en'] });
            window.chrome = { runtime: {} };
        """)

        logger.info("Browser navigating to %s", url)
        try:
            page.goto(url, wait_until="networkidle", timeout=45_000)
        except PWTimeout:
            logger.warning("Page load timed out; trying to scrape partial content")

        # Give JS frameworks extra time to hydrate
        page.wait_for_timeout(3_000)

        # ── Strategy 1: JSON-LD structured data in the DOM ────────────────────
        for handle in page.query_selector_all("script[type='application/ld+json']"):
            try:
                raw = handle.inner_text()
                data = json.loads(raw)
                items = data if isinstance(data, list) else data.get("@graph", [data])
                for item in items:
                    venue = str(item.get("location", {}).get("name", ""))
                    if not _is_nynj_venue(venue):
                        continue
                    link = item.get("url", url)
                    results.append(TicketListing(
                        match_name=item.get("name", "FIFA 2026 Match"),
                        venue=venue,
                        match_date=str(item.get("startDate", "TBD")),
                        category=item.get("offers", {}).get("category", ""),
                        price=str(item.get("offers", {}).get("price", "see link")),
                        url=link,
                    ))
            except Exception:
                pass

        if results:
            logger.info("JSON-LD strategy found %d NY/NJ listing(s)", len(results))
            browser.close()
            return results

        # ── Strategy 2: heuristic card scraping ───────────────────────────────
        selectors = [
            "[class*='match']",
            "[class*='event']",
            "[class*='ticket']",
            "[class*='card']",
            "article",
            "li[class]",
        ]
        for selector in selectors:
            for el in page.query_selector_all(selector):
                listing = _parse_listing_from_element(el, url)
                if listing and listing.url not in {r.url for r in results}:
                    results.append(listing)

        if results:
            logger.info("Card scraping found %d NY/NJ listing(s)", len(results))
        else:
            logger.info("No NY/NJ listings found in page")

        browser.close()

    return results
