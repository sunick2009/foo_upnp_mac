#include "upnp/SoapBrowseRequestBuilder.hpp"

#include <sstream>

#include <pugixml.hpp>

namespace upnp {

namespace {

const char* browseFlagName(BrowseFlag flag) {
    switch (flag) {
        case BrowseFlag::DirectChildren: return "BrowseDirectChildren";
        case BrowseFlag::Metadata: return "BrowseMetadata";
    }
    return "BrowseDirectChildren";
}

} // namespace

SoapRequest SoapBrowseRequestBuilder::build(const std::string& serviceType,
                                            const BrowseParams& params) {
    // Build via pugixml so argument values (ObjectID, Filter, ...) are
    // XML-escaped by the serializer, not by hand.
    pugi::xml_document doc;

    auto envelope = doc.append_child("s:Envelope");
    envelope.append_attribute("xmlns:s") = "http://schemas.xmlsoap.org/soap/envelope/";
    envelope.append_attribute("s:encodingStyle") = "http://schemas.xmlsoap.org/soap/encoding/";

    auto body = envelope.append_child("s:Body");
    auto browse = body.append_child("u:Browse");
    browse.append_attribute("xmlns:u") = serviceType.c_str();

    auto addArg = [&](const char* name, const std::string& value) {
        browse.append_child(name).text().set(value.c_str());
    };
    addArg("ObjectID", params.objectId);
    addArg("BrowseFlag", browseFlagName(params.browseFlag));
    addArg("Filter", params.filter);
    addArg("StartingIndex", std::to_string(params.startingIndex));
    addArg("RequestedCount", std::to_string(params.requestedCount));
    addArg("SortCriteria", params.sortCriteria);

    std::ostringstream out;
    out << R"(<?xml version="1.0" encoding="utf-8"?>)" << "\n";
    doc.save(out, "", pugi::format_raw);

    SoapRequest req;
    req.body = out.str();
    req.soapAction = "\"" + serviceType + "#Browse\"";
    req.contentType = "text/xml; charset=\"utf-8\"";
    return req;
}

} // namespace upnp
