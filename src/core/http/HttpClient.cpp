#include "http/HttpClient.hpp"

#include <memory>
#include <mutex>

#include <curl/curl.h>

#include "upnp/UpnpError.hpp"

namespace upnp {

namespace {

void globalInitOnce() {
    static std::once_flag flag;
    std::call_once(flag, [] { curl_global_init(CURL_GLOBAL_ALL); });
}

size_t writeToString(char* data, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(data, size * nmemb);
    return size * nmemb;
}

size_t collectHeader(char* data, size_t size, size_t nmemb, void* userdata) {
    auto* headers = static_cast<std::map<std::string, std::string>*>(userdata);
    std::string line(data, size * nmemb);
    size_t colon = line.find(':');
    if (colon != std::string::npos) {
        std::string name = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        // trim leading space and trailing CRLF
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of("\r\n") + 1);
        (*headers)[name] = value;
    }
    return size * nmemb;
}

struct CurlHandle {
    CURL* curl;
    CurlHandle() : curl(curl_easy_init()) {
        if (!curl) throw HttpException("failed to initialize libcurl handle");
    }
    ~CurlHandle() { curl_easy_cleanup(curl); }
};

struct CurlHeaderList {
    curl_slist* list = nullptr;
    void append(const std::string& header) { list = curl_slist_append(list, header.c_str()); }
    ~CurlHeaderList() { curl_slist_free_all(list); }
};

HttpClient::Response perform(CURL* curl, const std::string& url,
                             const HttpClient::Options& options) {
    HttpClient::Response response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, options.timeoutSeconds);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, options.connectTimeoutSeconds);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, collectHeader);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response.headers);

    CURLcode code = curl_easy_perform(curl);
    if (code == CURLE_OPERATION_TIMEDOUT) {
        throw HttpException("request to " + url + " timed out after " +
                            std::to_string(options.timeoutSeconds) + "s");
    }
    if (code != CURLE_OK) {
        throw HttpException("request to " + url + " failed: " +
                            curl_easy_strerror(code));
    }

    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    response.statusCode = static_cast<int>(status);
    return response;
}

} // namespace

HttpClient::HttpClient() : HttpClient(Options{}) {}

HttpClient::HttpClient(Options options) : options_(std::move(options)) {
    globalInitOnce();
}

HttpClient::Response HttpClient::get(const std::string& url) {
    CurlHandle handle;
    curl_easy_setopt(handle.curl, CURLOPT_USERAGENT, options_.userAgent.c_str());
    return perform(handle.curl, url, options_);
}

HttpClient::Response HttpClient::post(const std::string& url,
                                      const std::string& body,
                                      const std::string& contentType,
                                      const std::map<std::string, std::string>& extraHeaders) {
    CurlHandle handle;
    curl_easy_setopt(handle.curl, CURLOPT_USERAGENT, options_.userAgent.c_str());
    curl_easy_setopt(handle.curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(handle.curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));

    CurlHeaderList headers;
    headers.append("Content-Type: " + contentType);
    for (const auto& [name, value] : extraHeaders) {
        headers.append(name + ": " + value);
    }
    curl_easy_setopt(handle.curl, CURLOPT_HTTPHEADER, headers.list);

    return perform(handle.curl, url, options_);
}

} // namespace upnp
