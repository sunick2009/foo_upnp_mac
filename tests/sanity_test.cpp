#include <catch2/catch_test_macros.hpp>

#include <pugixml.hpp>

TEST_CASE("build sanity: pugixml parses a trivial document", "[sanity]") {
    pugi::xml_document doc;
    auto result = doc.load_string("<root><child>value</child></root>");
    REQUIRE(result);
    REQUIRE(std::string(doc.child("root").child("child").child_value()) == "value");
}
