# Copyright 2026 Bloomberg Finance L.P.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

"""Configuration for the FIFA 2026 ticket monitor."""

# ─── Monitoring target ────────────────────────────────────────────────────────
# FIFA 2026 ticketing base URL (update if FIFA changes the domain)
FIFA_TICKETS_BASE_URL = "https://tickets.fifa.com"

# Stadium / venue filter — MetLife Stadium is the NY/NJ host venue for 2026
TARGET_VENUE_KEYWORDS = [
    "metlife",
    "new york",
    "new jersey",
    "newark",
    "east rutherford",
]

# ─── Polling ──────────────────────────────────────────────────────────────────
# How often to check (seconds).  FIFA rate-limits aggressively; stay polite.
CHECK_INTERVAL_SECONDS = 120

# HTTP request timeout in seconds
REQUEST_TIMEOUT_SECONDS = 30

# User-Agent to send with requests
REQUEST_HEADERS = {
    "User-Agent": (
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/125.0 Safari/537.36"
    ),
    "Accept-Language": "en-US,en;q=0.9",
    "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
}

# ─── Notification ─────────────────────────────────────────────────────────────
# Set to True to enable email alerts (requires EMAIL_* settings below)
EMAIL_ENABLED = False
EMAIL_SMTP_HOST = "smtp.gmail.com"
EMAIL_SMTP_PORT = 587
EMAIL_USERNAME = ""        # e.g. youraddress@gmail.com
EMAIL_PASSWORD = ""        # App password (not your account password)
EMAIL_FROM = ""
EMAIL_TO = []              # List of recipient addresses

# Print a desktop/terminal bell on match
TERMINAL_BELL = True

# ─── Logging ──────────────────────────────────────────────────────────────────
LOG_LEVEL = "INFO"         # DEBUG | INFO | WARNING | ERROR
LOG_FILE = ""              # Leave empty to log to stdout only
