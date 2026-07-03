#include <catch2/catch_test_macros.hpp>

#include "component/HintFields.hpp"

using component::hintFieldsFor;
using component::parseDidlDuration;
using upnp::UpnpObject;
using upnp::UpnpObjectType;

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
    object.album = "Some Album";
    object.genre = "Jazz";

    const auto data = hintFieldsFor(object, "0:03:25");
    REQUIRE(data.meta.size() == 4);
    CHECK(data.meta[0] == std::make_pair(std::string("title"), std::string("第二首歌")));
    CHECK(data.meta[1] == std::make_pair(std::string("artist"), std::string("Some Artist")));
    CHECK(data.meta[2] == std::make_pair(std::string("album"), std::string("Some Album")));
    CHECK(data.meta[3] == std::make_pair(std::string("genre"), std::string("Jazz")));
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
