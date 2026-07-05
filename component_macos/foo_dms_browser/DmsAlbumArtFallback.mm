// Album art fallback (ADR-016): fb2k asks fallbacks when no embedded or
// external art exists, which is always the case for remote UPnP streams.
// We answer by downloading the DIDL albumArtURI that addToActivePlaylist
// hinted into metadb as UPNP_ALBUM_ART_URI.
#include "SDK/foobar2000.h"

#include <algorithm>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "component/HintFields.hpp"
#include "http/HttpClient.hpp"
#include "upnp/UpnpError.hpp"

namespace {

// Successful downloads only; transient failures stay retryable. Cleared
// wholesale at the cap — art queries are rare enough that LRU is overkill.
class ArtCache {
public:
    bool get(const std::string& url, album_art_data_ptr& out) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(url);
        if (it == entries_.end()) return false;
        out = it->second;
        return true;
    }

    void put(const std::string& url, album_art_data_ptr data) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (entries_.size() >= 64) entries_.clear();
        entries_[url] = data;
    }

private:
    std::mutex mutex_;
    std::unordered_map<std::string, album_art_data_ptr> entries_;
};

ArtCache& artCache() {
    static ArtCache cache;
    return cache;
}

album_art_data_ptr downloadArt(const std::string& url, abort_callback& abort) {
    album_art_data_ptr cached;
    if (artCache().get(url, cached)) return cached;

    abort.check();
    upnp::HttpClient::Response response;
    try {
        response = upnp::HttpClient().get(url);
    } catch (const upnp::HttpException&) {
        return nullptr;
    }
    abort.check();
    if (response.statusCode != 200 || response.body.empty()) return nullptr;

    auto data = album_art_data_impl::g_create(response.body.data(),
                                              response.body.size());
    artCache().put(url, data);
    return data;
}

class UriPathList : public album_art_path_list {
public:
    explicit UriPathList(std::vector<std::string> paths)
        : paths_(std::move(paths)) {}
    const char* get_path(t_size index) const override {
        return paths_[index].c_str();
    }
    t_size get_count() const override { return paths_.size(); }

private:
    std::vector<std::string> paths_;
};

class DmsAlbumArtInstance : public album_art_extractor_instance_v2 {
public:
    explicit DmsAlbumArtInstance(std::vector<std::string> uris)
        : uris_(std::move(uris)) {}

    album_art_data_ptr query(const GUID& what, abort_callback& abort) override {
        if (what != album_art_ids::cover_front) throw exception_album_art_not_found();
        for (const auto& uri : uris_) {
            auto data = downloadArt(uri, abort);
            if (data.is_valid()) {
                served_ = uri;
                return data;
            }
        }
        throw exception_album_art_not_found();
    }

    album_art_path_list::ptr query_paths(const GUID& what,
                                         abort_callback& abort) override {
        (void)abort;
        std::vector<std::string> paths;
        if (what == album_art_ids::cover_front && !served_.empty())
            paths.push_back(served_);
        return fb2k::service_new<UriPathList>(std::move(paths));
    }

private:
    std::vector<std::string> uris_;
    std::string served_;
};

class DmsAlbumArtFallback : public album_art_fallback {
public:
    album_art_extractor_instance_v2::ptr open(
        metadb_handle_list_cref items,
        pfc::list_base_const_t<GUID> const& ids,
        abort_callback& abort) override {
        (void)ids;
        (void)abort;
        std::vector<std::string> uris;
        for (t_size i = 0; i < items.get_count(); ++i) {
            // addToActivePlaylist hints via add_hint_browse, so the field
            // lives in browse info; primary info wins if both exist.
            metadb_info_container::ptr info, browse;
            items[i]->get_browse_info_ref(info, browse);
            const char* uri = nullptr;
            if (info.is_valid())
                uri = info->info().meta_get(component::kAlbumArtUriField, 0);
            if ((uri == nullptr || *uri == '\0') && browse.is_valid())
                uri = browse->info().meta_get(component::kAlbumArtUriField, 0);
            if (uri == nullptr || *uri == '\0') continue;
            if (std::find(uris.begin(), uris.end(), uri) == uris.end())
                uris.emplace_back(uri);
        }
        // Never return null: fb2k core dereferences the result without a
        // null check (verified by a real crash). An instance with no URIs
        // reports not-found through the documented exception path instead.
        return fb2k::service_new<DmsAlbumArtInstance>(std::move(uris));
    }
};

FB2K_SERVICE_FACTORY(DmsAlbumArtFallback);

} // namespace
