#pragma once

#include <string>

namespace upnp {

// The parts of a device description we need to talk to ContentDirectory.
// URLs are as found in the document (possibly relative); resolve them
// with UrlResolver before use.
struct DeviceDescription {
    std::string friendlyName;
    std::string urlBase;      // empty when <URLBase> is absent
    std::string serviceType;  // e.g. "urn:schemas-upnp-org:service:ContentDirectory:1"
    std::string controlUrl;
    std::string eventSubUrl;
    std::string scpdUrl;
};

// Parses a UPnP device description (rootDesc.xml) and locates the
// ContentDirectory service, searching embedded devices too.
//
// Throws XmlParseException on malformed XML,
// ServiceNotFoundException when no ContentDirectory service exists.
class DeviceDescriptionParser {
public:
    static DeviceDescription parse(const std::string& xml);
};

} // namespace upnp
