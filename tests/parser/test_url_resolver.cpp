#include <catch2/catch_test_macros.hpp>

#include "upnp/UrlResolver.hpp"

using upnp::UrlResolver;

// Test matrix from ADR-004.
TEST_CASE("UrlResolver resolves controlURL variants", "[parser][url]") {
    SECTION("root-relative path against rootDesc URL") {
        UrlResolver r("http://a:8200/rootDesc.xml");
        CHECK(r.resolve("/cds") == "http://a:8200/cds");
    }

    SECTION("absolute controlURL is returned as-is") {
        UrlResolver r("http://a:8200/rootDesc.xml");
        CHECK(r.resolve("http://b:9000/cds") == "http://b:9000/cds");
    }

    SECTION("URLBase overrides rootDesc URL") {
        UrlResolver r("http://a:8200/rootDesc.xml", "http://a:8200/");
        CHECK(r.resolve("/cds") == "http://a:8200/cds");
    }

    SECTION("rootDesc URL with a path still resolves root-relative from host") {
        UrlResolver r("http://a:8200/path/rootDesc.xml");
        CHECK(r.resolve("/cds") == "http://a:8200/cds");
    }

    SECTION("relative path resolves against base directory") {
        UrlResolver r("http://a:8200/rootDesc.xml");
        CHECK(r.resolve("cds") == "http://a:8200/cds");
    }
}

TEST_CASE("UrlResolver edge cases", "[parser][url]") {
    SECTION("URLBase with a directory path is used for relative controlURL") {
        UrlResolver r("http://a:8200/rootDesc.xml", "http://a:8200/upnp/");
        CHECK(r.resolve("cds") == "http://a:8200/upnp/cds");
        CHECK(r.resolve("/cds") == "http://a:8200/cds"); // root-relative ignores base path
    }

    SECTION("base without any path") {
        UrlResolver r("http://a:8200");
        CHECK(r.resolve("/cds") == "http://a:8200/cds");
        CHECK(r.resolve("cds") == "http://a:8200/cds");
    }

    SECTION("nested rootDesc path resolves relative sibling") {
        UrlResolver r("http://a:8200/dev/0/rootDesc.xml");
        CHECK(r.resolve("ctl") == "http://a:8200/dev/0/ctl");
    }

    SECTION("https and uppercase scheme are accepted") {
        UrlResolver r("HTTPS://a:8443/rootDesc.xml");
        CHECK(r.resolve("/cds") == "HTTPS://a:8443/cds");
    }

    SECTION("empty controlURL throws") {
        UrlResolver r("http://a:8200/rootDesc.xml");
        CHECK_THROWS_AS(r.resolve(""), std::invalid_argument);
    }

    SECTION("non-absolute base throws") {
        CHECK_THROWS_AS(UrlResolver("rootDesc.xml"), std::invalid_argument);
        CHECK_THROWS_AS(UrlResolver("http://a/rootDesc.xml", "/base/"), std::invalid_argument);
    }
}
