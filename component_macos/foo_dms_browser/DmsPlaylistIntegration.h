#pragma once

#include <vector>

#include "upnp/UpnpObject.hpp"

namespace dms {

struct AddToPlaylistResult {
    size_t added = 0;
    size_t skipped = 0;
};

// Adds the playable items among `objects` to the active playlist,
// pre-filling DIDL metadata via browse-info hints (ADR-016). Must be
// called on the fb2k main thread. Objects without an http-get resource
// are skipped (ADR-007).
AddToPlaylistResult addToActivePlaylist(
    const std::vector<upnp::UpnpObject>& objects);

} // namespace dms
