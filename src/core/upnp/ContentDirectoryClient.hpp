#pragma once

#include <string>

#include "http/HttpClient.hpp"
#include "upnp/BrowseTypes.hpp"
#include "upnp/DeviceDescriptionParser.hpp"

namespace upnp {

// Orchestrates a ContentDirectory session (ADR-006): fetches the device
// description, resolves the control URL, and runs Browse calls through
// the SOAP builder/parsers. Does no XML work itself.
//
// Blocking, like everything in Phase 0 (ADR-008). The HttpClient must
// outlive the client.
class ContentDirectoryClient {
public:
    // Fetches and parses rootDesc.xml. Throws HttpException on network
    // or non-200 failures, XmlParseException / ServiceNotFoundException
    // when the description is unusable.
    static ContentDirectoryClient connect(HttpClient& http, const std::string& rootDescUrl);

    // One Browse action call; no automatic pagination (ADR-012).
    BrowseResponse browse(const BrowseParams& params = {});

    const DeviceDescription& description() const { return description_; }
    const std::string& controlUrl() const { return controlUrl_; }

private:
    ContentDirectoryClient(HttpClient& http, DeviceDescription description,
                           std::string controlUrl);

    HttpClient& http_;
    DeviceDescription description_;
    std::string controlUrl_;
};

} // namespace upnp
