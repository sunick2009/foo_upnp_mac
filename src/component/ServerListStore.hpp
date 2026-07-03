#pragma once

#include <string>
#include <vector>

namespace component {

// One manually added media server (ADR-015).
struct ServerEntry {
    std::string name;
    std::string url; // device description (rootDesc.xml) URL
};

bool operator==(const ServerEntry& a, const ServerEntry& b);

// JSON array <-> entries, e.g. [{"name":"NAS","url":"http://..."}].
// The JSON lives inside a single fb2k cfg_var (ADR-015); unknown object
// keys are ignored for forward compatibility. Malformed input yields an
// empty list — a corrupt config must never take the component down.
std::vector<ServerEntry> parseServerList(const std::string& json);
std::string serializeServerList(const std::vector<ServerEntry>& entries);

} // namespace component
