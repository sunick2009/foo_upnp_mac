#pragma once

#include <optional>
#include <string>

#include "component/BrowsePager.hpp"
#include "http/HttpClient.hpp"
#include "upnp/ContentDirectoryClient.hpp"

namespace dms {

// One connected media server. Not thread-safe by itself: the browser
// funnels every call through a single serial worker queue (ADR-017),
// so the blocking HttpClient is never used concurrently.
class BrowseSession {
public:
    explicit BrowseSession(std::string rootDescUrl)
        : rootDescUrl_(std::move(rootDescUrl)) {}

    // Fetches the device description on first use. Throws upnp errors.
    void connectIfNeeded() {
        if (!client_) {
            client_.emplace(
                upnp::ContentDirectoryClient::connect(http_, rootDescUrl_));
        }
    }

    // All pages of one container (ADR-012 revision). Throws upnp errors.
    component::PagedBrowseResult fetchChildren(const std::string& objectId) {
        connectIfNeeded();
        return component::fetchAllChildren(
            [this](const upnp::BrowseParams& params) {
                return client_->browse(params);
            },
            objectId);
    }

    std::string friendlyName() const {
        return client_ ? client_->description().friendlyName : std::string{};
    }

    const std::string& rootDescUrl() const { return rootDescUrl_; }

private:
    std::string rootDescUrl_;
    upnp::HttpClient http_;
    std::optional<upnp::ContentDirectoryClient> client_;
};

} // namespace dms
