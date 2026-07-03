#include "SDK/foobar2000.h"

#include "DmsConfig.hpp"

namespace dms {

namespace {

// {72A4825B-DDCB-49A5-84BE-E07119105A2D}
const GUID kServersGuid = {0x72a4825b, 0xddcb, 0x49a5,
    {0x84, 0xbe, 0xe0, 0x71, 0x19, 0x10, 0x5a, 0x2d}};

cfg_var_modern::cfg_string g_servers(kServersGuid, "[]");

} // namespace

std::vector<component::ServerEntry> loadServers() {
    return component::parseServerList(g_servers.get().get_ptr());
}

void saveServers(const std::vector<component::ServerEntry>& servers) {
    g_servers.set(component::serializeServerList(servers).c_str());
}

} // namespace dms
