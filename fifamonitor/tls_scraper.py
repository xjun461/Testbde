# Copyright 2026 Bloomberg Finance L.P.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

"""
tls-client scraper backend — no browser, no display required.

tls-client wraps curl-impersonate so every HTTPS connection uses the exact
TLS fingerprint (JA3/JA4) of a real Chrome/Firefox build.  This defeats the
most common 403 trigger without spinning up a browser process.

Install: pip install tls-client
"""

import json
import logging
import re
from typing import List, Optional
from urllib.parse import urljoin

import config as cfg
from notifier import TicketListing

logger = logging.getLogger(__name__)


def _is_nynj_venue(text: str) -> bool:
    lower = text.lower()
    return any(kw in lower for kw in cfg.TARGET_VENUE_KEYWORDS)


def _make_tls_session():
    """Return a tls_client.Session impersonating Chrome 124."""
    import tls_client  # deferred so the rest of the app works without it
    session = tls_client.Session(
        client_identifier="chrome_124",   # TLS fingerprint profile
        random_tls_extension_order=True,  # randomises extension order like real Chrome
    )
    # Mimic real Chrome request headers
    session.headers = {
        "User-Agent": cfg.REQUEST_HEADERS["User-Agent"],
        "Accept": (
            "text/html,application/xhtml+xml,application/xml;q=0.9,"
            "image/avif,image/webp,*/*;q=0.8"
        ),
        "Accept-Language": "en-US,en;q=0.9",
        "Accept-Encoding": "gzip, deflate, br",
        "Connection": "keep-alive",
        "Upgrade-Insecure-Requests": "1",
        "Sec-Fetch-Dest": "document",
        "Sec-Fetch-Mode": "navigate",
        "Sec-Fetch-Site": "none",
        "Sec-Fetch-User": "?1",
        "sec-ch-ua": '"Chromium";v="124", "Google Chrome";v="124", "Not-A.Brand";v="99"',
        "sec-ch-ua-mobile": "?0",
        "sec-ch-ua-platform": '"Linux"',
    }
    return session


def _inject_cookies(session, cookies: "list[dict] | None") -> None:
    """Add saved browser cookies to the session (optional but maximally effective)."""
    if not cookies:
        return
    import http.cookiejar, requests.cookies
    for c in cookies:
        session.cookies.set(
            c["name"], c["value"],
            domain=c.get("domain", ".tickets.fifa.com"),
            path=c.get("path", "/"),
        )
    logger.debug("Injected %d cookie(s) into tls session", len(cookies))


def _fetch(session, url: str) -> Optional[str]:
    try:
        resp = session.get(url, timeout_seconds=cfg.REQUEST_TIMEOUT_SECONDS)
        if resp.status_code == 200:
            return resp.text
        logger.warning("tls-client got HTTP %d for %s", resp.status_code, url)
    except Exception as exc:
        logger.warning("tls-client request failed for %s: %s", url, exc)
    return None


# ── Parsers (reuse same logic as monitor.py) ──────────────────────────────────

def _parse_json_response(data, base_url: str) -> List[TicketListing]:
    matches = None
    if isinstance(data, list):
        matches = data
    elif isinstance(data, dict):
        for key in ("matches", "data", "events", "results"):
            if key in data and isinstance(data[key], list):
                matches = data[key]
                break
    if not matches:
        return []

    results = []
    for m in matches:
        venue = (
            m.get("venue", {}).get("name", "")
            or m.get("venueName", "")
            or m.get("location", "")
        )
        if not _is_nynj_venue(venue):
            continue
        name = (
            m.get("name", "")
            or m.get("matchName", "")
            or f"{m.get('homeTeam','?')} vs {m.get('awayTeam','?')}"
        )
        date = m.get("date", "") or m.get("matchDate", "") or m.get("startTime", "")
        url = m.get("url", "") or m.get("ticketUrl", "") or base_url
        if not url.startswith("http"):
            url = urljoin(cfg.FIFA_TICKETS_BASE_URL, url)
        results.append(TicketListing(
            match_name=str(name),
            venue=str(venue),
            match_date=str(date),
            category=str(m.get("category", "")),
            price=str(m.get("price", "") or m.get("minPrice", "see link")),
            url=url,
        ))
    return results


def _parse_html(html: str, page_url: str) -> List[TicketListing]:
    from bs4 import BeautifulSoup
    soup = BeautifulSoup(html, "lxml")
    results = []

    # JSON-LD blocks embedded in the page
    for script in soup.find_all("script", type="application/ld+json"):
        try:
            data = json.loads(script.string or "")
            results.extend(_parse_json_response(data, page_url))
        except (ValueError, TypeError):
            pass

    if results:
        return results

    # Heuristic card scraping
    for container in soup.find_all(
        ["article", "section", "div", "li"],
        class_=re.compile(r"(match|event|ticket|card|item)", re.I),
    ):
        text = container.get_text(" ", strip=True)
        if not _is_nynj_venue(text):
            continue
        link_tag = container.find("a", href=True)
        url = link_tag["href"] if link_tag else page_url
        if not url.startswith("http"):
            url = urljoin(cfg.FIFA_TICKETS_BASE_URL, url)
        price_m = re.search(r"\$[\d,]+(?:\.\d{2})?", text)
        date_m = re.search(
            r"\b(?:Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\w*\s+\d{1,2},?\s+\d{4}",
            text, re.I,
        )
        results.append(TicketListing(
            match_name=text.split("\n")[0][:80].strip() or "FIFA 2026 Match",
            venue="MetLife Stadium, NJ",
            match_date=date_m.group(0) if date_m else "TBD",
            category="",
            price=price_m.group(0) if price_m else "see link",
            url=url,
        ))
    return results


# ── Public entry point ────────────────────────────────────────────────────────

def scrape_with_tls_client(cookies: "list[dict] | None" = None) -> List[TicketListing]:
    """
    Fetch FIFA NY/NJ ticket listings using a Chrome-impersonating TLS session.

    Args:
        cookies: Optional saved browser cookies (see config.BROWSER_COOKIES).
                 Pass these to bypass consent gates or login walls.

    Returns:
        List of TicketListing for NY/NJ matches.  Empty list on failure.
    """
    try:
        session = _make_tls_session()
    except ImportError:
        logger.error("tls-client not installed. Run: pip install tls-client")
        return []

    _inject_cookies(session, cookies)

    # ── Try JSON API endpoints first (fastest) ────────────────────────────────
    api_candidates = [
        f"{cfg.FIFA_TICKETS_BASE_URL}/api/v1/matches",
        f"{cfg.FIFA_TICKETS_BASE_URL}/api/matches",
        f"{cfg.FIFA_TICKETS_BASE_URL}/api/v2/catalog",
    ]
    for url in api_candidates:
        html = _fetch(session, url)
        if not html:
            continue
        try:
            data = json.loads(html)
            listings = _parse_json_response(data, url)
            if listings:
                logger.info("tls-client JSON API hit at %s — %d listing(s)", url, len(listings))
                return listings
        except ValueError:
            pass

    # ── Fall back to HTML page ────────────────────────────────────────────────
    page_url = f"{cfg.FIFA_TICKETS_BASE_URL}/en/tickets"
    # Hit the root first so cookies/consent state is set, then the tickets page
    _fetch(session, cfg.FIFA_TICKETS_BASE_URL)
    html = _fetch(session, page_url)
    if not html:
        logger.warning("tls-client: no HTML received from %s", page_url)
        return []

    listings = _parse_html(html, page_url)
    logger.info("tls-client HTML scrape — %d NY/NJ listing(s)", len(listings))
    return listings
