# Copyright 2026 Bloomberg Finance L.P.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

"""Notification handlers for FIFA ticket alerts."""

import logging
import smtplib
import sys
from dataclasses import dataclass
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from typing import List

logger = logging.getLogger(__name__)


@dataclass
class TicketListing:
    match_name: str
    venue: str
    match_date: str
    category: str
    price: str
    url: str


def notify(listings: List[TicketListing], cfg) -> None:
    """Dispatch all enabled notification channels."""
    if not listings:
        return

    _notify_terminal(listings, cfg)

    if cfg.EMAIL_ENABLED:
        _notify_email(listings, cfg)


def _notify_terminal(listings: List[TicketListing], cfg) -> None:
    """Print a highlighted alert to stdout."""
    bell = "\a" if cfg.TERMINAL_BELL else ""
    separator = "=" * 70

    print(f"\n{bell}{separator}")
    print(f"  FIFA 2026 TICKETS AVAILABLE — NY/NJ (MetLife Stadium)")
    print(separator)

    for listing in listings:
        print(f"\n  Match   : {listing.match_name}")
        print(f"  Date    : {listing.match_date}")
        print(f"  Venue   : {listing.venue}")
        print(f"  Category: {listing.category}")
        print(f"  Price   : {listing.price}")
        print(f"  Link    : {listing.url}")

    print(f"\n{separator}\n")


def _notify_email(listings: List[TicketListing], cfg) -> None:
    """Send an email with the available listings."""
    if not cfg.EMAIL_TO:
        logger.warning("EMAIL_ENABLED is True but EMAIL_TO is empty — skipping email")
        return

    subject = f"FIFA 2026 NY/NJ Tickets Available ({len(listings)} listing(s))"

    body_lines = ["FIFA 2026 Tickets Available — NY/NJ (MetLife Stadium)\n"]
    for listing in listings:
        body_lines += [
            f"Match   : {listing.match_name}",
            f"Date    : {listing.match_date}",
            f"Venue   : {listing.venue}",
            f"Category: {listing.category}",
            f"Price   : {listing.price}",
            f"Link    : {listing.url}",
            "",
        ]

    body = "\n".join(body_lines)

    msg = MIMEMultipart("alternative")
    msg["Subject"] = subject
    msg["From"] = cfg.EMAIL_FROM
    msg["To"] = ", ".join(cfg.EMAIL_TO)
    msg.attach(MIMEText(body, "plain"))

    try:
        with smtplib.SMTP(cfg.EMAIL_SMTP_HOST, cfg.EMAIL_SMTP_PORT) as server:
            server.ehlo()
            server.starttls()
            server.login(cfg.EMAIL_USERNAME, cfg.EMAIL_PASSWORD)
            server.sendmail(cfg.EMAIL_FROM, cfg.EMAIL_TO, msg.as_string())
        logger.info("Email sent to %s", cfg.EMAIL_TO)
    except Exception as exc:
        logger.error("Failed to send email: %s", exc)
