#include "upnp/DeviceDescriptionParser.hpp"

#include <pugixml.hpp>

#include "upnp/UpnpError.hpp"

namespace upnp {

namespace {

constexpr const char* kContentDirectoryPrefix =
    "urn:schemas-upnp-org:service:ContentDirectory:";

bool isContentDirectory(const std::string& serviceType) {
    return serviceType.rfind(kContentDirectoryPrefix, 0) == 0;
}

} // namespace

DeviceDescription DeviceDescriptionParser::parse(const std::string& xml) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml.c_str());
    if (!result) {
        throw XmlParseException(
            std::string("device description is not valid XML: ") + result.description());
    }

    DeviceDescription desc;
    desc.urlBase = doc.child("root").child("URLBase").child_value();
    desc.friendlyName = doc.child("root").child("device").child("friendlyName").child_value();

    // "//service" also finds services of embedded devices (deviceList).
    for (auto& node : doc.select_nodes("//service")) {
        std::string serviceType = node.node().child("serviceType").child_value();
        if (!isContentDirectory(serviceType)) continue;

        desc.serviceType = serviceType;
        desc.controlUrl = node.node().child("controlURL").child_value();
        desc.eventSubUrl = node.node().child("eventSubURL").child_value();
        desc.scpdUrl = node.node().child("SCPDURL").child_value();

        if (desc.controlUrl.empty()) {
            throw XmlParseException(
                "ContentDirectory service found but its controlURL is empty");
        }
        return desc;
    }

    throw ServiceNotFoundException(
        "no ContentDirectory service in device description" +
        (desc.friendlyName.empty() ? "" : " of \"" + desc.friendlyName + "\""));
}

} // namespace upnp
