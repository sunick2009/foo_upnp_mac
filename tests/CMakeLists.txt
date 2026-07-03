# Catch2 v3: pinned via FetchContent (ADR-011)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.6.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(upnp_tests
    sanity_test.cpp
    model/test_upnp_object.cpp
    parser/test_device_description_parser.cpp
    parser/test_didl_lite_parser.cpp
    parser/test_soap_browse_request_builder.cpp
    parser/test_soap_browse_response_parser.cpp
    parser/test_url_resolver.cpp
)

target_link_libraries(upnp_tests PRIVATE
    upnp_core
    Catch2::Catch2WithMain
)

target_include_directories(upnp_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Fixture files are read relative to this compile-time path (ADR-011)
target_compile_definitions(upnp_tests PRIVATE
    FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(upnp_tests)
