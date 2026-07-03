#include <catch2/catch_test_macros.hpp>

#include "upnp/UpnpError.hpp"
#include "upnp/UpnpObject.hpp"

TEST_CASE("UpnpObject defaults to Unknown with empty fields", "[model]") {
    upnp::UpnpObject obj;
    CHECK(obj.type == upnp::UpnpObjectType::Unknown);
    CHECK(obj.id.empty());
    CHECK_FALSE(obj.childCount.has_value());
    CHECK_FALSE(obj.artist.has_value());
    CHECK(obj.resources.empty());
}

TEST_CASE("exception hierarchy preserves diagnostic fields", "[model][error]") {
    SECTION("HttpException carries status code") {
        upnp::HttpException e("connection refused");
        CHECK(e.statusCode == -1);
        upnp::HttpException e2("not found", 404);
        CHECK(e2.statusCode == 404);
    }

    SECTION("SoapFaultException formats code into message") {
        upnp::SoapFaultException e(701, "No such object");
        CHECK(e.faultCode == 701);
        CHECK(e.faultString == "No such object");
        CHECK(std::string(e.what()) == "SOAP fault 701: No such object");
    }

    SECTION("all exceptions are catchable as UpnpException") {
        auto throwing = [] { throw upnp::ServiceNotFoundException("no ContentDirectory"); };
        CHECK_THROWS_AS(throwing(), upnp::UpnpException);
    }
}
