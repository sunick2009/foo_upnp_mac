#pragma once

#include <map>
#include <string>

namespace upnp {

// Blocking HTTP client over libcurl (ADR-008/009).
//
// Transport failures (timeout, refused, DNS) throw HttpException with
// statusCode -1. HTTP 4xx/5xx are returned as normal responses: SOAP
// faults arrive as HTTP 500 and the caller decides what an error is.
//
// Methods are virtual so tests can substitute a fake transport.
class HttpClient {
public:
    struct Response {
        int statusCode = 0;
        std::string body;
        std::map<std::string, std::string> headers;
    };

    struct Options {
        long timeoutSeconds = 10;
        std::string userAgent = "foo_dms_browser_mac/0.1";
    };

    HttpClient();
    explicit HttpClient(Options options);
    virtual ~HttpClient() = default;

    virtual Response get(const std::string& url);

    virtual Response post(const std::string& url,
                          const std::string& body,
                          const std::string& contentType,
                          const std::map<std::string, std::string>& extraHeaders = {});

private:
    Options options_;
};

} // namespace upnp
