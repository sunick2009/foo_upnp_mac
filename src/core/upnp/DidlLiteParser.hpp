#pragma once

#include <string>
#include <vector>

#include "upnp/UpnpObject.hpp"

namespace upnp {

// Parses a DIDL-Lite document into UpnpObjects (ADR-001/002).
//
// An empty input yields an empty vector (a Browse of an empty
// container legitimately returns an empty Result). Malformed XML
// throws XmlParseException.
class DidlLiteParser {
public:
    static std::vector<UpnpObject> parse(const std::string& didlXml);
};

} // namespace upnp
