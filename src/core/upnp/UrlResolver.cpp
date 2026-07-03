#include "upnp/UrlResolver.hpp"

#include <stdexcept>

namespace upnp {

namespace {

bool isAbsoluteHttpUrl(const std::string& url) {
    auto startsWithIgnoreCase = [&](const char* prefix) {
        size_t len = std::char_traits<char>::length(prefix);
        if (url.size() < len) return false;
        for (size_t i = 0; i < len; ++i) {
            if (std::tolower(static_cast<unsigned char>(url[i])) != prefix[i]) return false;
        }
        return true;
    };
    return startsWithIgnoreCase("http://") || startsWithIgnoreCase("https://");
}

// "http://host:port/a/b" -> "http://host:port"
std::string schemeHostPortOf(const std::string& url) {
    size_t schemeEnd = url.find("://");
    size_t pathStart = url.find('/', schemeEnd + 3);
    if (pathStart == std::string::npos) return url;
    return url.substr(0, pathStart);
}

// "http://host:port/a/b.xml" -> "http://host:port/a/"
std::string baseDirOf(const std::string& url) {
    size_t schemeEnd = url.find("://");
    size_t pathStart = url.find('/', schemeEnd + 3);
    if (pathStart == std::string::npos) return url + "/";
    size_t lastSlash = url.rfind('/');
    return url.substr(0, lastSlash + 1);
}

} // namespace

UrlResolver::UrlResolver(std::string rootDescUrl, std::string urlBase) {
    const std::string& base = urlBase.empty() ? rootDescUrl : urlBase;
    if (!isAbsoluteHttpUrl(base)) {
        throw std::invalid_argument("base URL is not an absolute http(s) URL: " + base);
    }
    schemeHostPort_ = schemeHostPortOf(base);
    baseDir_ = baseDirOf(base);
}

std::string UrlResolver::resolve(const std::string& controlUrl) const {
    if (controlUrl.empty()) {
        throw std::invalid_argument("controlURL is empty");
    }
    if (isAbsoluteHttpUrl(controlUrl)) {
        return controlUrl;
    }
    if (controlUrl[0] == '/') {
        return schemeHostPort_ + controlUrl;
    }
    return baseDir_ + controlUrl;
}

} // namespace upnp
