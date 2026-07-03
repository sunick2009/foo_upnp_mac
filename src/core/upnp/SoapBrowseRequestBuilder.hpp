#pragma once

#include <string>

#include "upnp/BrowseTypes.hpp"

namespace upnp {

// A ready-to-send SOAP request: POST body plus the headers UPnP requires.
struct SoapRequest {
    std::string body;
    std::string soapAction;  // value for the SOAPAction header, including quotes
    std::string contentType; // value for the Content-Type header
};

// Builds the SOAP envelope for a ContentDirectory Browse call.
// serviceType is taken from the device description so the action
// namespace and SOAPAction header match the service version.
class SoapBrowseRequestBuilder {
public:
    static SoapRequest build(const std::string& serviceType, const BrowseParams& params);
};

} // namespace upnp
