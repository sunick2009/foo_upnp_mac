#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "upnp/BrowseTypes.hpp"

namespace component {

// Result of fetching all pages of a container (ADR-012 revision).
struct PagedBrowseResult {
    std::vector<upnp::UpnpObject> objects;
    uint32_t totalMatches = 0;
    bool truncated = false; // hit maxItems before the server ran out
};

using BrowseFn = std::function<upnp::BrowseResponse(const upnp::BrowseParams&)>;

// Loops BrowseDirectChildren until the server is exhausted or maxItems
// is reached. Exceptions from `browse` propagate to the caller (the
// component surfaces them per ADR-014). Guards against servers that
// report totalMatches=0 (unknown) or return short/empty pages.
PagedBrowseResult fetchAllChildren(const BrowseFn& browse,
                                   const std::string& objectId,
                                   uint32_t pageSize = 100,
                                   uint32_t maxItems = 10000);

} // namespace component
