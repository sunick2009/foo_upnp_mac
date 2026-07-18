#include "component/BrowsePager.hpp"

#include <algorithm>

namespace component {

PagedBrowseResult fetchAllChildren(const BrowseFn& browse,
                                   const std::string& objectId,
                                   uint32_t pageSize,
                                   uint32_t maxItems,
                                   CancellationFn isCancelled) {
    PagedBrowseResult result;
    if (pageSize == 0) pageSize = 100;

    uint32_t index = 0;
    while (true) {
        if (isCancelled && isCancelled()) {
            result.cancelled = true;
            break;
        }
        upnp::BrowseParams params;
        params.objectId = objectId;
        params.browseFlag = upnp::BrowseFlag::DirectChildren;
        params.startingIndex = index;
        params.requestedCount = std::min(pageSize, maxItems - index);

        const upnp::BrowseResponse page = browse(params);
        result.totalMatches = page.totalMatches;

        for (const auto& object : page.objects) {
            if (result.objects.size() >= maxItems) break;
            result.objects.push_back(object);
        }
        index = static_cast<uint32_t>(result.objects.size());

        // Server has nothing more to give.
        if (page.objects.empty()) break;
        // Known total reached.
        if (page.totalMatches > 0 && index >= page.totalMatches) break;
        // Unknown total (totalMatches=0): a short page means the end.
        if (page.totalMatches == 0 && page.objects.size() < params.requestedCount) break;
        // Cap reached.
        if (index >= maxItems) {
            result.truncated =
                page.totalMatches == 0 || page.totalMatches > maxItems;
            break;
        }
    }
    return result;
}

} // namespace component
