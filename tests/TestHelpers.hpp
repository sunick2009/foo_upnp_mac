#pragma once

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

// Reads a fixture file relative to tests/fixtures (FIXTURE_DIR is
// injected by CMake). Throws if the file is missing so a bad path
// fails the test instead of silently parsing an empty string.
inline std::string readFixture(const std::string& relativePath) {
    std::string path = std::string(FIXTURE_DIR) + "/" + relativePath;
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("fixture not found: " + path);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}
