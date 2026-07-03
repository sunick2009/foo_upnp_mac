#include "upnp/SoapBrowseResponseParser.hpp"

#include <cstdlib>

#include <pugixml.hpp>

#include "upnp/UpnpError.hpp"

namespace upnp {

namespace {

// SOAP namespace prefixes vary between servers ("s", "SOAP-ENV", ...),
// so all lookups match on local name only.
pugi::xml_node findByLocalName(const pugi::xml_node& scope, const char* localName) {
    std::string expr = std::string(".//*[local-name()='") + localName + "']";
    return scope.select_node(expr.c_str()).node();
}

uint32_t toUint32(const char* text) {
    if (!text || !*text) return 0;
    return static_cast<uint32_t>(std::strtoul(text, nullptr, 10));
}

} // namespace

BrowseResult SoapBrowseResponseParser::parse(const std::string& soapXml) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(soapXml.c_str());
    if (!result) {
        throw XmlParseException(
            std::string("SOAP response is not valid XML: ") + result.description());
    }

    if (auto fault = findByLocalName(doc, "Fault")) {
        auto errorCode = findByLocalName(fault, "errorCode");
        auto errorDesc = findByLocalName(fault, "errorDescription");
        std::string description = errorDesc ? errorDesc.child_value() : "";
        if (description.empty()) {
            auto faultString = findByLocalName(fault, "faultstring");
            description = faultString ? faultString.child_value() : "unknown SOAP fault";
        }
        throw SoapFaultException(toUint32(errorCode.child_value()), description);
    }

    auto response = findByLocalName(doc, "BrowseResponse");
    if (!response) {
        throw XmlParseException("SOAP body contains neither BrowseResponse nor Fault");
    }

    BrowseResult browse;
    // <Result> holds XML-escaped DIDL-Lite; pugixml already unescaped it
    // when reading the text content (ADR-006).
    browse.resultXml = findByLocalName(response, "Result").child_value();
    browse.numberReturned = toUint32(findByLocalName(response, "NumberReturned").child_value());
    browse.totalMatches = toUint32(findByLocalName(response, "TotalMatches").child_value());
    browse.updateId = toUint32(findByLocalName(response, "UpdateID").child_value());
    return browse;
}

} // namespace upnp
