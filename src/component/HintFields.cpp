#include "component/HintFields.hpp"

#include <cstdlib>

namespace component {

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
    if (object.artist && !object.artist->empty())
        data.meta.emplace_back("artist", *object.artist);
    if (object.album && !object.album->empty())
        data.meta.emplace_back("album", *object.album);
    if (object.genre && !object.genre->empty())
        data.meta.emplace_back("genre", *object.genre);
    data.lengthSeconds = parseDidlDuration(resourceDuration);
    return data;
}

} // namespace component
