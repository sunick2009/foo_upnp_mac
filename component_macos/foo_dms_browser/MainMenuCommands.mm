// View → DMS Browser menu command opening the browser window (ADR-014).
#include "SDK/foobar2000.h"

#import "DmsBrowserWindowController.h"

namespace {

// {5A0C3C91-7682-4166-9CE7-1FACF596D410}
const GUID kOpenBrowserGuid = {0x5a0c3c91, 0x7682, 0x4166,
    {0x9c, 0xe7, 0x1f, 0xac, 0xf5, 0x96, 0xd4, 0x10}};

class DmsMainMenuCommands : public mainmenu_commands {
public:
    t_uint32 get_command_count() override { return 1; }
    GUID get_command(t_uint32) override { return kOpenBrowserGuid; }
    void get_name(t_uint32, pfc::string_base& out) override {
        out = "DMS Browser";
    }
    bool get_description(t_uint32, pfc::string_base& out) override {
        out = "Browse UPnP/DLNA media servers.";
        return true;
    }
    GUID get_parent() override { return mainmenu_groups::view; }
    void execute(t_uint32, ctx_t) override {
        [DmsBrowserWindowController showBrowser];
    }
};

FB2K_SERVICE_FACTORY(DmsMainMenuCommands);

} // namespace
