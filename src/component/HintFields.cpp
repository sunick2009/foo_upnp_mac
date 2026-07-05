#include "component/HintFields.hpp"

#include <cstdlib>
#include <string>

namespace component {

namespace {

void addMeta(std::vector<std::pair<std::string, std::string>>& meta,
             const std::string& field,
             const std::optional<std::string>& value) {
    if (value && !value->empty()) meta.emplace_back(field, *value);
}

void addMetaIfDistinct(std::vector<std::pair<std::string, std::string>>& meta,
                       const std::string& field,
                       const std::optional<std::string>& value,
                       const std::optional<std::string>& other) {
    if (!value || value->empty()) return;
    if (other && *value == *other) return;
    meta.emplace_back(field, *value);
}

void addInfo(std::vector<std::pair<std::string, std::string>>& info,
             const std::string& field,
             const std::optional<uint32_t>& value) {
    if (value) info.emplace_back(field, std::to_string(*value));
}

} // namespace

std::optional<double> parseDidlDuration(const std::string& duration) {
    if (duration.empty()) return std::nullopt;

    // Split on ':' into at most three parts: [HH:]MM:SS[.fraction]
    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        size_t colon = duration.find(':', start);
        if (colon == std::string::npos) {
            parts.push_back(duration.substr(start));
            break;
        }
        parts.push_back(duration.substr(start, colon - start));
        start = colon + 1;
    }
    if (parts.size() < 2 || parts.size() > 3) return std::nullopt;

    double total = 0.0;
    for (const auto& part : parts) {
        if (part.empty()) return std::nullopt;
        char* end = nullptr;
        const double value = std::strtod(part.c_str(), &end);
        if (end == nullptr || *end != '\0' || value < 0) return std::nullopt;
        total = total * 60.0 + value;
    }
    return total;
}

HintData hintFieldsFor(const upnp::UpnpObject& object,
                       const std::string& resourceDuration) {
    HintData data;
    if (!object.title.empty()) data.meta.emplace_back("title", object.title);
    const auto artist = object.artist ? object.artist : object.creator;
    addMeta(data.meta, "artist", artist);
    addMeta(data.meta, "album artist", object.albumArtist);
    addMeta(data.meta, "album", object.album);
    addMeta(data.meta, "genre", object.genre);
    addMeta(data.meta, "date", object.date);
    addMeta(data.meta, "tracknumber", object.originalTrackNumber);
    addMeta(data.meta, "discnumber", object.discNumber);
    addMeta(data.meta, "totaldiscs", object.totalDiscs);
    addMeta(data.meta, "comment", object.longDescription);
    addMetaIfDistinct(data.meta, "creator", object.creator, artist);
    addMeta(data.meta, kAlbumArtUriField, object.albumArtUri);
    data.lengthSeconds = parseDidlDuration(resourceDuration);
    return data;
}

HintData hintFieldsFor(const upnp::UpnpObject& object,
                       const upnp::UpnpResource& resource) {
    HintData data = hintFieldsFor(object, resource.duration);
    if (resource.bitrate) {
        const uint32_t kbps = static_cast<uint32_t>((*resource.bitrate * 8 + 999) / 1000);
        data.info.emplace_back("bitrate", std::to_string(kbps));
    }
    addInfo(data.info, "bitspersample", resource.bitsPerSample);
    addInfo(data.info, "samplerate", resource.sampleFrequency);
    addInfo(data.info, "channels", resource.nrAudioChannels);
    return data;
}

} // namespace component
