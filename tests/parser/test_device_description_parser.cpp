#include <catch2/catch_test_macros.hpp>

#include "TestHelpers.hpp"
#include "upnp/DeviceDescriptionParser.hpp"
#include "upnp/UpnpError.hpp"

using upnp::DeviceDescriptionParser;

TEST_CASE("parses MiniDLNA-style device description", "[parser][device]") {
    auto desc = DeviceDescriptionParser::parse(
        readFixture("device_descriptions/minidlna_rootDesc.xml"));

    CHECK(desc.friendlyName == "MiniDLNA on nas");
    CHECK(desc.urlBase.empty());
    CHECK(desc.serviceType == "urn:schemas-upnp-org:service:ContentDirectory:1");
    CHECK(desc.controlUrl == "/ctl/ContentDir");
    CHECK(desc.eventSubUrl == "/evt/ContentDir");
    CHECK(desc.scpdUrl == "/ContentDir.xml");
}

TEST_CASE("parses Jellyfin-style device description", "[parser][device]") {
    auto desc = DeviceDescriptionParser::parse(
        readFixture("device_descriptions/jellyfin_rootDesc.xml"));

    CHECK(desc.friendlyName == "Jellyfin - media");
    CHECK(desc.controlUrl == "/dlna/0a1b2c3d4e5f607182930000/contentdirectory/control");
}

TEST_CASE("finds ContentDirectory v2 in an embedded device with URLBase",
          "[parser][device]") {
    auto desc = DeviceDescriptionParser::parse(
        readFixture("device_descriptions/urlbase_nested_rootDesc.xml"));

    CHECK(desc.urlBase == "http://192.168.1.50:9000/");
    CHECK(desc.serviceType == "urn:schemas-upnp-org:service:ContentDirectory:2");
    CHECK(desc.controlUrl == "MediaServer/ContentDirectory/Control");
}

TEST_CASE("device without ContentDirectory throws ServiceNotFound",
          "[parser][device]") {
    CHECK_THROWS_AS(
        DeviceDescriptionParser::parse(
            readFixture("device_descriptions/igd_no_contentdirectory.xml")),
        upnp::ServiceNotFoundException);
}

TEST_CASE("malformed XML throws XmlParseException", "[parser][device]") {
    CHECK_THROWS_AS(DeviceDescriptionParser::parse("<root><unclosed>"),
                    upnp::XmlParseException);
    CHECK_THROWS_AS(DeviceDescriptionParser::parse(""),
                    upnp::XmlParseException);
}
