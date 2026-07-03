// THROWAWAY spike (docs/20 step 1): verify metadb_hint_list works on
// mac. On startup, add one remote http track with DIDL-style hinted
// metadata to a dedicated "DMS Spike" playlist, then read the fields
// back via title formatting and record the result. If the read-back
// shows the hinted values, ADR-016's risk gate passes.
// Delete this file (and its CMake entry) once the spike is decided.
#include "SDK/foobar2000.h"

#import <Foundation/Foundation.h>

#include <fstream>
#include <string>

namespace {

constexpr const char* kSpikePlaylist = "DMS Spike";

void writeMarker(const std::string& line) {
#ifdef FB2K_HINT_SPIKE_MARKER_FILE
    std::ofstream marker(FB2K_HINT_SPIKE_MARKER_FILE, std::ios::app);
    marker << line << "\n";
#endif
    FB2K_console_formatter() << "foo_dms_browser spike: " << line.c_str();
}

file_info_impl makeSpikeInfo(const char* variant) {
    file_info_impl info;
    info.meta_set("title", (std::string("Spike Title 測試曲 ") + variant).c_str());
    info.meta_set("artist", "Spike Artist");
    info.meta_set("album", "Spike Album");
    info.set_length(123.0);
    return info;
}

std::string readBack(const metadb_handle_ptr& handle) {
    titleformat_object::ptr script;
    titleformat_compiler::get()->compile_force(
        script, "%title%|%artist%|%album%|%length_seconds%");
    pfc::string8 out;
    handle->format_title(nullptr, out, script, nullptr);
    return out.get_ptr();
}

void runSpike() {
    try {
        auto mdb = metadb::get();
        auto plm = playlist_manager::get();
        metadb_handle_list items;

        // Variant A: primary hint, freshflag=false (playlist-file semantics).
        metadb_handle_ptr a = mdb->handle_create(
            "http://example.com/dms-spike-a.flac", 0);
        {
            auto hints = metadb_hint_list::create();
            hints->add_hint(a, makeSpikeInfo("A"), filestats_invalid, false);
            hints->on_done();
        }
        items.add_item(a);
        writeMarker(std::string("A(add_hint,fresh=false)=") + readBack(a));

        // Variant B: primary hint, freshflag=true (as-if-read-from-file).
        metadb_handle_ptr b = mdb->handle_create(
            "http://example.com/dms-spike-b.flac", 0);
        {
            auto hints = metadb_hint_list::create();
            hints->add_hint(b, makeSpikeInfo("B"), filestats_invalid, true);
            hints->on_done();
        }
        items.add_item(b);
        writeMarker(std::string("B(add_hint,fresh=true)=") + readBack(b));

        // Variant C: browse info (the mechanism behind m3u EXTINF titles,
        // semantically closest to DIDL-sourced metadata).
        metadb_handle_ptr c = mdb->handle_create(
            "http://example.com/dms-spike-c.flac", 0);
        {
            auto hints = metadb_hint_list::create();
            metadb_hint_list_v2::ptr v2;
            if (v2 &= hints) {
                v2->add_hint_browse(c, makeSpikeInfo("C"),
                                    pfc::fileTimeNow());
                v2->on_done();
                items.add_item(c);
                writeMarker(std::string("C(add_hint_browse)=") + readBack(c));
            } else {
                writeMarker("C(add_hint_browse)=NO_V2_INTERFACE");
            }
        }

        const t_size playlist = plm->find_or_create_playlist(kSpikePlaylist);
        plm->playlist_add_items(playlist, items, bit_array_false());
        writeMarker("DONE");
    } catch (const std::exception& e) {
        writeMarker(std::string("EXCEPTION=") + e.what());
    }
}

class SpikeInitQuit : public initquit {
public:
    void on_init() override {
        // Defer a few seconds so playlists and UI are fully up.
        dispatch_after(
            dispatch_time(DISPATCH_TIME_NOW, (int64_t)(3 * NSEC_PER_SEC)),
            dispatch_get_main_queue(), ^{ runSpike(); });
    }
    void on_quit() override {}
};

FB2K_SERVICE_FACTORY(SpikeInitQuit);

} // namespace
