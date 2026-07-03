#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace upnp {

enum class UpnpObjectType {
    Container,
    AudioItem,
    Unknown,
};

// One <res> element: a playable representation of an item (ADR-002).
// mimeType is the third field of protocolInfo, parsed by DidlLiteParser;
// empty string when protocolInfo is absent or malformed.
struct UpnpResource {
    std::string url;
    std::string protocolInfo;
    std::string mimeType;
    std::string duration;
    std::optional<uint64_t> size;
    std::optional<uint32_t> bitrate;
};

// One DIDL-Lite <container> or <item> (ADR-001).
// Field validity depends on `type`: childCount is container-only,
// artist/album/genre/albumArtUri/resources are item-only.
struct UpnpObject {
    UpnpObjectType type = UpnpObjectType::Unknown;
    std::string id;
    std::string parentId;
    std::string title;

    // Container fields (valid only when type == Container)
    std::optional<uint32_t> childCount;

    // Item fields (valid only when type == AudioItem or Unknown)
    std::optional<std::string> artist;
    std::optional<std::string> album;
    std::optional<std::string> genre;
    std::optional<std::string> albumArtUri;
    std::vector<UpnpResource> resources;
};

} // namespace upnp
