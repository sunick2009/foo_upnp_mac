#pragma once

#include <vector>

#include "upnp/UpnpObject.hpp"

namespace dms {

// Adds the playable items among `objects` to the active playlist,
// pre-filling DIDL metadata via browse-info hints (ADR-016). Must be
// called on the fb2k main thread. Returns the number of tracks added
// (objects without an http-get resource are skipped, ADR-007).
size_t addToActivePlaylist(const std::vector<upnp::UpnpObject>& objects);

} // namespace dms
