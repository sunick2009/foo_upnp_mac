#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc == 1) {
        std::cout << "upnp-browser-cli\n";
        std::cout << "Usage: upnp-browser-cli browse --server <rootDesc.xml URL> --object-id 0\n";
        return 0;
    }

    std::cout << "Phase 0 CLI skeleton: command parsing is not implemented yet.\n";
    return 0;
}
