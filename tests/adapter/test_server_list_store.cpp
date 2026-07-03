#include <catch2/catch_test_macros.hpp>

#include "component/ServerListStore.hpp"

using component::parseServerList;
using component::serializeServerList;
using component::ServerEntry;

TEST_CASE("empty list round-trips", "[serverlist]") {
    CHECK(serializeServerList({}) == "[]");
    CHECK(parseServerList("[]").empty());
    CHECK(parseServerList(" [ ] ").empty());
}

TEST_CASE("entries round-trip through serialize/parse", "[serverlist]") {
    const std::vector<ServerEntry> entries = {
        {"客廳 NAS", "http://10.0.0.2:2333/DeviceDescription.xml"},
        {"Jellyfin", "http://media.local:8096/dlna/x/description.xml"},
    };
    const auto parsed = parseServerList(serializeServerList(entries));
    REQUIRE(parsed.size() == 2);
    CHECK(parsed[0] == entries[0]);
    CHECK(parsed[1] == entries[1]);
}

TEST_CASE("special characters survive the round-trip", "[serverlist]") {
    const std::vector<ServerEntry> entries = {
        {"quo\"te back\\slash\nnewline\ttab", "http://h/?a=1&b=\"x\""},
    };
    const auto parsed = parseServerList(serializeServerList(entries));
    REQUIRE(parsed.size() == 1);
    CHECK(parsed[0] == entries[0]);
}

TEST_CASE("unknown keys are ignored for forward compatibility", "[serverlist]") {
    const auto parsed = parseServerList(
        R"([{"name":"n","url":"u","auth":"basic","future":"x"}])");
    REQUIRE(parsed.size() == 1);
    CHECK(parsed[0].name == "n");
    CHECK(parsed[0].url == "u");
}

TEST_CASE("unicode escapes decode to utf-8", "[serverlist]") {
    const auto parsed = parseServerList(R"([{"name":"客廳","url":"u"}])");
    REQUIRE(parsed.size() == 1);
    CHECK(parsed[0].name == "客廳");
}

TEST_CASE("surrogate pairs decode to non-bmp code points", "[serverlist]") {
    const auto parsed = parseServerList(R"([{"name":"🎵","url":"u"}])");
    REQUIRE(parsed.size() == 1);
    CHECK(parsed[0].name == "\xF0\x9F\x8E\xB5"); // U+1F3B5
}

TEST_CASE("malformed input yields an empty list", "[serverlist]") {
    CHECK(parseServerList("").empty());
    CHECK(parseServerList("not json").empty());
    CHECK(parseServerList("[{\"name\":\"n\"").empty());          // truncated
    CHECK(parseServerList(R"([{"name":123,"url":"u"}])").empty()); // non-string value
    CHECK(parseServerList(R"({"name":"n"})").empty());           // not an array
    CHECK(parseServerList(R"([{"name":"\ud800broken","url":"u"}])").empty());
}

TEST_CASE("trimWhitespace strips stray whitespace", "[serverlist]") {
    using component::trimWhitespace;
    CHECK(trimWhitespace("  http://h/desc.xml \n") == "http://h/desc.xml");
    CHECK(trimWhitespace("\tname\r\n") == "name");
    CHECK(trimWhitespace("no-op") == "no-op");
    CHECK(trimWhitespace("   ").empty());
    CHECK(trimWhitespace("").empty());
    CHECK(trimWhitespace("inner space kept") == "inner space kept");
}

TEST_CASE("parse trims whitespace inside stored values", "[serverlist]") {
    // Self-heal entries that were saved with stray whitespace before
    // input sanitizing existed (the real-server 404 bug).
    const auto parsed =
        parseServerList(R"([{"name":" NAS ","url":"http://h/desc.xml\n"}])");
    REQUIRE(parsed.size() == 1);
    CHECK(parsed[0].name == "NAS");
    CHECK(parsed[0].url == "http://h/desc.xml");
}

TEST_CASE("missing fields default to empty strings", "[serverlist]") {
    const auto parsed = parseServerList(R"([{"url":"http://only-url/"}])");
    REQUIRE(parsed.size() == 1);
    CHECK(parsed[0].name.empty());
    CHECK(parsed[0].url == "http://only-url/");
}
