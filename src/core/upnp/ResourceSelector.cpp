#include "upnp/ResourceSelector.hpp"

#include <limits>

namespace upnp {

const std::vector<std::string>& defaultMimeTypePriority() {
    static const std::vector<std::string> kPriority = {
        "audio/flac",
        "audio/x-flac",
        "audio/wav",
        "audio/x-wav",
        "audio/aiff",
        "audio/mpeg",
        "audio/mp4",
        "audio/ogg",
        "audio/x-ms-wma",
    };
    return kPriority;
}

namespace {

bool isHttpGet(const UpnpResource& resource) {
    return resource.protocolInfo.rfind("http-get", 0) == 0;
}

size_t priorityRank(const std::string& mimeType,
                    const std::vector<std::string>& priority) {
    for (size_t i = 0; i < priority.size(); ++i) {
        if (priority[i] == mimeType) return i;
    }
    return std::numeric_limits<size_t>::max();
}

} // namespace

std::optional<UpnpResource> selectBestResource(
    const std::vector<UpnpResource>& resources,
    const std::vector<std::string>& mimeTypePriority) {
    const UpnpResource* best = nullptr;
    size_t bestRank = std::numeric_limits<size_t>::max();

    for (const auto& resource : resources) {
        if (!isHttpGet(resource)) continue;
        size_t rank = priorityRank(resource.mimeType, mimeTypePriority);
        if (!best || rank < bestRank) {
            best = &resource;
            bestRank = rank;
        }
    }

    if (!best) return std::nullopt;
    return *best;
}

} // namespace upnp
