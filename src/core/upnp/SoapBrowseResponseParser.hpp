#pragma once

#include <string>

#include "upnp/BrowseTypes.hpp"

namespace upnp {

// Parses the SOAP layer of a Browse response (ADR-006): extracts the
// still-unparsed DIDL-Lite payload and the count fields. DIDL parsing
// is DidlLiteParser's job; orchestration is ContentDirectoryClient's.
//
// Throws SoapFaultException when the body contains a Fault,
// XmlParseException when the document is malformed or not a
// BrowseResponse.
class SoapBrowseResponseParser {
public:
    static BrowseResult parse(const std::string& soapXml);
};

} // namespace upnp
