cmake_minimum_required(VERSION 3.20)

project(foo_dms_browser_mac_phase0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dependencies (ADR-009, ADR-010) -----------------------------------------
# libcurl: system / Homebrew via find_package
find_package(CURL REQUIRED)

include(FetchContent)

# pugixml: pinned via FetchContent (ADR-010)
FetchContent_Declare(
    pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG        v1.14
)
FetchContent_MakeAvailable(pugixml)

# --- Core library --------------------------------------------------------------
add_library(upnp_core
    src/core/upnp/DeviceDescriptionParser.cpp
    src/core/upnp/SoapBrowseRequestBuilder.cpp
    src/core/upnp/UrlResolver.cpp
)

target_include_directories(upnp_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

target_link_libraries(upnp_core
    PUBLIC  pugixml::pugixml
    PRIVATE CURL::libcurl
)

# --- CLI -----------------------------------------------------------------------
add_executable(upnp-browser-cli
    src/cli/main.cpp
)

target_link_libraries(upnp-browser-cli PRIVATE upnp_core)

# --- Tests (ADR-011: Catch2 v3) --------------------------------------------------
enable_testing()
add_subdirectory(tests)
