#pragma once

#include <optional>
#include <string>
#include <vector>

#include "upnp/UpnpObject.hpp"

namespace upnp {

// Preferred audio formats, best first (ADR-007). Callers may pass
// their own list to selectBestResource.
const std::vector<std::string>& defaultMimeTypePriority();

// Picks the resource to hand to the player (ADR-007):
//   1. only http-get resources are considered (foobar2000 plays http)
//   2. best-ranked mimeType in the priority list wins
//   3. otherwise the first http-get resource
//   4. nullopt when nothing is playable
std::optional<UpnpResource> selectBestResource(
    const std::vector<UpnpResource>& resources,
    const std::vector<std::string>& mimeTypePriority = defaultMimeTypePriority());

} // namespace upnp
