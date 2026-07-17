#include <catch2/catch_test_macros.hpp>

#include "TestHelpers.hpp"
#include "upnp/DidlLiteParser.hpp"
#include "upnp/UpnpError.hpp"

using upnp::DidlLiteParser;
using upnp::UpnpObjectType;

TEST_CASE("parses containers with childCount", "[parser][didl]") {
    auto objects = DidlLiteParser::parse(readFixture("didl_lite/containers.xml"));

    REQUIRE(objects.size() == 2);
    CHECK(objects[0].type == UpnpObjectType::Container);
    CHECK(objects[0].id == "music");
    CHECK(objects[0].parentId == "0");
    CHECK(objects[0].title == "Music");
    REQUIRE(objects[0].childCount.has_value());
    CHECK(*objects[0].childCount == 120);
    CHECK(objects[0].resources.empty());

    CHECK(objects[1].id == "music/albums");
    CHECK(*objects[1].childCount == 34);
}

TEST_CASE("parses audio items with full and minimal metadata", "[parser][didl]") {
    auto objects = DidlLiteParser::parse(readFixture("didl_lite/audio_items.xml"));

    REQUIRE(objects.size() == 2);

    const auto& full = objects[0];
    CHECK(full.type == UpnpObjectType::AudioItem);
    CHECK(full.title == "Example Track");
    CHECK(full.artist == "Example Artist");
    REQUIRE(full.artists.size() == 2);
    CHECK(full.artists[0].name == "Example Artist");
    CHECK_FALSE(full.artists[0].role.has_value());
    CHECK(full.artists[1].name == "Example Album Artist");
    CHECK(full.artists[1].role == "AlbumArtist");
    CHECK(full.albumArtist == "Example Album Artist");
    CHECK(full.album == "Example Album");
    CHECK(full.genre == "Jazz");
    CHECK(full.creator == "Example Creator");
    CHECK(full.date == "2024-05-06");
    CHECK(full.originalTrackNumber == "7");
    CHECK(full.discNumber == "2");
    CHECK(full.totalDiscs == "3");
    CHECK(full.longDescription == "Mastered from original tapes");
    CHECK(full.albumArtUri == "http://192.168.1.10:8200/AlbumArt/1-123.jpg");
    REQUIRE(full.resources.size() == 1);
    CHECK(full.resources[0].url == "http://192.168.1.10:8200/media/track-123.flac");
    CHECK(full.resources[0].mimeType == "audio/flac");
    CHECK(full.resources[0].duration == "0:04:12.000");
    CHECK(full.resources[0].size == 31415926);
    CHECK(full.resources[0].bitrate == 120000);
    CHECK(full.resources[0].bitsPerSample == 24);
    CHECK(full.resources[0].sampleFrequency == 96000);
    CHECK(full.resources[0].nrAudioChannels == 2);

    const auto& minimal = objects[1];
    CHECK(minimal.type == UpnpObjectType::AudioItem);
    CHECK_FALSE(minimal.artist.has_value());
    CHECK(minimal.artists.empty());
    CHECK_FALSE(minimal.albumArtUri.has_value());
    REQUIRE(minimal.resources.size() == 1);
    CHECK(minimal.resources[0].mimeType == "audio/mpeg");
    CHECK_FALSE(minimal.resources[0].size.has_value());
}

TEST_CASE("parses all res elements including malformed protocolInfo",
          "[parser][didl]") {
    auto objects = DidlLiteParser::parse(readFixture("didl_lite/multiple_resources.xml"));

    REQUIRE(objects.size() == 1);
    REQUIRE(objects[0].resources.size() == 4);

    CHECK(objects[0].resources[0].mimeType == "audio/flac");
    CHECK(objects[0].resources[0].protocolInfo ==
          "http-get:*:audio/flac:DLNA.ORG_PN=FLAC;DLNA.ORG_OP=01");
    CHECK(objects[0].resources[1].mimeType == "audio/mpeg");
    CHECK(objects[0].resources[2].mimeType == "audio/x-rtp");
    // missing protocolInfo -> empty mimeType, not a crash (ADR-002)
    CHECK(objects[0].resources[3].protocolInfo.empty());
    CHECK(objects[0].resources[3].mimeType.empty());
}

TEST_CASE("preserves unicode titles and unescapes entities", "[parser][didl]") {
    auto objects = DidlLiteParser::parse(readFixture("didl_lite/unicode_titles.xml"));

    REQUIRE(objects.size() == 3);
    CHECK(objects[0].title == "華語流行");
    CHECK(objects[1].title == "月亮代表我的心 — 鄧麗君");
    CHECK(objects[1].artist == "鄧麗君");
    CHECK(objects[2].title == "Amp & Volt <Live>");
}

// Captured verbatim from MiniDLNA 1.3.3 (docker vladgh/minidlna,
// 2026-07-18, issue #6). Notable shapes: refID attribute, dlna
// namespace on albumArtURI (dlna:profileID), DLNA parameters as the
// 4th protocolInfo field, and MiniDLNA's tag mapping — ID3 album
// artist (TPE2) lands in upnp:artist while track artist (TPE1) lands
// in dc:creator.
TEST_CASE("parses a real MiniDLNA 1.3.3 browse payload", "[parser][didl]") {
    auto objects =
        DidlLiteParser::parse(readFixture("didl_lite/minidlna_133_audio_items.xml"));

    REQUIRE(objects.size() == 4);
    const auto& first = objects[0];
    CHECK(first.id == "1$4$0");
    CHECK(first.title == "First Song");
    CHECK(first.type == UpnpObjectType::AudioItem);
    REQUIRE(first.artist.has_value());
    CHECK(*first.artist == "Mini Album Artist"); // TPE2 via upnp:artist
    REQUIRE(first.creator.has_value());
    CHECK(*first.creator == "Mini Artist"); // TPE1 via dc:creator
    CHECK(first.album == "Mini Test Album");
    CHECK(first.date == "2024-01-01"); // MiniDLNA normalizes "2024"
    CHECK(first.originalTrackNumber == "1");
    REQUIRE(first.albumArtUri.has_value());
    CHECK(first.albumArtUri->find("/AlbumArt/") != std::string::npos);

    REQUIRE(first.resources.size() == 1);
    const auto& res = first.resources[0];
    // DLNA params are the 4th protocolInfo field; mimeType stays clean.
    CHECK(res.mimeType == "audio/mpeg");
    CHECK(res.protocolInfo.find("DLNA.ORG_PN=MP3") != std::string::npos);
    CHECK(res.duration == "0:00:04.075");
    CHECK(res.bitrate == 128000u);
    CHECK(res.sampleFrequency == 44100u);
    CHECK(res.nrAudioChannels == 2u);

    CHECK(objects[1].title == "第二首歌");
    CHECK(objects[2].title == "三曲目のテスト");
}

TEST_CASE("empty input yields empty vector", "[parser][didl]") {
    CHECK(DidlLiteParser::parse("").empty());
}

TEST_CASE("malformed or non-DIDL XML throws XmlParseException", "[parser][didl]") {
    CHECK_THROWS_AS(DidlLiteParser::parse("<DIDL-Lite><container>"),
                    upnp::XmlParseException);
    CHECK_THROWS_AS(DidlLiteParser::parse("<html>oops</html>"),
                    upnp::XmlParseException);
}
