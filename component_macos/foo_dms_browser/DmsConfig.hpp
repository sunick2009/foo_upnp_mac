#pragma once

#include <vector>

#include "component/ServerListStore.hpp"

namespace dms {

// The manually configured server list, persisted as a JSON array in a
// single fb2k cfg_var (ADR-015).
std::vector<component::ServerEntry> loadServers();
void saveServers(const std::vector<component::ServerEntry>& servers);

} // namespace dms
