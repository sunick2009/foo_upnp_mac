# macOS component build targets (ADR-013). Two independent verification
# gates, each behind its own option so their failure modes stay separate:
#   ENABLE_MACOS_BUNDLE_SMOKE_TEST — CLT toolchain can build an ObjC++ bundle
#   ENABLE_FB2K_SDK_CHECK          — foobar2000 SDK builds under CMake + CLT

if(ENABLE_MACOS_BUNDLE_SMOKE_TEST)
    add_library(foo_dms_browser_macos_smoke MODULE
        smoke/Fb2kSmokeComponent.mm
    )

    set_target_properties(foo_dms_browser_macos_smoke PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION bundle
        OUTPUT_NAME foo_dms_browser_macos_smoke
    )

    target_compile_features(foo_dms_browser_macos_smoke PRIVATE cxx_std_20)
    target_link_libraries(foo_dms_browser_macos_smoke PRIVATE
        "-framework Cocoa"
    )
endif()

if(ENABLE_FB2K_SDK_CHECK)
    include(fb2k_sdk.cmake)
endif()
