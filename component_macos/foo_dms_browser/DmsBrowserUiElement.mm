// macOS layout element host for the reusable browser view controller (ADR-014).
#include "SDK/foobar2000.h"

#import "DmsBrowserViewController.h"

#include <cstring>

namespace {

// {0A8F25A3-A285-496C-99CC-242745C0DA14}
const GUID kBrowserUiElementGuid = {0x0a8f25a3, 0xa285, 0x496c,
    {0x99, 0xcc, 0x24, 0x27, 0x45, 0xc0, 0xda, 0x14}};

class DmsBrowserUiElement : public ui_element_mac {
public:
    service_ptr instantiate(service_ptr arg) override {
        (void)arg;
        return fb2k::wrapNSObject([DmsBrowserViewController new]);
    }

    bool match_name(const char* name) override {
        return name != nullptr && std::strcmp(name, "DMS Browser") == 0;
    }

    fb2k::stringRef get_name() override {
        return fb2k::makeString("DMS Browser");
    }

    GUID get_guid() override { return kBrowserUiElementGuid; }
};

FB2K_SERVICE_FACTORY(DmsBrowserUiElement);

} // namespace
