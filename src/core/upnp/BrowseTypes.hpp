#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "upnp/UpnpObject.hpp"

namespace upnp {

// UPnP Browse action's BrowseFlag argument (ADR-003 naming).
enum class BrowseFlag {
    DirectChildren, // "BrowseDirectChildren"
    Metadata,       // "BrowseMetadata"
};

// In arguments of the Browse action (ADR-012).
// Defaults browse the root container's first page.
struct BrowseParams {
    std::string objectId = "0";
    BrowseFlag browseFlag = BrowseFlag::DirectChildren;
    std::string filter = "*";
    uint32_t startingIndex = 0;
    uint32_t requestedCount = 100; // 0 = let the server decide
    std::string sortCriteria;
};

// Intermediate result from SoapBrowseResponseParser (ADR-006):
// the DIDL-Lite payload still unparsed, plus the count fields.
struct BrowseResult {
    std::string resultXml; // unescaped DIDL-Lite XML
    uint32_t numberReturned = 0;
    uint32_t totalMatches = 0;
    uint32_t updateId = 0;
};

// Fully parsed Browse response (ADR-012).
struct BrowseResponse {
    std::vector<UpnpObject> objects;
    uint32_t numberReturned = 0;
    uint32_t totalMatches = 0;
    uint32_t updateId = 0;
};

} // namespace upnp
