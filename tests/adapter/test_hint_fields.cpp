#include <catch2/catch_test_macros.hpp>

#include <algorithm>

#include "component/HintFields.hpp"

using component::hintFieldsFor;
using component::parseDidlDuration;
using upnp::UpnpObject;
using upnp::UpnpObjectType;
using upnp::UpnpResource;

namespace {

std::optional<std::string> metaValue(const component::HintData& data,
                                     const std::string& field) {
    auto it = std::find_if(data.meta.begin(), data.meta.end(), [&](const auto& pair) {
        return pair.first == field;
    });
    if (it == data.meta.end()) return std::nullopt;
    return it->second;
}

std::optional<std::string> infoValue(const component::HintData& data,
                                     const std::string& field) {
    auto it = std::find_if(data.info.begin(), data.info.end(), [&](const auto& pair) {
        return pair.first == field;
    });
    if (it == data.info.end()) return std::nullopt;
    return it->second;
}

} // namespace

TEST_CASE("didl duration parsing", "[hints]") {
    CHECK(parseDidlDuration("0:03:25") == 205.0);
    CHECK(parseDidlDuration("1:02:03") == 3723.0);
    CHECK(parseDidlDuration("02:03") == 123.0);
    CHECK(parseDidlDuration("0:00:30.500") == 30.5);
    CHECK_FALSE(parseDidlDuration("").has_value());
    CHECK_FALSE(parseDidlDuration("abc").has_value());
    CHECK_FALSE(parseDidlDuration("1:2:3:4").has_value());
    CHECK_FALSE(parseDidlDuration("-1:00").has_value());
    CHECK_FALSE(parseDidlDuration("1:").has_value());
}

TEST_CASE("full didl object maps to all hint fields", "[hints]") {
    UpnpObject object;
    object.type = UpnpObjectType::AudioItem;
    object.title = "第二首歌";
    object.artist = "Some Artist";
    object.albumArtist = "Some Album Artist";
    object.album = "Some Album";
    object.genre = "Jazz";
    object.creator = "Some Creator";
    object.date = "2024-05-06";
    object.originalTrackNumber = "07";
    object.discNumber = "2";
    object.totalDiscs = "3";
    object.longDescription = "Ripped with confidence";

    const auto data = hintFieldsFor(object, "0:03:25");
    REQUIRE(data.meta.size() == 11);
    CHECK(metaValue(data, "title") == "第二首歌");
    CHECK(metaValue(data, "artist") == "Some Artist");
    CHECK(metaValue(data, "album artist") == "Some Album Artist");
    CHECK(metaValue(data, "album") == "Some Album");
    CHECK(metaValue(data, "genre") == "Jazz");
    CHECK(metaValue(data, "date") == "2024-05-06");
    CHECK(metaValue(data, "tracknumber") == "07");
    CHECK(metaValue(data, "discnumber") == "2");
    CHECK(metaValue(data, "totaldiscs") == "3");
    CHECK(metaValue(data, "comment") == "Ripped with confidence");
    CHECK(metaValue(data, "creator") == "Some Creator");
    CHECK(data.lengthSeconds == 205.0);
}

TEST_CASE("creator falls back to artist when upnp artist is absent", "[hints]") {
    UpnpObject object;
    object.type = UpnpObjectType::AudioItem;
    object.title = "Only Creator";
    object.creator = "Creator Name";

    const auto data = hintFieldsFor(object, "0:01:00");
    CHECK(metaValue(data, "artist") == "Creator Name");
    CHECK_FALSE(metaValue(data, "creator").has_value());
}

TEST_CASE("resource technical metadata maps to info hints", "[hints]") {
    UpnpObject object;
    object.type = UpnpObjectType::AudioItem;
    object.title = "Technical Track";

    UpnpResource resource;
    resource.duration = "0:03:25";
    resource.bitrate = 176400;
    resource.bitsPerSample = 16;
    resource.sampleFrequency = 44100;
    resource.nrAudioChannels = 2;

    const auto data = hintFieldsFor(object, resource);
    CHECK(infoValue(data, "bitrate") == "1412");
    CHECK(infoValue(data, "bitspersample") == "16");
    CHECK(infoValue(data, "samplerate") == "44100");
    CHECK(infoValue(data, "channels") == "2");
    CHECK(data.lengthSeconds == 205.0);
}

TEST_CASE("absent optionals produce no hint fields", "[hints]") {
    UpnpObject object;
    object.type = UpnpObjectType::AudioItem;
    object.title = "Only Title";

    const auto data = hintFieldsFor(object, "");
    REQUIRE(data.meta.size() == 1);
    CHECK(data.meta[0].first == "title");
    CHECK_FALSE(data.lengthSeconds.has_value());
}

TEST_CASE("empty-string optionals are skipped", "[hints]") {
    UpnpObject object;
    object.type = UpnpObjectType::AudioItem;
    object.title = "T";
    object.artist = std::string{};
    object.album = std::string{};

    const auto data = hintFieldsFor(object, "bogus");
    CHECK(data.meta.size() == 1);
    CHECK_FALSE(data.lengthSeconds.has_value());
}
