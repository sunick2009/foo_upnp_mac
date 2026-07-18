#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "component/BrowsePager.hpp"

using component::fetchAllChildren;
using upnp::BrowseParams;
using upnp::BrowseResponse;
using upnp::UpnpObject;
using upnp::UpnpObjectType;

namespace {

UpnpObject makeItem(int index) {
    UpnpObject object;
    object.type = UpnpObjectType::AudioItem;
    object.id = "item-" + std::to_string(index);
    object.title = "Track " + std::to_string(index);
    return object;
}

// Fake server with `total` items, honouring startingIndex/requestedCount.
component::BrowseFn fakeServer(uint32_t total, uint32_t reportedTotal,
                               std::vector<BrowseParams>* calls = nullptr) {
    return [=](const BrowseParams& params) {
        if (calls) calls->push_back(params);
        BrowseResponse response;
        response.totalMatches = reportedTotal;
        for (uint32_t i = params.startingIndex;
             i < total && i < params.startingIndex + params.requestedCount; ++i) {
            response.objects.push_back(makeItem(static_cast<int>(i)));
        }
        response.numberReturned = static_cast<uint32_t>(response.objects.size());
        return response;
    };
}

} // namespace

TEST_CASE("single page fits entirely", "[pager]") {
    const auto result = fetchAllChildren(fakeServer(5, 5), "0");
    CHECK(result.objects.size() == 5);
    CHECK(result.totalMatches == 5);
    CHECK_FALSE(result.truncated);
}

TEST_CASE("multiple pages are concatenated in order", "[pager]") {
    std::vector<BrowseParams> calls;
    const auto result = fetchAllChildren(fakeServer(250, 250, &calls), "0", 100);
    REQUIRE(result.objects.size() == 250);
    CHECK(result.objects.front().id == "item-0");
    CHECK(result.objects.back().id == "item-249");
    REQUIRE(calls.size() == 3);
    CHECK(calls[0].startingIndex == 0);
    CHECK(calls[1].startingIndex == 100);
    CHECK(calls[2].startingIndex == 200);
    CHECK_FALSE(result.truncated);
}

TEST_CASE("empty container returns no objects", "[pager]") {
    const auto result = fetchAllChildren(fakeServer(0, 0), "0");
    CHECK(result.objects.empty());
    CHECK_FALSE(result.truncated);
}

TEST_CASE("cap truncates and flags oversized containers", "[pager]") {
    const auto result = fetchAllChildren(fakeServer(500, 500), "0", 100, 250);
    CHECK(result.objects.size() == 250);
    CHECK(result.truncated);
}

TEST_CASE("cap equal to total is not truncation", "[pager]") {
    const auto result = fetchAllChildren(fakeServer(200, 200), "0", 100, 200);
    CHECK(result.objects.size() == 200);
    CHECK_FALSE(result.truncated);
}

TEST_CASE("unknown total ends on a short page", "[pager]") {
    // Server reports totalMatches=0 (unknown) but has 150 items.
    std::vector<BrowseParams> calls;
    const auto result = fetchAllChildren(fakeServer(150, 0, &calls), "0", 100);
    CHECK(result.objects.size() == 150);
    CHECK(calls.size() == 2);
    CHECK_FALSE(result.truncated);
}

TEST_CASE("server repeating the same page cannot loop forever", "[pager]") {
    // Broken server: ignores startingIndex, always returns the same full
    // page and claims a huge total. The cap must end the loop.
    int callCount = 0;
    auto broken = [&](const BrowseParams& params) {
        ++callCount;
        BrowseResponse response;
        response.totalMatches = 100000;
        for (uint32_t i = 0; i < params.requestedCount; ++i)
            response.objects.push_back(makeItem(static_cast<int>(i)));
        response.numberReturned = static_cast<uint32_t>(response.objects.size());
        return response;
    };
    const auto result = fetchAllChildren(broken, "0", 100, 1000);
    CHECK(result.objects.size() == 1000);
    CHECK(result.truncated);
    CHECK(callCount <= 11);
}

TEST_CASE("browse exceptions propagate to the caller", "[pager]") {
    auto throwing = [](const BrowseParams&) -> BrowseResponse {
        throw std::runtime_error("boom");
    };
    CHECK_THROWS_AS(fetchAllChildren(throwing, "0"), std::runtime_error);
}

TEST_CASE("cancellation stops pagination between pages", "[pager]") {
    std::vector<BrowseParams> calls;
    int fetches = 0;
    // Cancel after the first page has been fetched: the second poll
    // (before page two) must stop the loop.
    const auto result = fetchAllChildren(
        fakeServer(250, 250, &calls), "0", 100, 10000,
        [&] { return fetches++ >= 1; });
    CHECK(result.cancelled);
    REQUIRE(calls.size() == 1);
    CHECK(result.objects.size() == 100); // partial pages are kept...
    // ...and callers are expected to discard them when cancelled.
}

TEST_CASE("cancellation before the first page fetches nothing", "[pager]") {
    std::vector<BrowseParams> calls;
    const auto result = fetchAllChildren(
        fakeServer(50, 50, &calls), "0", 100, 10000, [] { return true; });
    CHECK(result.cancelled);
    CHECK(calls.empty());
    CHECK(result.objects.empty());
}

TEST_CASE("no cancellation callback behaves as before", "[pager]") {
    const auto result = fetchAllChildren(fakeServer(5, 5), "0", 100, 10000);
    CHECK_FALSE(result.cancelled);
    CHECK(result.objects.size() == 5);
}
