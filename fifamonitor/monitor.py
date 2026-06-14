# Copyright 2026 Bloomberg Finance L.P.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

"""
FIFA 2026 Ticket Monitor — New York / New Jersey (MetLife Stadium)

Polls the FIFA ticketing website on a configurable interval and alerts
you (terminal + optional email) the moment NY/NJ tickets appear.

Usage:
    python monitor.py [--interval SECONDS] [--once]

Options:
    --interval SECONDS   Override CHECK_INTERVAL_SECONDS from config.py
    --once               Run a single check and exit (useful for cron)
    --debug              Enable DEBUG logging
"""

import argparse
import json
import logging
import re
import sys
import time
from typing import List, Optional
from urllib.parse import urljoin

import requests
from bs4 import BeautifulSoup

import config as cfg
from notifier import TicketListing, notify

logger = logging.getLogger(__name__)


# ─── Helpers ──────────────────────────────────────────────────────────────────

def _is_nynj_venue(text: str) -> bool:
    """Return True if text mentions any of the NY/NJ venue keywords."""
    lower = text.lower()
    return any(kw in lower for kw in cfg.TARGET_VENUE_KEYWORDS)


def _make_session() -> requests.Session:
    s = requests.Session()
    s.headers.update(cfg.REQUEST_HEADERS)
    return s


# ─── Scrapers ─────────────────────────────────────────────────────────────────

def _fetch_html(session: requests.Session, url: str) -> Optional[str]:
    try:
        resp = session.get(url, timeout=cfg.REQUEST_TIMEOUT_SECONDS)
        resp.raise_for_status()
        return resp.text
    except requests.RequestException as exc:
        logger.warning("HTTP error fetching %s: %s", url, exc)
        return None


def _try_json_api(session: requests.Session) -> List[TicketListing]:
    """
    Try the unofficial FIFA ticket API endpoint first (fastest path).
    FIFA often serves match data as JSON from an internal REST endpoint;
    the exact path changes per tournament cycle so we probe a few candidates.
    """
    candidates = [
        f"{cfg.FIFA_TICKETS_BASE_URL}/api/v1/matches",
        f"{cfg.FIFA_TICKETS_BASE_URL}/api/matches",
        f"{cfg.FIFA_TICKETS_BASE_URL}/en/tickets",
    ]

    for url in candidates:
        try:
            resp = session.get(url, timeout=cfg.REQUEST_TIMEOUT_SECONDS)
            if resp.status_code != 200:
                continue
            data = resp.json()
            listings = _parse_json_matches(data, url)
            if listings is not None:
                logger.debug("JSON API matched at %s", url)
                return listings
        except (requests.RequestException, ValueError):
            continue

    return []


def _parse_json_matches(data, base_url: str) -> Optional[List[TicketListing]]:
    """Parse a generic JSON match list.  Returns None if format is unrecognised."""
    matches = None

    # Handle both {"matches": [...]} and plain [...]
    if isinstance(data, list):
        matches = data
    elif isinstance(data, dict):
        for key in ("matches", "data", "events", "results"):
            if key in data and isinstance(data[key], list):
                matches = data[key]
                break

    if matches is None:
        return None

    results: List[TicketListing] = []
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
            or f"{m.get('homeTeam', '?')} vs {m.get('awayTeam', '?')}"
        )
        date = m.get("date", "") or m.get("matchDate", "") or m.get("startTime", "")
        category = m.get("category", "") or m.get("ticketCategory", "")
        price = m.get("price", "") or m.get("minPrice", "")
        url = m.get("url", "") or m.get("ticketUrl", "") or base_url

        if not url.startswith("http"):
            url = urljoin(cfg.FIFA_TICKETS_BASE_URL, url)

        results.append(TicketListing(
            match_name=str(name),
            venue=str(venue),
            match_date=str(date),
            category=str(category),
            price=str(price),
            url=url,
        ))

    return results


def _scrape_html_page(session: requests.Session) -> List[TicketListing]:
    """
    Fall back to HTML scraping.  Parses common ticket-listing patterns from
    the FIFA ticketing site.  Because FIFA may update its markup, this uses
    broad heuristics rather than fragile CSS selectors.
    """
    url = f"{cfg.FIFA_TICKETS_BASE_URL}/en/tickets"
    html = _fetch_html(session, url)
    if not html:
        return []

    soup = BeautifulSoup(html, "html.parser")
    results: List[TicketListing] = []

    # Strategy 1: look for JSON-LD structured data embedded in the page
    for script in soup.find_all("script", type="application/ld+json"):
        try:
            data = json.loads(script.string or "")
            parsed = _parse_json_matches(data, url)
            if parsed:
                results.extend(parsed)
        except (ValueError, TypeError):
            pass

    if results:
        return results

    # Strategy 2: heuristic card / article scraping
    # Look for any element containing venue text and a ticket/buy link nearby
    candidate_containers = soup.find_all(
        ["article", "section", "div", "li"],
        class_=re.compile(r"(match|event|ticket|card|item)", re.I),
    )

    for container in candidate_containers:
        text = container.get_text(" ", strip=True)
        if not _is_nynj_venue(text):
            continue

        link_tag = container.find("a", href=True)
        link_url = link_tag["href"] if link_tag else url
        if not link_url.startswith("http"):
            link_url = urljoin(cfg.FIFA_TICKETS_BASE_URL, link_url)

        # Extract a price if visible
        price_match = re.search(r"\$[\d,]+(?:\.\d{2})?", text)
        price = price_match.group(0) if price_match else "see link"

        # Extract a date-like string
        date_match = re.search(
            r"\b(?:Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\w*\s+\d{1,2},?\s+\d{4}",
            text,
            re.I,
        )
        date_str = date_match.group(0) if date_match else "TBD"

        # Try to grab the venue more precisely from a child element
        venue_tag = container.find(
            True, string=re.compile(r"metlife|new jersey|new york", re.I)
        )
        venue = venue_tag.get_text(strip=True) if venue_tag else "MetLife Stadium, NJ"

        # First line of text is often the match name
        first_line = text.split("\n")[0][:80].strip()

        results.append(TicketListing(
            match_name=first_line or "FIFA 2026 Match",
            venue=venue,
            match_date=date_str,
            category="",
            price=price,
            url=link_url,
        ))

    return results


# ─── Core monitor loop ────────────────────────────────────────────────────────

def check_once(session: requests.Session, seen_urls: set) -> List[TicketListing]:
    """Perform one check cycle; return only listings not seen before."""
    logger.info("Checking FIFA tickets for NY/NJ…")

    # Try the JSON API first; fall back to HTML scraping
    listings = _try_json_api(session)
    if not listings:
        listings = _scrape_html_page(session)

    if not listings:
        logger.info("No NY/NJ listings found this cycle.")
        return []

    new_listings = [l for l in listings if l.url not in seen_urls]
    for l in new_listings:
        seen_urls.add(l.url)

    if new_listings:
        logger.info("Found %d new NY/NJ listing(s)!", len(new_listings))
    else:
        logger.info("Found %d listing(s) (all already seen).", len(listings))

    return new_listings


def run(interval: int = cfg.CHECK_INTERVAL_SECONDS, run_once: bool = False) -> None:
    session = _make_session()
    seen_urls: set = set()

    logger.info(
        "FIFA 2026 NY/NJ Ticket Monitor started (interval=%ds, venue keywords=%s)",
        interval,
        cfg.TARGET_VENUE_KEYWORDS,
    )

    while True:
        try:
            new_listings = check_once(session, seen_urls)
            notify(new_listings, cfg)
        except KeyboardInterrupt:
            logger.info("Monitor stopped by user.")
            sys.exit(0)
        except Exception as exc:
            logger.error("Unexpected error during check: %s", exc, exc_info=True)

        if run_once:
            break

        logger.info("Next check in %d seconds…", interval)
        try:
            time.sleep(interval)
        except KeyboardInterrupt:
            logger.info("Monitor stopped by user.")
            sys.exit(0)


# ─── Entry point ──────────────────────────────────────────────────────────────

def _setup_logging(debug: bool) -> None:
    level = logging.DEBUG if debug else getattr(logging, cfg.LOG_LEVEL, logging.INFO)
    handlers: list = [logging.StreamHandler(sys.stdout)]
    if cfg.LOG_FILE:
        handlers.append(logging.FileHandler(cfg.LOG_FILE))
    logging.basicConfig(
        level=level,
        format="%(asctime)s  %(levelname)-8s  %(name)s: %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
        handlers=handlers,
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="FIFA 2026 NY/NJ Ticket Monitor")
    parser.add_argument(
        "--interval",
        type=int,
        default=cfg.CHECK_INTERVAL_SECONDS,
        help=f"Polling interval in seconds (default: {cfg.CHECK_INTERVAL_SECONDS})",
    )
    parser.add_argument(
        "--once",
        action="store_true",
        help="Run one check and exit (useful for cron jobs)",
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="Enable DEBUG logging",
    )
    args = parser.parse_args()

    _setup_logging(args.debug)
    run(interval=args.interval, run_once=args.once)
