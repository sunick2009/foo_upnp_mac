# Catch2 v3: pinned via FetchContent (ADR-011)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.6.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(upnp_tests
    sanity_test.cpp
    adapter/test_browse_pager.cpp
    adapter/test_hint_fields.cpp
    adapter/test_recursive_browse_collector.cpp
    adapter/test_server_list_store.cpp
    http/test_http_client.cpp
    integration/test_content_directory_client.cpp
    model/test_upnp_object.cpp
    parser/test_device_description_parser.cpp
    parser/test_didl_lite_parser.cpp
    parser/test_resource_selector.cpp
    parser/test_soap_browse_request_builder.cpp
    parser/test_soap_browse_response_parser.cpp
    parser/test_url_resolver.cpp
)

# upnp_adapter publicly links (and re-exports headers of) upnp_core.
target_link_libraries(upnp_tests PRIVATE
    upnp_adapter
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

# End-to-end: CLI against the python mock server (requires python3)
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    add_test(NAME e2e_mock_server
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/e2e/run_e2e.sh
                $<TARGET_FILE:upnp-browser-cli>
                ${CMAKE_SOURCE_DIR}/tools/mock_upnp_server.py
    )
    set_tests_properties(e2e_mock_server PROPERTIES TIMEOUT 60)
endif()
