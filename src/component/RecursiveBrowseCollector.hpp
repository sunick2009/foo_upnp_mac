#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "component/BrowsePager.hpp"

namespace component {

struct RecursiveBrowseOptions {
    size_t maxTracks = 10000;
    size_t maxContainers = 10000;
};

struct RecursiveBrowseProgress {
    size_t containersVisited = 0;
    size_t tracksFound = 0;
};

struct RecursiveBrowseResult {
    std::vector<upnp::UpnpObject> objects;
    size_t containersVisited = 0;
    bool cancelled = false;
    bool truncated = false;
};

using FetchChildrenFn = std::function<PagedBrowseResult(const std::string&)>;
using CancellationFn = std::function<bool()>;
using ProgressFn = std::function<void(const RecursiveBrowseProgress&)>;

// Breadth-first recursive BrowseDirectChildren collector for M4 container add.
// Exceptions from fetchChildren propagate so the UI can surface them using the
// existing ADR-014 error path.
RecursiveBrowseResult collectRecursiveChildren(
    const FetchChildrenFn& fetchChildren,
    const std::string& rootObjectId,
    RecursiveBrowseOptions options = {},
    CancellationFn isCancelled = {},
    ProgressFn onProgress = {});

} // namespace component
