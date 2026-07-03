#include <catch2/catch_test_macros.hpp>

#include "TestHelpers.hpp"
#include "upnp/ContentDirectoryClient.hpp"
#include "upnp/UpnpError.hpp"

namespace {

// In-memory transport: canned GET response plus captured POST calls.
class FakeHttpClient : public upnp::HttpClient {
public:
    Response getResponse;
    Response postResponse;

    // captured for assertions
    std::string lastGetUrl;
    std::string lastPostUrl;
    std::string lastPostBody;
    std::string lastContentType;
    std::map<std::string, std::string> lastHeaders;

    Response get(const std::string& url) override {
        lastGetUrl = url;
        return getResponse;
    }

    Response post(const std::string& url, const std::string& body,
                  const std::string& contentType,
                  const std::map<std::string, std::string>& extraHeaders) override {
        lastPostUrl = url;
        lastPostBody = body;
        lastContentType = contentType;
        lastHeaders = extraHeaders;
        return postResponse;
    }
};

constexpr const char* kRootDescUrl = "http://192.168.1.10:8200/rootDesc.xml";

FakeHttpClient makeConnectedFake() {
    FakeHttpClient fake;
    fake.getResponse.statusCode = 200;
    fake.getResponse.body = readFixture("device_descriptions/minidlna_rootDesc.xml");
    return fake;
}

} // namespace

TEST_CASE("connect resolves control url from device description",
          "[integration][client]") {
    auto fake = makeConnectedFake();
    auto client = upnp::ContentDirectoryClient::connect(fake, kRootDescUrl);

    CHECK(fake.lastGetUrl == kRootDescUrl);
    CHECK(client.controlUrl() == "http://192.168.1.10:8200/ctl/ContentDir");
    CHECK(client.description().friendlyName == "MiniDLNA on nas");
}

TEST_CASE("browse posts soap and returns parsed objects",
          "[integration][client]") {
    auto fake = makeConnectedFake();
    auto client = upnp::ContentDirectoryClient::connect(fake, kRootDescUrl);

    fake.postResponse.statusCode = 200;
    fake.postResponse.body = readFixture("soap_responses/browse_root_response.xml");

    auto response = client.browse();

    CHECK(fake.lastPostUrl == "http://192.168.1.10:8200/ctl/ContentDir");
    CHECK(fake.lastHeaders.at("SOAPAction") ==
          "\"urn:schemas-upnp-org:service:ContentDirectory:1#Browse\"");
    CHECK(fake.lastPostBody.find("BrowseDirectChildren") != std::string::npos);

    CHECK(response.numberReturned == 2);
    CHECK(response.totalMatches == 2);
    REQUIRE(response.objects.size() == 2);
    CHECK(response.objects[0].title == "Music");
    CHECK(response.objects[0].childCount == 120);
}

TEST_CASE("soap fault beats generic http 500", "[integration][client]") {
    auto fake = makeConnectedFake();
    auto client = upnp::ContentDirectoryClient::connect(fake, kRootDescUrl);

    fake.postResponse.statusCode = 500;
    fake.postResponse.body = readFixture("soap_responses/soap_fault.xml");

    upnp::BrowseParams params;
    params.objectId = "does-not-exist";
    try {
        client.browse(params);
        FAIL("expected SoapFaultException");
    } catch (const upnp::SoapFaultException& e) {
        CHECK(e.faultCode == 701);
    }
}

TEST_CASE("non-soap error body surfaces as HttpException with status",
          "[integration][client]") {
    auto fake = makeConnectedFake();
    auto client = upnp::ContentDirectoryClient::connect(fake, kRootDescUrl);

    fake.postResponse.statusCode = 404;
    fake.postResponse.body = "<html><body>Not Found</body></html>";

    try {
        client.browse();
        FAIL("expected HttpException");
    } catch (const upnp::HttpException& e) {
        CHECK(e.statusCode == 404);
    }
}

TEST_CASE("connect propagates http errors for the rootDesc fetch",
          "[integration][client]") {
    FakeHttpClient fake;
    fake.getResponse.statusCode = 404;
    fake.getResponse.body = "not here";

    try {
        upnp::ContentDirectoryClient::connect(fake, kRootDescUrl);
        FAIL("expected HttpException");
    } catch (const upnp::HttpException& e) {
        CHECK(e.statusCode == 404);
        CHECK(std::string(e.what()).find(kRootDescUrl) != std::string::npos);
    }
}
