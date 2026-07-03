#include <catch2/catch_test_macros.hpp>

#include "http/HttpClient.hpp"
#include "upnp/UpnpError.hpp"

// Network-free failure-path tests. Success paths are covered by the
// end-to-end mock-server test (docs/phase0-cli.md).

TEST_CASE("connection refused throws HttpException with transport code",
          "[http]") {
    upnp::HttpClient::Options opts;
    opts.timeoutSeconds = 2;
    upnp::HttpClient client(opts);

    // port 9 (discard) is almost never open; connection is refused fast
    try {
        client.get("http://127.0.0.1:9/rootDesc.xml");
        FAIL("expected HttpException");
    } catch (const upnp::HttpException& e) {
        CHECK(e.statusCode == -1);
        CHECK(std::string(e.what()).find("127.0.0.1") != std::string::npos);
    }
}

TEST_CASE("invalid URL throws HttpException", "[http]") {
    upnp::HttpClient client;
    CHECK_THROWS_AS(client.get("not-a-url"), upnp::HttpException);
}
