#!/usr/bin/env python3
"""Minimal mock UPnP MediaServer for end-to-end testing of upnp-browser-cli.

Serves a MiniDLNA-shaped rootDesc.xml and answers ContentDirectory
Browse with canned DIDL-Lite. Object tree:

    0 (root)
      music        container, 2 children
        track-1    FLAC + MP3 audio item
        track-2    MP3 audio item
      video        container, 0 children

Browse of any other ObjectID returns UPnP error 701 (No such object).

Usage:
    python3 tools/mock_upnp_server.py [port]     # default port 18200
"""

import sys
import xml.sax.saxutils as sax
from http.server import BaseHTTPRequestHandler, HTTPServer

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

DIDL_ROOT = (
    '<DIDL-Lite xmlns="urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/"'
    ' xmlns:dc="http://purl.org/dc/elements/1.1/"'
    ' xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/">'
    '<container id="music" parentID="0" restricted="1" childCount="2">'
    "<dc:title>Music</dc:title><upnp:class>object.container</upnp:class></container>"
    '<container id="video" parentID="0" restricted="1" childCount="0">'
    "<dc:title>Video</dc:title><upnp:class>object.container</upnp:class></container>"
    "</DIDL-Lite>"
)

DIDL_MUSIC_TEMPLATE = (
    '<DIDL-Lite xmlns="urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/"'
    ' xmlns:dc="http://purl.org/dc/elements/1.1/"'
    ' xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/">'
    '<item id="track-1" parentID="music" restricted="1">'
    "<dc:title>First Track</dc:title>"
    "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
    "<upnp:artist>Mock Artist</upnp:artist><upnp:album>Mock Album</upnp:album>"
    '<res protocolInfo="http-get:*:audio/flac:*" duration="0:03:00.000" size="20000000">'
    "http://{host}/media/track-1.flac</res>"
    '<res protocolInfo="http-get:*:audio/mpeg:*" duration="0:03:00.000" size="7000000">'
    "http://{host}/media/track-1.mp3</res></item>"
    '<item id="track-2" parentID="music" restricted="1">'
    "<dc:title>第二首歌</dc:title>"
    "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
    '<res protocolInfo="http-get:*:audio/mpeg:*" duration="0:04:20.000">'
    "http://{host}/media/track-2.mp3</res></item>"
    "</DIDL-Lite>"
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


class MockUpnpHandler(BaseHTTPRequestHandler):
    def _send(self, status, body, content_type="text/xml; charset=utf-8"):
        data = body.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        if self.path == "/rootDesc.xml":
            self._send(200, ROOT_DESC)
        else:
            self._send(404, "<html><body>Not Found</body></html>", "text/html")

    def do_POST(self):
        if self.path != "/ctl/ContentDir":
            self._send(404, "<html><body>Not Found</body></html>", "text/html")
            return

        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length).decode("utf-8")

        # crude ObjectID extraction; enough for a mock
        object_id = ""
        start = body.find("<ObjectID>")
        if start != -1:
            end = body.find("</ObjectID>", start)
            object_id = body[start + len("<ObjectID>"):end]

        host = self.headers.get("Host", "127.0.0.1")
        if object_id == "0":
            didl = DIDL_ROOT
            returned = total = 2
        elif object_id == "music":
            didl = DIDL_MUSIC_TEMPLATE.format(host=host)
            returned = total = 2
        else:
            self._send(500, SOAP_FAULT_701)
            return

        self._send(200, BROWSE_RESPONSE.format(
            result=sax.escape(didl), returned=returned, total=total))

    def log_message(self, fmt, *args):
        print("[mock-upnp] " + fmt % args, file=sys.stderr)


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 18200
    server = HTTPServer(("127.0.0.1", port), MockUpnpHandler)
    print(f"[mock-upnp] serving on http://127.0.0.1:{port}/rootDesc.xml", file=sys.stderr)
    server.serve_forever()


if __name__ == "__main__":
    main()
