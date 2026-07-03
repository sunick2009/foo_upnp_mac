# foobar2000 macOS SDK compile check (ADR-013 step 2, the risk gate).
#
# The SDK ships Xcode projects only; per ADR-013 we port the build settings
# to CMake instead of maintaining .xcodeproj. Source lists, prefix headers,
# C++ standards and ARC below mirror the SDK's own Xcode targets
# (SDK-2025-03-07). The SDK itself is NOT committed to this repo — see
# the FATAL_ERROR message below for how to obtain it.

set(FB2K_SDK_ROOT "${CMAKE_SOURCE_DIR}/third_party/fb2k_sdk" CACHE PATH
    "Root of an extracted foobar2000 SDK (contains pfc/, foobar2000/)")

if(NOT EXISTS "${FB2K_SDK_ROOT}/foobar2000/SDK/foobar2000.h")
    message(FATAL_ERROR
        "foobar2000 SDK not found at ${FB2K_SDK_ROOT}.\n"
        "Download https://www.foobar2000.org/SDK and extract it there:\n"
        "  mkdir -p third_party/fb2k_sdk && cd third_party/fb2k_sdk\n"
        "  curl -sSLO https://www.foobar2000.org/downloads/SDK-2025-03-07.7z\n"
        "  tar -xf SDK-2025-03-07.7z\n"
        "Do not commit the SDK (ADR-013).")
endif()

# Common settings shared by every SDK static lib, matching the Xcode
# projects: header search paths `..`/`../..`, ARC for ObjC++.
function(fb2k_apply_common_settings target cxx_standard prefix_header)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD ${cxx_standard}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS ON
    )
    target_include_directories(${target} PUBLIC
        "${FB2K_SDK_ROOT}"
        "${FB2K_SDK_ROOT}/foobar2000"
    )
    target_compile_options(${target} PRIVATE
        "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>"
    )
    # The Xcode projects define DEBUG=1 in Debug config; pfc-lite.h warns
    # if neither DEBUG nor NDEBUG is set. Release already gets NDEBUG
    # from CMake's default flags.
    target_compile_definitions(${target} PRIVATE
        "$<$<CONFIG:Debug>:DEBUG=1>"
    )
    if(prefix_header)
        target_compile_options(${target} PRIVATE "-include" "${prefix_header}")
    endif()
endfunction()

# --- pfc (Xcode target: pfc-Mac) -------------------------------------------
set(FB2K_PFC_DIR "${FB2K_SDK_ROOT}/pfc")
add_library(fb2k_pfc STATIC
    ${FB2K_PFC_DIR}/audio_math.cpp
    ${FB2K_PFC_DIR}/audio_sample.cpp
    ${FB2K_PFC_DIR}/base64.cpp
    ${FB2K_PFC_DIR}/bigmem.cpp
    ${FB2K_PFC_DIR}/bit_array.cpp
    ${FB2K_PFC_DIR}/bsearch.cpp
    ${FB2K_PFC_DIR}/charDownConvert.cpp
    ${FB2K_PFC_DIR}/cpuid.cpp
    ${FB2K_PFC_DIR}/crashWithMessage.cpp
    ${FB2K_PFC_DIR}/filehandle.cpp
    ${FB2K_PFC_DIR}/filetimetools.cpp
    ${FB2K_PFC_DIR}/guid.cpp
    ${FB2K_PFC_DIR}/nix-objects.cpp
    ${FB2K_PFC_DIR}/obj-c.mm
    ${FB2K_PFC_DIR}/other.cpp
    ${FB2K_PFC_DIR}/pathUtils.cpp
    ${FB2K_PFC_DIR}/pfc-fb2k-hooks.cpp
    ${FB2K_PFC_DIR}/printf.cpp
    ${FB2K_PFC_DIR}/selftest.cpp
    ${FB2K_PFC_DIR}/SmartStrStr.cpp
    ${FB2K_PFC_DIR}/sort.cpp
    ${FB2K_PFC_DIR}/splitString2.cpp
    ${FB2K_PFC_DIR}/stdafx.cpp
    ${FB2K_PFC_DIR}/string_base.cpp
    ${FB2K_PFC_DIR}/string_conv.cpp
    ${FB2K_PFC_DIR}/string-compare.cpp
    ${FB2K_PFC_DIR}/string-conv-lite.cpp
    ${FB2K_PFC_DIR}/string-lite.cpp
    ${FB2K_PFC_DIR}/synchro_nix.cpp
    ${FB2K_PFC_DIR}/threads.cpp
    ${FB2K_PFC_DIR}/timers.cpp
    ${FB2K_PFC_DIR}/unicode-normalize.cpp
    ${FB2K_PFC_DIR}/utf8.cpp
    ${FB2K_PFC_DIR}/wildcard.cpp
    ${FB2K_PFC_DIR}/win-objects.cpp
)
fb2k_apply_common_settings(fb2k_pfc 17 "${FB2K_PFC_DIR}/pfc-lite.h")

# --- foobar2000 SDK ----------------------------------------------------------
set(FB2K_SDK_DIR "${FB2K_SDK_ROOT}/foobar2000/SDK")
add_library(fb2k_sdk STATIC
    ${FB2K_SDK_DIR}/abort_callback.cpp
    ${FB2K_SDK_DIR}/advconfig.cpp
    ${FB2K_SDK_DIR}/album_art.cpp
    ${FB2K_SDK_DIR}/app_close_blocker.cpp
    ${FB2K_SDK_DIR}/audio_chunk_channel_config.cpp
    ${FB2K_SDK_DIR}/audio_chunk.cpp
    ${FB2K_SDK_DIR}/cfg_var_legacy.cpp
    ${FB2K_SDK_DIR}/cfg_var.cpp
    ${FB2K_SDK_DIR}/chapterizer.cpp
    ${FB2K_SDK_DIR}/commandline.cpp
    ${FB2K_SDK_DIR}/commonObjects-Apple.mm
    ${FB2K_SDK_DIR}/commonObjects.cpp
    ${FB2K_SDK_DIR}/completion_notify.cpp
    ${FB2K_SDK_DIR}/componentversion.cpp
    ${FB2K_SDK_DIR}/config_io_callback.cpp
    ${FB2K_SDK_DIR}/config_object.cpp
    ${FB2K_SDK_DIR}/configStore.cpp
    ${FB2K_SDK_DIR}/console.cpp
    ${FB2K_SDK_DIR}/dsp_manager.cpp
    ${FB2K_SDK_DIR}/dsp.cpp
    ${FB2K_SDK_DIR}/file_cached_impl.cpp
    ${FB2K_SDK_DIR}/file_info_const_impl.cpp
    ${FB2K_SDK_DIR}/file_info_impl.cpp
    ${FB2K_SDK_DIR}/file_info_merge.cpp
    ${FB2K_SDK_DIR}/file_info.cpp
    ${FB2K_SDK_DIR}/file_operation_callback.cpp
    ${FB2K_SDK_DIR}/filesystem_helper.cpp
    ${FB2K_SDK_DIR}/filesystem.cpp
    ${FB2K_SDK_DIR}/foosort.cpp
    ${FB2K_SDK_DIR}/fsItem.cpp
    ${FB2K_SDK_DIR}/guids.cpp
    ${FB2K_SDK_DIR}/hasher_md5.cpp
    ${FB2K_SDK_DIR}/image.cpp
    ${FB2K_SDK_DIR}/input_file_type.cpp
    ${FB2K_SDK_DIR}/input.cpp
    ${FB2K_SDK_DIR}/link_resolver.cpp
    ${FB2K_SDK_DIR}/main_thread_callback.cpp
    ${FB2K_SDK_DIR}/mainmenu.cpp
    ${FB2K_SDK_DIR}/mem_block_container.cpp
    ${FB2K_SDK_DIR}/menu_helpers.cpp
    ${FB2K_SDK_DIR}/menu_item.cpp
    ${FB2K_SDK_DIR}/menu_manager.cpp
    ${FB2K_SDK_DIR}/metadb_handle_list.cpp
    ${FB2K_SDK_DIR}/metadb_handle.cpp
    ${FB2K_SDK_DIR}/metadb.cpp
    ${FB2K_SDK_DIR}/output.cpp
    ${FB2K_SDK_DIR}/packet_decoder.cpp
    ${FB2K_SDK_DIR}/playable_location.cpp
    ${FB2K_SDK_DIR}/playback_control.cpp
    ${FB2K_SDK_DIR}/playlist_loader.cpp
    ${FB2K_SDK_DIR}/playlist.cpp
    ${FB2K_SDK_DIR}/popup_message.cpp
    ${FB2K_SDK_DIR}/preferences_page.cpp
    ${FB2K_SDK_DIR}/replaygain_info.cpp
    ${FB2K_SDK_DIR}/replaygain.cpp
    ${FB2K_SDK_DIR}/service.cpp
    ${FB2K_SDK_DIR}/stdafx.cpp
    ${FB2K_SDK_DIR}/tag_processor_id3v2.cpp
    ${FB2K_SDK_DIR}/tag_processor.cpp
    ${FB2K_SDK_DIR}/threaded_process.cpp
    ${FB2K_SDK_DIR}/titleformat.cpp
    ${FB2K_SDK_DIR}/track_property.cpp
    ${FB2K_SDK_DIR}/ui_element.cpp
    ${FB2K_SDK_DIR}/ui.cpp
    ${FB2K_SDK_DIR}/utility.cpp
)
fb2k_apply_common_settings(fb2k_sdk 17 "${FB2K_SDK_DIR}/foobar2000-sdk-pch.h")
target_link_libraries(fb2k_sdk PUBLIC fb2k_pfc)

# --- shared -------------------------------------------------------------------
set(FB2K_SHARED_DIR "${FB2K_SDK_ROOT}/foobar2000/shared")
add_library(fb2k_shared STATIC
    ${FB2K_SHARED_DIR}/audio_math.cpp
    ${FB2K_SHARED_DIR}/shared-apple.mm
    ${FB2K_SHARED_DIR}/shared-nix.cpp
    ${FB2K_SHARED_DIR}/stdafx.cpp
    ${FB2K_SHARED_DIR}/utf8.cpp
)
fb2k_apply_common_settings(fb2k_shared 17 "${FB2K_SHARED_DIR}/shared.h")
target_link_libraries(fb2k_shared PUBLIC fb2k_pfc)

# --- SDK helpers ---------------------------------------------------------------
set(FB2K_HELPERS_DIR "${FB2K_SDK_ROOT}/foobar2000/helpers")
add_library(fb2k_sdk_helpers STATIC
    ${FB2K_HELPERS_DIR}/album_art_helpers.cpp
    ${FB2K_HELPERS_DIR}/cfg_guidlist.cpp
    ${FB2K_HELPERS_DIR}/cfg_var_import.cpp
    ${FB2K_HELPERS_DIR}/create_directory_helper.cpp
    ${FB2K_HELPERS_DIR}/cue_creator.cpp
    ${FB2K_HELPERS_DIR}/cue_parser_embedding.cpp
    ${FB2K_HELPERS_DIR}/cue_parser.cpp
    ${FB2K_HELPERS_DIR}/cuesheet_index_list.cpp
    ${FB2K_HELPERS_DIR}/dialog_resize_helper.cpp
    ${FB2K_HELPERS_DIR}/dropdown_helper.cpp
    ${FB2K_HELPERS_DIR}/dynamic_bitrate_helper.cpp
    ${FB2K_HELPERS_DIR}/file_list_helper.cpp
    ${FB2K_HELPERS_DIR}/file_move_helper.cpp
    ${FB2K_HELPERS_DIR}/file_win32_wrapper.cpp
    ${FB2K_HELPERS_DIR}/filetimetools.cpp
    ${FB2K_HELPERS_DIR}/input_helper_cue.cpp
    ${FB2K_HELPERS_DIR}/input_helpers.cpp
    ${FB2K_HELPERS_DIR}/mp3_utils.cpp
    ${FB2K_HELPERS_DIR}/packet_decoder_aac_common.cpp
    ${FB2K_HELPERS_DIR}/packet_decoder_mp3_common.cpp
    ${FB2K_HELPERS_DIR}/readers.cpp
    ${FB2K_HELPERS_DIR}/seekabilizer.cpp
    ${FB2K_HELPERS_DIR}/StdAfx.cpp
    ${FB2K_HELPERS_DIR}/stream_buffer_helper.cpp
    ${FB2K_HELPERS_DIR}/text_file_loader_v2.cpp
    ${FB2K_HELPERS_DIR}/text_file_loader.cpp
    ${FB2K_HELPERS_DIR}/ThreadUtils.cpp
    ${FB2K_HELPERS_DIR}/track_property_callback_impl.cpp
    ${FB2K_HELPERS_DIR}/VisUtils.cpp
    ${FB2K_HELPERS_DIR}/VolumeMap.cpp
    ${FB2K_HELPERS_DIR}/win-systemtime.cpp
    ${FB2K_HELPERS_DIR}/win32_misc.cpp
    ${FB2K_HELPERS_DIR}/writer_wav.cpp
)
fb2k_apply_common_settings(fb2k_sdk_helpers 17 "${FB2K_HELPERS_DIR}/StdAfx.h")
target_link_libraries(fb2k_sdk_helpers PUBLIC fb2k_sdk fb2k_shared)

# --- component client (the component entry point) -------------------------------
set(FB2K_CLIENT_DIR "${FB2K_SDK_ROOT}/foobar2000/foobar2000_component_client")
add_library(fb2k_component_client STATIC
    ${FB2K_CLIENT_DIR}/component_client.cpp
)
fb2k_apply_common_settings(fb2k_component_client 20 "")
target_link_libraries(fb2k_component_client PUBLIC fb2k_sdk)

# --- link check: minimal component bundle ---------------------------------------
# Mirrors foo_sample's link line: the five static libs + Cocoa, output a
# loadable bundle. Passing this proves the whole SDK builds and links
# under CMake + CLT.
add_library(foo_dms_sdk_check MODULE
    sdk_check/SdkCheckComponent.mm
)
set_target_properties(foo_dms_sdk_check PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION component
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS ON
)
target_compile_options(foo_dms_sdk_check PRIVATE
    "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>"
)
target_link_libraries(foo_dms_sdk_check PRIVATE
    fb2k_component_client
    fb2k_sdk_helpers
    fb2k_sdk
    fb2k_shared
    fb2k_pfc
    "-framework Cocoa"
)
