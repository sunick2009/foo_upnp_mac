#include <catch2/catch_test_macros.hpp>

#include <map>
#include <string>
#include <vector>

#include "component/RecursiveBrowseCollector.hpp"

using component::PagedBrowseResult;
using component::RecursiveBrowseOptions;
using component::RecursiveBrowseProgress;
using component::collectRecursiveChildren;
using upnp::UpnpObject;
using upnp::UpnpObjectType;

namespace {

UpnpObject makeContainer(const std::string& id) {
    UpnpObject object;
    object.type = UpnpObjectType::Container;
    object.id = id;
    object.title = id;
    return object;
}

UpnpObject makeItem(const std::string& id) {
    UpnpObject object;
    object.type = UpnpObjectType::AudioItem;
    object.id = id;
    object.title = id;
    return object;
}

} // namespace

TEST_CASE("recursive collector walks containers breadth first", "[recursive]") {
    const std::map<std::string, std::vector<UpnpObject>> tree = {
        {"0", {makeContainer("a"), makeItem("root-track"), makeContainer("b")}},
        {"a", {makeItem("a-track")}},
        {"b", {makeContainer("b-child")}},
        {"b-child", {makeItem("b-child-track")}},
    };
    std::vector<std::string> visited;

    const auto result = collectRecursiveChildren(
        [&](const std::string& objectId) {
            visited.push_back(objectId);
            PagedBrowseResult page;
            page.objects = tree.at(objectId);
            return page;
        },
        "0");

    REQUIRE(result.objects.size() == 3);
    CHECK(result.objects[0].id == "root-track");
    CHECK(result.objects[1].id == "a-track");
    CHECK(result.objects[2].id == "b-child-track");
    CHECK(visited == std::vector<std::string>{"0", "a", "b", "b-child"});
    CHECK(result.containersVisited == 4);
    CHECK_FALSE(result.cancelled);
    CHECK_FALSE(result.truncated);
}

TEST_CASE("recursive collector reports progress after each container",
          "[recursive]") {
    const std::map<std::string, std::vector<UpnpObject>> tree = {
        {"0", {makeContainer("a"), makeItem("root-track")}},
        {"a", {makeItem("a-track")}},
    };
    std::vector<RecursiveBrowseProgress> progress;

    const auto result = collectRecursiveChildren(
        [&](const std::string& objectId) {
            PagedBrowseResult page;
            page.objects = tree.at(objectId);
            return page;
        },
        "0", {}, {}, [&](RecursiveBrowseProgress value) {
            progress.push_back(value);
        });

    CHECK(result.objects.size() == 2);
    REQUIRE(progress.size() == 2);
    CHECK(progress[0].containersVisited == 1);
    CHECK(progress[0].tracksFound == 1);
    CHECK(progress[1].containersVisited == 2);
    CHECK(progress[1].tracksFound == 2);
}

TEST_CASE("recursive collector cancels at item boundaries", "[recursive]") {
    const std::map<std::string, std::vector<UpnpObject>> tree = {
        {"0", {makeItem("one"), makeItem("two"), makeItem("three")}},
    };
    int cancelChecks = 0;

    const auto result = collectRecursiveChildren(
        [&](const std::string& objectId) {
            PagedBrowseResult page;
            page.objects = tree.at(objectId);
            return page;
        },
        "0", {}, [&] {
            ++cancelChecks;
            return cancelChecks >= 3;
        });

    CHECK(result.cancelled);
    CHECK(result.objects.size() == 1);
    CHECK_FALSE(result.truncated);
}

TEST_CASE("recursive collector truncates at track cap", "[recursive]") {
    const std::map<std::string, std::vector<UpnpObject>> tree = {
        {"0", {makeItem("one"), makeItem("two"), makeItem("three")}},
    };

    const auto result = collectRecursiveChildren(
        [&](const std::string& objectId) {
            PagedBrowseResult page;
            page.objects = tree.at(objectId);
            return page;
        },
        "0", RecursiveBrowseOptions{.maxTracks = 2, .maxContainers = 10});

    REQUIRE(result.objects.size() == 2);
    CHECK(result.objects[0].id == "one");
    CHECK(result.objects[1].id == "two");
    CHECK(result.truncated);
    CHECK_FALSE(result.cancelled);
}

TEST_CASE("recursive collector truncates at container cap", "[recursive]") {
    const std::map<std::string, std::vector<UpnpObject>> tree = {
        {"0", {makeContainer("a"), makeContainer("b")}},
        {"a", {makeItem("a-track")}},
        {"b", {makeItem("b-track")}},
    };

    const auto result = collectRecursiveChildren(
        [&](const std::string& objectId) {
            PagedBrowseResult page;
            page.objects = tree.at(objectId);
            return page;
        },
        "0", RecursiveBrowseOptions{.maxTracks = 10, .maxContainers = 2});

    CHECK(result.containersVisited == 2);
    REQUIRE(result.objects.size() == 1);
    CHECK(result.objects[0].id == "a-track");
    CHECK(result.truncated);
}

TEST_CASE("recursive collector propagates fetch errors", "[recursive]") {
    CHECK_THROWS_AS(
        collectRecursiveChildren(
            [](const std::string&) -> PagedBrowseResult {
                throw std::runtime_error("boom");
            },
            "0"),
        std::runtime_error);
}

TEST_CASE("recursive collector honours a fetch cancelled mid-pagination",
          "[recursive]") {
    // The cancel can land inside the pager while the LAST pending
    // container is being fetched: no further loop iteration runs the
    // collector's own cancellation check, so the flag must come from
    // the page itself or the partial collection would get added.
    const auto fetch = [](const std::string& objectId) {
        PagedBrowseResult page;
        if (objectId == "root") {
            page.objects = {makeItem("kept"), makeContainer("last")};
        } else {
            // Cancelled while paging through the final container.
            page.objects = {makeItem("partial")};
            page.cancelled = true;
        }
        return page;
    };
    const auto result = collectRecursiveChildren(fetch, "root");
    CHECK(result.cancelled);
}
