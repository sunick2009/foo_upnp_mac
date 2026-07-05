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

if(ENABLE_FB2K_SDK_CHECK OR ENABLE_FB2K_COMPONENT)
    include(fb2k_sdk.cmake)
endif()

# --- foo_dms_browser: the actual component (ADR-014..017, docs/20) -------------
if(ENABLE_FB2K_COMPONENT)
    add_library(foo_dms_browser MODULE
        foo_dms_browser/ComponentEntry.mm
        foo_dms_browser/DmsAlbumArtFallback.mm
        foo_dms_browser/DmsBrowserUiElement.mm
        foo_dms_browser/DmsBrowserViewController.mm
        foo_dms_browser/DmsBrowserWindowController.mm
        foo_dms_browser/DmsConfig.mm
        foo_dms_browser/DmsPlaylistIntegration.mm
        foo_dms_browser/DmsPreferencesPage.mm
        foo_dms_browser/DmsTreeNode.mm
        foo_dms_browser/MainMenuCommands.mm
    )
    set_target_properties(foo_dms_browser PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION component
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS ON
    )
    target_compile_options(foo_dms_browser PRIVATE
        "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>"
    )
    target_compile_definitions(foo_dms_browser PRIVATE
        "$<$<CONFIG:Debug>:DEBUG=1>"
    )
    target_link_libraries(foo_dms_browser PRIVATE
        upnp_adapter
        fb2k_component_client
        fb2k_sdk_helpers
        fb2k_sdk
        fb2k_shared
        fb2k_pfc
        "-framework Cocoa"
    )
endif()
