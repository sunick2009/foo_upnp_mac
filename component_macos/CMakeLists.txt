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
