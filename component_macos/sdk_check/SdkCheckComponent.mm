// Minimal component translation unit for the ADR-013 SDK link check.
// Declares a component version and, when FB2K_SDK_CHECK_MARKER_FILE is
// set, writes a marker file on init so a load test can verify the
// component actually ran inside foobar2000 without touching the UI.
#include "SDK/foobar2000.h"

#include <fstream>

DECLARE_COMPONENT_VERSION(
    "DMS Browser SDK Check",
    "0.0.1",
    "Throwaway target proving the foobar2000 SDK builds under CMake + CLT."
);

namespace {

class SdkCheckInitQuit : public initquit {
public:
    void on_init() override {
        FB2K_console_formatter() << "foo_dms_sdk_check: loaded, SDK runtime OK";
#ifdef FB2K_SDK_CHECK_MARKER_FILE
        std::ofstream marker(FB2K_SDK_CHECK_MARKER_FILE);
        marker << "foo_dms_sdk_check loaded in foobar2000 "
               << core_version_info::g_get_version_string() << "\n";
#endif
    }
    void on_quit() override {}
};

FB2K_SERVICE_FACTORY(SdkCheckInitQuit);

} // namespace
