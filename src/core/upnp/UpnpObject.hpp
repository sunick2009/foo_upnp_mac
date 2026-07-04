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
    std::optional<uint32_t> bitsPerSample;
    std::optional<uint32_t> sampleFrequency;
    std::optional<uint32_t> nrAudioChannels;
};

struct UpnpArtist {
    std::string name;
    std::optional<std::string> role;
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
    std::vector<UpnpArtist> artists;
    std::optional<std::string> albumArtist;
    std::optional<std::string> album;
    std::optional<std::string> genre;
    std::optional<std::string> creator;
    std::optional<std::string> date;
    std::optional<std::string> originalTrackNumber;
    std::optional<std::string> discNumber;
    std::optional<std::string> totalDiscs;
    std::optional<std::string> longDescription;
    std::optional<std::string> albumArtUri;
    std::vector<UpnpResource> resources;
};

} // namespace upnp
