#include <catch2/catch_test_macros.hpp>

#include <pugixml.hpp>

#include "upnp/SoapBrowseRequestBuilder.hpp"

using upnp::BrowseFlag;
using upnp::BrowseParams;
using upnp::SoapBrowseRequestBuilder;

namespace {
constexpr const char* kServiceType = "urn:schemas-upnp-org:service:ContentDirectory:1";
}

TEST_CASE("builds a Browse envelope with all arguments", "[builder][soap]") {
    BrowseParams params;
    params.objectId = "music/albums";
    params.browseFlag = BrowseFlag::DirectChildren;
    params.startingIndex = 20;
    params.requestedCount = 50;

    auto req = SoapBrowseRequestBuilder::build(kServiceType, params);

    // The body must itself be valid XML with the arguments round-trippable.
    pugi::xml_document doc;
    REQUIRE(doc.load_string(req.body.c_str()));

    auto browse = doc.child("s:Envelope").child("s:Body").child("u:Browse");
    REQUIRE(browse);
    CHECK(std::string(browse.attribute("xmlns:u").value()) == kServiceType);
    CHECK(std::string(browse.child("ObjectID").child_value()) == "music/albums");
    CHECK(std::string(browse.child("BrowseFlag").child_value()) == "BrowseDirectChildren");
    CHECK(std::string(browse.child("Filter").child_value()) == "*");
    CHECK(std::string(browse.child("StartingIndex").child_value()) == "20");
    CHECK(std::string(browse.child("RequestedCount").child_value()) == "50");
}

TEST_CASE("SOAPAction header matches the service version", "[builder][soap]") {
    auto req = SoapBrowseRequestBuilder::build(
        "urn:schemas-upnp-org:service:ContentDirectory:2", {});
    CHECK(req.soapAction == "\"urn:schemas-upnp-org:service:ContentDirectory:2#Browse\"");
    CHECK(req.contentType == "text/xml; charset=\"utf-8\"");
}

TEST_CASE("BrowseMetadata flag is serialized correctly", "[builder][soap]") {
    BrowseParams params;
    params.browseFlag = BrowseFlag::Metadata;
    auto req = SoapBrowseRequestBuilder::build(kServiceType, params);
    CHECK(req.body.find("BrowseMetadata") != std::string::npos);
}

TEST_CASE("ObjectID with XML special characters is escaped", "[builder][soap]") {
    BrowseParams params;
    params.objectId = R"(a<b&"c")";
    auto req = SoapBrowseRequestBuilder::build(kServiceType, params);

    pugi::xml_document doc;
    REQUIRE(doc.load_string(req.body.c_str()));
    auto browse = doc.child("s:Envelope").child("s:Body").child("u:Browse");
    CHECK(std::string(browse.child("ObjectID").child_value()) == R"(a<b&"c")");
    // raw body must not contain the unescaped sequence
    CHECK(req.body.find("a<b&") == std::string::npos);
}
