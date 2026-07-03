// Minimal component translation unit for the ADR-013 SDK link check.
// Declares a component version so the bundle exports real SDK-backed
// symbols; no functionality beyond that.
#include "SDK/foobar2000.h"

DECLARE_COMPONENT_VERSION(
    "DMS Browser SDK Check",
    "0.0.1",
    "Throwaway target proving the foobar2000 SDK builds under CMake + CLT."
);
