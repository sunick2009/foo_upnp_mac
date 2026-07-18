#include <catch2/catch_test_macros.hpp>

#include "TestHelpers.hpp"
#include "upnp/SoapBrowseResponseParser.hpp"
#include "upnp/UpnpError.hpp"

using upnp::SoapBrowseResponseParser;

TEST_CASE("extracts unescaped DIDL-Lite and counts", "[parser][soap]") {
    auto result = SoapBrowseResponseParser::parse(
        readFixture("soap_responses/browse_root_response.xml"));

    CHECK(result.numberReturned == 2);
    CHECK(result.totalMatches == 2);
    CHECK(result.updateId == 17);
    // must be real XML now, not the escaped entity form
    CHECK(result.resultXml.find("<DIDL-Lite") == 0);
    CHECK(result.resultXml.find("&lt;") == std::string::npos);
    CHECK(result.resultXml.find("childCount=\"120\"") != std::string::npos);
}

TEST_CASE("handles SOAP-ENV prefixed envelopes", "[parser][soap]") {
    auto result = SoapBrowseResponseParser::parse(
        readFixture("soap_responses/browse_response_soapenv_prefix.xml"));

    CHECK(result.numberReturned == 0);
    CHECK(result.updateId == 3);
    CHECK(result.resultXml.find("<DIDL-Lite") == 0);
}

TEST_CASE("SOAP fault throws SoapFaultException with upnp code", "[parser][soap]") {
    try {
        SoapBrowseResponseParser::parse(readFixture("soap_responses/soap_fault.xml"));
        FAIL("expected SoapFaultException");
    } catch (const upnp::SoapFaultException& e) {
        CHECK(e.faultCode == 701);
        CHECK(e.faultString == "No such object");
    }
}

TEST_CASE("malformed or unexpected documents throw XmlParseException",
          "[parser][soap]") {
    CHECK_THROWS_AS(SoapBrowseResponseParser::parse("not xml at all <"),
                    upnp::XmlParseException);
    CHECK_THROWS_AS(SoapBrowseResponseParser::parse("<html><body>404</body></html>"),
                    upnp::XmlParseException);
}

// Captured verbatim from MiniDLNA 1.3.3 (2026-07-18, issue #6).
TEST_CASE("parses a real MiniDLNA 1.3.3 browse response", "[parser][soap]") {
    const auto response = SoapBrowseResponseParser::parse(
        readFixture("soap_responses/minidlna_133_browse_response.xml"));
    CHECK(response.numberReturned == 4);
    CHECK(response.totalMatches == 13);
    CHECK(response.resultXml.find("Mini Test Album") != std::string::npos);
    CHECK(response.resultXml.find("第二首歌") != std::string::npos);
}
