#include "component/RecursiveBrowseCollector.hpp"

#include <deque>

namespace component {

namespace {

bool cancelled(const CancellationFn& isCancelled) {
    return isCancelled && isCancelled();
}

} // namespace

RecursiveBrowseResult collectRecursiveChildren(
    const FetchChildrenFn& fetchChildren,
    const std::string& rootObjectId,
    RecursiveBrowseOptions options,
    CancellationFn isCancelled,
    ProgressFn onProgress) {
    if (options.maxTracks == 0 || options.maxContainers == 0) {
        RecursiveBrowseResult result;
        result.truncated = true;
        result.cancelled = cancelled(isCancelled);
        return result;
    }

    RecursiveBrowseResult result;
    std::deque<std::string> pending;
    pending.push_back(rootObjectId);

    while (!pending.empty()) {
        if (cancelled(isCancelled)) {
            result.cancelled = true;
            break;
        }
        if (result.containersVisited >= options.maxContainers) {
            result.truncated = true;
            break;
        }

        const std::string objectId = pending.front();
        pending.pop_front();
        ++result.containersVisited;

        PagedBrowseResult page = fetchChildren(objectId);
        if (page.truncated) result.truncated = true;

        for (const auto& object : page.objects) {
            if (cancelled(isCancelled)) {
                result.cancelled = true;
                break;
            }
            if (object.type == upnp::UpnpObjectType::Container) {
                pending.push_back(object.id);
            } else if (object.type == upnp::UpnpObjectType::AudioItem) {
                if (result.objects.size() >= options.maxTracks) {
                    result.truncated = true;
                    break;
                }
                result.objects.push_back(object);
            }
        }

        if (onProgress) {
            onProgress({result.containersVisited, result.objects.size()});
        }
        if (result.cancelled) break;
        if (result.truncated &&
            (result.objects.size() >= options.maxTracks ||
             result.containersVisited >= options.maxContainers)) {
            break;
        }
    }

    return result;
}

} // namespace component
