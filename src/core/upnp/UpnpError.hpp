#pragma once

#include <stdexcept>
#include <string>

namespace upnp {

// Base for all expected failures in the upnp core (ADR-005).
class UpnpException : public std::runtime_error {
public:
    explicit UpnpException(const std::string& msg) : std::runtime_error(msg) {}
};

// Network-level failure: timeout, connection refused, DNS, TLS.
// statusCode is -1 for transport errors; HTTP 4xx/5xx are returned
// as normal responses by HttpClient, not thrown (ADR-008).
class HttpException : public UpnpException {
public:
    HttpException(const std::string& msg, int statusCode = -1)
        : UpnpException(msg), statusCode(statusCode) {}

    int statusCode;
};

// XML could not be parsed or a required node is missing.
class XmlParseException : public UpnpException {
public:
    explicit XmlParseException(const std::string& msg) : UpnpException(msg) {}
};

// SOAP response contained <s:Fault> (HTTP 200 with a UPnP error payload).
class SoapFaultException : public UpnpException {
public:
    SoapFaultException(int faultCode, const std::string& faultString)
        : UpnpException("SOAP fault " + std::to_string(faultCode) + ": " + faultString),
          faultCode(faultCode),
          faultString(faultString) {}

    int faultCode;
    std::string faultString;
};

// Device description has no ContentDirectory service.
class ServiceNotFoundException : public UpnpException {
public:
    explicit ServiceNotFoundException(const std::string& msg) : UpnpException(msg) {}
};

// Item has no playable (http-get) resource.
class NoResourceException : public UpnpException {
public:
    explicit NoResourceException(const std::string& msg) : UpnpException(msg) {}
};

} // namespace upnp
