#include <catch2/catch_test_macros.hpp>

#include "upnp/ResourceSelector.hpp"

using upnp::selectBestResource;
using upnp::UpnpResource;

namespace {

UpnpResource makeResource(const std::string& protocol, const std::string& mime,
                          const std::string& url) {
    UpnpResource r;
    r.protocolInfo = protocol + ":*:" + mime + ":*";
    r.mimeType = mime;
    r.url = url;
    return r;
}

} // namespace

// Test matrix from ADR-007.
TEST_CASE("selectBestResource picks by mime priority", "[selector]") {
    SECTION("single flac resource is selected") {
        auto best = selectBestResource({
            makeResource("http-get", "audio/flac", "http://s/t.flac"),
        });
        REQUIRE(best.has_value());
        CHECK(best->url == "http://s/t.flac");
    }

    SECTION("flac beats mp3 regardless of order") {
        auto best = selectBestResource({
            makeResource("http-get", "audio/mpeg", "http://s/t.mp3"),
            makeResource("http-get", "audio/flac", "http://s/t.flac"),
        });
        REQUIRE(best.has_value());
        CHECK(best->mimeType == "audio/flac");
    }

    SECTION("mp3 beats wma") {
        auto best = selectBestResource({
            makeResource("http-get", "audio/x-ms-wma", "http://s/t.wma"),
            makeResource("http-get", "audio/mpeg", "http://s/t.mp3"),
        });
        REQUIRE(best.has_value());
        CHECK(best->mimeType == "audio/mpeg");
    }
}

TEST_CASE("selectBestResource filters non-http protocols", "[selector]") {
    SECTION("rtsp-only item is unplayable") {
        auto best = selectBestResource({
            makeResource("rtsp-rtp-udp", "audio/x-rtp", "rtsp://s/t"),
        });
        CHECK_FALSE(best.has_value());
    }

    SECTION("empty resource list yields nullopt") {
        CHECK_FALSE(selectBestResource({}).has_value());
    }

    SECTION("flac wins over rtsp even when rtsp comes first") {
        auto best = selectBestResource({
            makeResource("rtsp-rtp-udp", "audio/x-rtp", "rtsp://s/t"),
            makeResource("http-get", "audio/flac", "http://s/t.flac"),
        });
        REQUIRE(best.has_value());
        CHECK(best->mimeType == "audio/flac");
    }
}

TEST_CASE("unknown mime falls back to first http-get resource", "[selector]") {
    auto best = selectBestResource({
        makeResource("http-get", "audio/weird-format", "http://s/a"),
        makeResource("http-get", "audio/other-weird", "http://s/b"),
    });
    REQUIRE(best.has_value());
    CHECK(best->url == "http://s/a");
}

TEST_CASE("caller-supplied priority overrides the default", "[selector]") {
    auto best = selectBestResource(
        {
            makeResource("http-get", "audio/flac", "http://s/t.flac"),
            makeResource("http-get", "audio/mpeg", "http://s/t.mp3"),
        },
        {"audio/mpeg", "audio/flac"});
    REQUIRE(best.has_value());
    CHECK(best->mimeType == "audio/mpeg");
}
