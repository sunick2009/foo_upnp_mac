#include "upnp/DidlLiteParser.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

#include <pugixml.hpp>

#include "upnp/UpnpError.hpp"

namespace upnp {

namespace {

// DIDL-Lite metadata elements carry namespace prefixes (dc:, upnp:)
// that servers could in principle rename, so match on local name.
const char* localName(const pugi::xml_node& node) {
    const char* name = node.name();
    const char* colon = std::strchr(name, ':');
    return colon ? colon + 1 : name;
}

pugi::xml_node childByLocalName(const pugi::xml_node& parent, const char* wanted) {
    for (auto& child : parent.children()) {
        if (std::strcmp(localName(child), wanted) == 0) return child;
    }
    return {};
}

std::optional<std::string> optionalText(const pugi::xml_node& parent, const char* name) {
    auto node = childByLocalName(parent, name);
    if (!node || !*node.child_value()) return std::nullopt;
    return std::string(node.child_value());
}

std::optional<std::string> optionalAttribute(const pugi::xml_node& node,
                                             const char* name) {
    auto attr = node.attribute(name);
    if (!attr || !*attr.value()) return std::nullopt;
    return std::string(attr.value());
}

std::string lowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool isAlbumArtistRole(const std::optional<std::string>& role) {
    if (!role) return false;
    const std::string lowered = lowerAscii(*role);
    return lowered == "albumartist" || lowered == "album artist" ||
           lowered == "album_artist";
}

UpnpObjectType typeFromUpnpClass(const std::string& upnpClass) {
    if (upnpClass.rfind("object.container", 0) == 0) return UpnpObjectType::Container;
    if (upnpClass.rfind("object.item.audioItem", 0) == 0) return UpnpObjectType::AudioItem;
    return UpnpObjectType::Unknown;
}

// "http-get:*:audio/flac:..." -> "audio/flac"; empty when malformed (ADR-002).
std::string mimeTypeFromProtocolInfo(const std::string& protocolInfo) {
    size_t first = protocolInfo.find(':');
    if (first == std::string::npos) return "";
    size_t second = protocolInfo.find(':', first + 1);
    if (second == std::string::npos) return "";
    size_t third = protocolInfo.find(':', second + 1);
    if (third == std::string::npos) return "";
    return protocolInfo.substr(second + 1, third - second - 1);
}

UpnpResource parseResource(const pugi::xml_node& res) {
    UpnpResource resource;
    resource.url = res.child_value();
    resource.protocolInfo = res.attribute("protocolInfo").value();
    resource.mimeType = mimeTypeFromProtocolInfo(resource.protocolInfo);
    resource.duration = res.attribute("duration").value();
    if (auto attr = res.attribute("size")) {
        resource.size = std::strtoull(attr.value(), nullptr, 10);
    }
    if (auto attr = res.attribute("bitrate")) {
        resource.bitrate = static_cast<uint32_t>(std::strtoul(attr.value(), nullptr, 10));
    }
    if (auto attr = res.attribute("bitsPerSample")) {
        resource.bitsPerSample =
            static_cast<uint32_t>(std::strtoul(attr.value(), nullptr, 10));
    }
    if (auto attr = res.attribute("sampleFrequency")) {
        resource.sampleFrequency =
            static_cast<uint32_t>(std::strtoul(attr.value(), nullptr, 10));
    }
    if (auto attr = res.attribute("nrAudioChannels")) {
        resource.nrAudioChannels =
            static_cast<uint32_t>(std::strtoul(attr.value(), nullptr, 10));
    }
    return resource;
}

UpnpObject parseObject(const pugi::xml_node& node, bool isContainer) {
    UpnpObject obj;
    obj.id = node.attribute("id").value();
    obj.parentId = node.attribute("parentID").value();
    obj.title = childByLocalName(node, "title").child_value();

    std::string upnpClass = childByLocalName(node, "class").child_value();
    obj.type = isContainer ? UpnpObjectType::Container : typeFromUpnpClass(upnpClass);

    if (isContainer) {
        if (auto attr = node.attribute("childCount")) {
            obj.childCount = static_cast<uint32_t>(std::strtoul(attr.value(), nullptr, 10));
        }
        return obj;
    }

    for (auto& child : node.children()) {
        if (std::strcmp(localName(child), "artist") != 0) continue;
        if (!*child.child_value()) continue;

        UpnpArtist artist;
        artist.name = child.child_value();
        artist.role = optionalAttribute(child, "role");
        obj.artists.push_back(artist);
        if (isAlbumArtistRole(artist.role) && !obj.albumArtist) {
            obj.albumArtist = artist.name;
        } else if (!obj.artist) {
            obj.artist = artist.name;
        }
    }
    obj.album = optionalText(node, "album");
    obj.genre = optionalText(node, "genre");
    obj.creator = optionalText(node, "creator");
    obj.date = optionalText(node, "date");
    obj.originalTrackNumber = optionalText(node, "originalTrackNumber");
    obj.discNumber = optionalText(node, "originalDiscNumber");
    if (!obj.discNumber) obj.discNumber = optionalText(node, "discNumber");
    obj.totalDiscs = optionalText(node, "totalDiscs");
    obj.longDescription = optionalText(node, "longDescription");
    obj.albumArtUri = optionalText(node, "albumArtURI");
    for (auto& child : node.children()) {
        if (std::strcmp(localName(child), "res") == 0) {
            obj.resources.push_back(parseResource(child));
        }
    }
    return obj;
}

} // namespace

std::vector<UpnpObject> DidlLiteParser::parse(const std::string& didlXml) {
    if (didlXml.empty()) return {};

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(didlXml.c_str());
    if (!result) {
        throw XmlParseException(
            std::string("DIDL-Lite is not valid XML: ") + result.description());
    }

    pugi::xml_node root = doc.document_element();
    if (std::strcmp(localName(root), "DIDL-Lite") != 0) {
        throw XmlParseException(
            std::string("expected DIDL-Lite root element, got: ") + root.name());
    }

    std::vector<UpnpObject> objects;
    for (auto& child : root.children()) {
        const char* name = localName(child);
        if (std::strcmp(name, "container") == 0) {
            objects.push_back(parseObject(child, /*isContainer=*/true));
        } else if (std::strcmp(name, "item") == 0) {
            objects.push_back(parseObject(child, /*isContainer=*/false));
        }
    }
    return objects;
}

} // namespace upnp
