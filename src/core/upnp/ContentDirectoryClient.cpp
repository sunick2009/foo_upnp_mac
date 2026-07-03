#include "upnp/ContentDirectoryClient.hpp"

#include "upnp/DidlLiteParser.hpp"
#include "upnp/SoapBrowseRequestBuilder.hpp"
#include "upnp/SoapBrowseResponseParser.hpp"
#include "upnp/UpnpError.hpp"
#include "upnp/UrlResolver.hpp"

namespace upnp {

ContentDirectoryClient::ContentDirectoryClient(HttpClient& http,
                                               DeviceDescription description,
                                               std::string controlUrl)
    : http_(http),
      description_(std::move(description)),
      controlUrl_(std::move(controlUrl)) {}

ContentDirectoryClient ContentDirectoryClient::connect(HttpClient& http,
                                                       const std::string& rootDescUrl) {
    auto response = http.get(rootDescUrl);
    if (response.statusCode != 200) {
        throw HttpException("fetching device description " + rootDescUrl +
                                " returned HTTP " + std::to_string(response.statusCode),
                            response.statusCode);
    }

    DeviceDescription description = DeviceDescriptionParser::parse(response.body);
    std::string controlUrl =
        UrlResolver(rootDescUrl, description.urlBase).resolve(description.controlUrl);
    return ContentDirectoryClient(http, std::move(description), std::move(controlUrl));
}

BrowseResponse ContentDirectoryClient::browse(const BrowseParams& params) {
    SoapRequest request = SoapBrowseRequestBuilder::build(description_.serviceType, params);

    auto httpResponse = http_.post(controlUrl_, request.body, request.contentType,
                                   {{"SOAPAction", request.soapAction}});

    // SOAP faults arrive as HTTP 500 with a Fault body, so parse first:
    // SoapFaultException (with the upnp error code) beats a generic
    // http error. Only fall back to HttpException when the body is not
    // SOAP at all (e.g. an html error page).
    BrowseResult result;
    try {
        result = SoapBrowseResponseParser::parse(httpResponse.body);
    } catch (const XmlParseException&) {
        if (httpResponse.statusCode != 200) {
            throw HttpException("Browse request to " + controlUrl_ + " returned HTTP " +
                                    std::to_string(httpResponse.statusCode),
                                httpResponse.statusCode);
        }
        throw;
    }

    BrowseResponse response;
    response.objects = DidlLiteParser::parse(result.resultXml);
    response.numberReturned = result.numberReturned;
    response.totalMatches = result.totalMatches;
    response.updateId = result.updateId;
    return response;
}

} // namespace upnp
