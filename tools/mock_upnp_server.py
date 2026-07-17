#!/usr/bin/env python3
"""Minimal mock UPnP MediaServer for end-to-end testing of upnp-browser-cli
and manual acceptance of the foo_dms_browser component (docs/21).

Serves a MiniDLNA-shaped rootDesc.xml and answers ContentDirectory
Browse with canned DIDL-Lite. Object tree:

    0 (root)
      music        container, 2 children
        track-1    FLAC + MP3 audio item
        track-2    MP3 audio item (unicode title)
      video        container, 0 children
      mixed        container, 3 children — full-metadata + skipped fixtures
        rich-track    every metadata field + albumArtURI + res attributes
        nores-track   audio item WITHOUT any <res> (must be skipped on add)
        plain-track   normal MP3 item
      broken       container; browsing it returns SOAP fault 701
      slow         container, 2 children; each Browse sleeps 5s
                   (capture 載入中…, UI must stay responsive)
      bigtree      container, 150 sub-containers × 100 tracks = 15,000
                   tracks; each Browse sleeps 50ms. Recursive add hits
                   the 10,000-track cap, and the delay gives a window
                   to press 取消加入 mid-scan.

Browse of any other ObjectID returns UPnP error 701 (No such object).
StartingIndex/RequestedCount are honored (RequestedCount=0 → all).
Album art is served at /art/cover.png.

Usage:
    python3 tools/mock_upnp_server.py [port]     # default port 18200
"""

import base64
import re
import sys
import time
import xml.sax.saxutils as sax
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

BIGTREE_CONTAINERS = 150
BIGTREE_TRACKS_PER_CONTAINER = 100
SLOW_DELAY_SECONDS = 5.0
BIGTREE_DELAY_SECONDS = 0.05

# 1x1 red PNG; NSImageView scales it up to a visible red square.
PNG_COVER = base64.b64decode(
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJ"
    "AAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg=="
)

ROOT_DESC = """<?xml version="1.0"?>
<root xmlns="urn:schemas-upnp-org:device-1-0">
  <specVersion><major>1</major><minor>0</minor></specVersion>
  <device>
    <deviceType>urn:schemas-upnp-org:device:MediaServer:1</deviceType>
    <friendlyName>Mock UPnP Server</friendlyName>
    <UDN>uuid:00000000-mock-0000-0000-000000000000</UDN>
    <serviceList>
      <service>
        <serviceType>urn:schemas-upnp-org:service:ContentDirectory:1</serviceType>
        <serviceId>urn:upnp-org:serviceId:ContentDirectory</serviceId>
        <controlURL>/ctl/ContentDir</controlURL>
        <eventSubURL>/evt/ContentDir</eventSubURL>
        <SCPDURL>/ContentDir.xml</SCPDURL>
      </service>
    </serviceList>
  </device>
</root>
"""

DIDL_WRAPPER = (
    '<DIDL-Lite xmlns="urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/"'
    ' xmlns:dc="http://purl.org/dc/elements/1.1/"'
    ' xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/">{entries}</DIDL-Lite>'
)

BROWSE_RESPONSE = """<?xml version="1.0" encoding="utf-8"?>
<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
  <s:Body>
    <u:BrowseResponse xmlns:u="urn:schemas-upnp-org:service:ContentDirectory:1">
      <Result>{result}</Result>
      <NumberReturned>{returned}</NumberReturned>
      <TotalMatches>{total}</TotalMatches>
      <UpdateID>1</UpdateID>
    </u:BrowseResponse>
  </s:Body>
</s:Envelope>
"""

SOAP_FAULT_701 = """<?xml version="1.0" encoding="utf-8"?>
<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
  <s:Body>
    <s:Fault>
      <faultcode>s:Client</faultcode>
      <faultstring>UPnPError</faultstring>
      <detail>
        <UPnPError xmlns="urn:schemas-upnp-org:control-1-0">
          <errorCode>701</errorCode>
          <errorDescription>No such object</errorDescription>
        </UPnPError>
      </detail>
    </s:Fault>
  </s:Body>
</s:Envelope>
"""


def container(cid, parent, title, child_count):
    return (
        f'<container id="{cid}" parentID="{parent}" restricted="1"'
        f' childCount="{child_count}">'
        f"<dc:title>{sax.escape(title)}</dc:title>"
        "<upnp:class>object.container</upnp:class></container>"
    )


def root_entries(host):
    return [
        container("music", "0", "Music", 2),
        container("video", "0", "Video", 0),
        container("mixed", "0", "Mixed Fixtures", 3),
        container("broken", "0", "Broken (SOAP fault)", 1),
        container("slow", "0", "Slow (5s per browse)", 2),
        container("bigtree", "0", "Big Tree (15000 tracks)", BIGTREE_CONTAINERS),
    ]


def music_entries(host):
    return [
        '<item id="track-1" parentID="music" restricted="1">'
        "<dc:title>First Track</dc:title>"
        "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
        "<upnp:artist>Mock Artist</upnp:artist><upnp:album>Mock Album</upnp:album>"
        '<res protocolInfo="http-get:*:audio/flac:*" duration="0:03:00.000" size="20000000">'
        f"http://{host}/media/track-1.flac</res>"
        '<res protocolInfo="http-get:*:audio/mpeg:*" duration="0:03:00.000" size="7000000">'
        f"http://{host}/media/track-1.mp3</res></item>",
        '<item id="track-2" parentID="music" restricted="1">'
        "<dc:title>第二首歌</dc:title>"
        "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
        '<res protocolInfo="http-get:*:audio/mpeg:*" duration="0:04:20.000">'
        f"http://{host}/media/track-2.mp3</res></item>",
    ]


def mixed_entries(host):
    return [
        # Every field docs/21 §4 asks to check: %artist%, %album artist%,
        # %album%, %date%, %tracknumber%, %comment%, %length_seconds%,
        # bitrate / samplerate / channels technical info, plus album art.
        '<item id="rich-track" parentID="mixed" restricted="1">'
        "<dc:title>Rich Track（全欄位）</dc:title>"
        "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
        '<upnp:artist role="AlbumArtist">Mock Album Artist</upnp:artist>'
        "<upnp:artist>Mock Artist</upnp:artist>"
        "<upnp:album>Mock Rich Album</upnp:album>"
        "<upnp:genre>Mock Genre</upnp:genre>"
        "<dc:date>2024-05-06</dc:date>"
        "<upnp:originalTrackNumber>7</upnp:originalTrackNumber>"
        "<upnp:longDescription>Mock comment text</upnp:longDescription>"
        f"<upnp:albumArtURI>http://{host}/art/cover.png</upnp:albumArtURI>"
        '<res protocolInfo="http-get:*:audio/mpeg:*" duration="0:03:30.000"'
        ' size="8400000" bitrate="40000" bitsPerSample="16"'
        ' sampleFrequency="44100" nrAudioChannels="2">'
        f"http://{host}/media/rich-track.mp3</res></item>",
        # Audio item with no <res> at all: add must skip it and report it.
        '<item id="nores-track" parentID="mixed" restricted="1">'
        "<dc:title>No Resource Track（應被略過）</dc:title>"
        "<upnp:class>object.item.audioItem.musicTrack</upnp:class></item>",
        '<item id="plain-track" parentID="mixed" restricted="1">'
        "<dc:title>Plain Track</dc:title>"
        "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
        '<res protocolInfo="http-get:*:audio/mpeg:*" duration="0:02:00.000">'
        f"http://{host}/media/plain-track.mp3</res></item>",
    ]


def slow_entries(host):
    return [
        f'<item id="slow-{i}" parentID="slow" restricted="1">'
        f"<dc:title>Slow Track {i}</dc:title>"
        "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
        '<res protocolInfo="http-get:*:audio/mpeg:*" duration="0:03:00.000">'
        f"http://{host}/media/slow-{i}.mp3</res></item>"
        for i in (1, 2)
    ]


def bigtree_entries(host):
    return [
        container(f"big-{k}", "bigtree", f"Folder {k:03d}",
                  BIGTREE_TRACKS_PER_CONTAINER)
        for k in range(BIGTREE_CONTAINERS)
    ]


def big_folder_entries(host, k):
    return [
        f'<item id="big-{k}-{i}" parentID="big-{k}" restricted="1">'
        f"<dc:title>Track {k:03d}-{i:03d}</dc:title>"
        "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
        '<res protocolInfo="http-get:*:audio/mpeg:*" duration="0:03:00.000">'
        f"http://{host}/media/big-{k}-{i}.mp3</res></item>"
        for i in range(BIGTREE_TRACKS_PER_CONTAINER)
    ]


def soap_param(body, name):
    match = re.search(f"<{name}>([^<]*)</{name}>", body)
    return match.group(1) if match else ""


class MockUpnpHandler(BaseHTTPRequestHandler):
    def _send(self, status, body, content_type="text/xml; charset=utf-8"):
        data = body if isinstance(body, bytes) else body.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        if self.path == "/rootDesc.xml":
            self._send(200, ROOT_DESC)
        elif self.path == "/art/cover.png":
            self._send(200, PNG_COVER, "image/png")
        else:
            self._send(404, "<html><body>Not Found</body></html>", "text/html")

    def _entries_for(self, object_id, host):
        """Returns (entries, delay_seconds) or None for a SOAP fault."""
        if object_id == "0":
            return root_entries(host), 0
        if object_id == "music":
            return music_entries(host), 0
        if object_id == "video":
            return [], 0
        if object_id == "mixed":
            return mixed_entries(host), 0
        if object_id == "slow":
            return slow_entries(host), SLOW_DELAY_SECONDS
        if object_id == "bigtree":
            return bigtree_entries(host), BIGTREE_DELAY_SECONDS
        match = re.fullmatch(r"big-(\d+)", object_id)
        if match and int(match.group(1)) < BIGTREE_CONTAINERS:
            return big_folder_entries(host, int(match.group(1))), \
                BIGTREE_DELAY_SECONDS
        return None  # includes "broken" and any bogus id

    def do_POST(self):
        if self.path != "/ctl/ContentDir":
            self._send(404, "<html><body>Not Found</body></html>", "text/html")
            return

        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length).decode("utf-8")
        object_id = soap_param(body, "ObjectID")
        host = self.headers.get("Host", "127.0.0.1")

        lookup = self._entries_for(object_id, host)
        if lookup is None:
            self._send(500, SOAP_FAULT_701)
            return
        entries, delay = lookup
        if delay:
            time.sleep(delay)

        try:
            start = int(soap_param(body, "StartingIndex") or 0)
            count = int(soap_param(body, "RequestedCount") or 0)
        except ValueError:
            start, count = 0, 0
        page = entries[start:start + count] if count > 0 else entries[start:]

        didl = DIDL_WRAPPER.format(entries="".join(page))
        self._send(200, BROWSE_RESPONSE.format(
            result=sax.escape(didl), returned=len(page), total=len(entries)))

    def log_message(self, fmt, *args):
        print("[mock-upnp] " + fmt % args, file=sys.stderr)


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 18200
    server = ThreadingHTTPServer(("127.0.0.1", port), MockUpnpHandler)
    print(f"[mock-upnp] serving on http://127.0.0.1:{port}/rootDesc.xml", file=sys.stderr)
    server.serve_forever()


if __name__ == "__main__":
    main()
