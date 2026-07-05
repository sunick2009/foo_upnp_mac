#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "upnp/UpnpObject.hpp"

namespace component {

// Custom metadb field carrying the DIDL albumArtURI so the album art
// fallback service can fetch cover art for remote tracks (ADR-016).
inline constexpr const char* kAlbumArtUriField = "UPNP_ALBUM_ART_URI";

// Metadata to hint into fb2k's metadb for one added track (ADR-016).
// meta pairs use fb2k field names ("title", "artist", ...).
struct HintData {
    std::vector<std::pair<std::string, std::string>> meta;
    std::vector<std::pair<std::string, std::string>> info;
    std::optional<double> lengthSeconds;
};

// Maps DIDL data to hint fields. `resourceDuration` is the duration
// string of the resource actually being added ("H:MM:SS[.mmm]").
HintData hintFieldsFor(const upnp::UpnpObject& object,
                       const std::string& resourceDuration);
HintData hintFieldsFor(const upnp::UpnpObject& object,
                       const upnp::UpnpResource& resource);

// Parses a DIDL duration ("1:02:03", "02:03", "0:00:30.500") into
// seconds; nullopt when empty or malformed.
std::optional<double> parseDidlDuration(const std::string& duration);

} // namespace component
