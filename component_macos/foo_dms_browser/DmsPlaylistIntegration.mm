#include "SDK/foobar2000.h"

#include "DmsPlaylistIntegration.h"

#include "component/HintFields.hpp"
#include "upnp/ResourceSelector.hpp"

namespace dms {

size_t addToActivePlaylist(const std::vector<upnp::UpnpObject>& objects) {
    auto mdb = metadb::get();
    auto hints = metadb_hint_list::create();
    metadb_hint_list_v2::ptr hintsV2;
    hintsV2 &= hints; // verified available on fb2k mac (ADR-016 spike)

    metadb_handle_list handles;
    for (const auto& object : objects) {
        if (object.type == upnp::UpnpObjectType::Container) continue;
        const auto resource = upnp::selectBestResource(object.resources);
        if (!resource) continue;

        metadb_handle_ptr handle = mdb->handle_create(resource->url.c_str(), 0);

        const auto hint = component::hintFieldsFor(object, resource->duration);
        file_info_impl info;
        for (const auto& [field, value] : hint.meta)
            info.meta_set(field.c_str(), value.c_str());
        if (hint.lengthSeconds) info.set_length(*hint.lengthSeconds);

        if (hintsV2.is_valid())
            hintsV2->add_hint_browse(handle, info, pfc::fileTimeNow());
        handles.add_item(handle);
    }

    if (handles.get_count() == 0) return 0;
    if (hintsV2.is_valid()) hintsV2->on_done();

    auto plm = playlist_manager::get();
    t_size playlist = plm->get_active_playlist();
    if (playlist == pfc_infinite) {
        playlist = plm->create_playlist("DMS Browser", pfc_infinite, pfc_infinite);
        plm->set_active_playlist(playlist);
    }
    plm->playlist_add_items(playlist, handles, bit_array_false());
    return handles.get_count();
}

} // namespace dms
