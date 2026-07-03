#pragma once

#include <string>

namespace upnp {

// Resolves service URLs (controlURL etc.) from a device description
// against the rootDesc.xml URL, honouring an optional <URLBase> (ADR-004).
//
// Resolution follows RFC 3986 semantics for the cases seen in the wild:
//   - absolute controlURL          -> returned as-is
//   - root-relative ("/path")      -> scheme+host+port of base + path
//   - relative ("path")            -> base directory + path
class UrlResolver {
public:
    // rootDescUrl: the URL used to fetch rootDesc.xml. Must be absolute http(s).
    // urlBase: content of <URLBase>, or empty when absent.
    // Throws std::invalid_argument if the effective base is not an absolute URL.
    explicit UrlResolver(std::string rootDescUrl, std::string urlBase = "");

    // Throws std::invalid_argument if controlUrl is empty.
    std::string resolve(const std::string& controlUrl) const;

private:
    std::string schemeHostPort_; // "http://host:port"
    std::string baseDir_;        // "http://host:port/dir/" (always ends with '/')
};

} // namespace upnp
