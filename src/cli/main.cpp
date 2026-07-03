#include <cstdlib>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include "http/HttpClient.hpp"
#include "upnp/ContentDirectoryClient.hpp"
#include "upnp/ResourceSelector.hpp"
#include "upnp/UpnpError.hpp"

namespace {

struct CliOptions {
    std::string server;
    upnp::BrowseParams params;
    long timeoutSeconds = 10;
    std::string output = "json"; // "json" | "table"
};

void printUsage() {
    std::cout <<
        "upnp-browser-cli - browse a UPnP/DLNA MediaServer (Phase 0 PoC)\n"
        "\n"
        "Usage:\n"
        "  upnp-browser-cli browse --server <rootDesc.xml URL> [options]\n"
        "\n"
        "Options:\n"
        "  --server <url>           rootDesc.xml URL (required)\n"
        "  --object-id <id>         ObjectID to browse (default: 0)\n"
        "  --browse-flag <flag>     BrowseDirectChildren | BrowseMetadata\n"
        "                           (default: BrowseDirectChildren)\n"
        "  --starting-index <n>     pagination start (default: 0)\n"
        "  --requested-count <n>    max items, 0 = server decides (default: 100)\n"
        "  --timeout <seconds>      request timeout (default: 10)\n"
        "  --output <format>        json | table (default: json)\n";
}

std::optional<CliOptions> parseArgs(int argc, char** argv) {
    CliOptions opts;

    auto nextValue = [&](int& i) -> const char* {
        if (i + 1 >= argc) {
            std::cerr << "Error: missing value for " << argv[i] << "\n";
            return nullptr;
        }
        return argv[++i];
    };

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        const char* value = nullptr;
        if (arg == "--server") {
            if (!(value = nextValue(i))) return std::nullopt;
            opts.server = value;
        } else if (arg == "--object-id") {
            if (!(value = nextValue(i))) return std::nullopt;
            opts.params.objectId = value;
        } else if (arg == "--browse-flag") {
            if (!(value = nextValue(i))) return std::nullopt;
            std::string flag = value;
            if (flag == "BrowseDirectChildren") {
                opts.params.browseFlag = upnp::BrowseFlag::DirectChildren;
            } else if (flag == "BrowseMetadata") {
                opts.params.browseFlag = upnp::BrowseFlag::Metadata;
            } else {
                std::cerr << "Error: unknown browse flag: " << flag << "\n";
                return std::nullopt;
            }
        } else if (arg == "--starting-index") {
            if (!(value = nextValue(i))) return std::nullopt;
            opts.params.startingIndex = static_cast<uint32_t>(std::strtoul(value, nullptr, 10));
        } else if (arg == "--requested-count") {
            if (!(value = nextValue(i))) return std::nullopt;
            opts.params.requestedCount = static_cast<uint32_t>(std::strtoul(value, nullptr, 10));
        } else if (arg == "--timeout") {
            if (!(value = nextValue(i))) return std::nullopt;
            opts.timeoutSeconds = std::strtol(value, nullptr, 10);
        } else if (arg == "--output") {
            if (!(value = nextValue(i))) return std::nullopt;
            opts.output = value;
            if (opts.output != "json" && opts.output != "table") {
                std::cerr << "Error: unknown output format: " << opts.output << "\n";
                return std::nullopt;
            }
        } else {
            std::cerr << "Error: unknown option: " << arg << "\n";
            return std::nullopt;
        }
    }

    if (opts.server.empty()) {
        std::cerr << "Error: --server is required\n";
        return std::nullopt;
    }
    return opts;
}

// --- JSON output -------------------------------------------------------------

std::string jsonEscape(const std::string& s) {
    std::ostringstream out;
    for (unsigned char c : s) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out << buf;
                } else {
                    out << c;
                }
        }
    }
    return out.str();
}

std::string jsonString(const std::string& s) { return "\"" + jsonEscape(s) + "\""; }

void printResourceJson(std::ostream& out, const upnp::UpnpResource& res,
                       const std::string& indent) {
    out << indent << "{\n";
    out << indent << "  \"url\": " << jsonString(res.url) << ",\n";
    out << indent << "  \"protocol_info\": " << jsonString(res.protocolInfo) << ",\n";
    out << indent << "  \"mime_type\": " << jsonString(res.mimeType);
    if (!res.duration.empty()) {
        out << ",\n" << indent << "  \"duration\": " << jsonString(res.duration);
    }
    if (res.size) {
        out << ",\n" << indent << "  \"size\": " << *res.size;
    }
    if (res.bitrate) {
        out << ",\n" << indent << "  \"bitrate\": " << *res.bitrate;
    }
    out << "\n" << indent << "}";
}

void printJson(const CliOptions& opts, const upnp::ContentDirectoryClient& client,
               const upnp::BrowseResponse& response) {
    std::ostream& out = std::cout;
    out << "{\n";
    out << "  \"server\": " << jsonString(opts.server) << ",\n";
    out << "  \"friendly_name\": " << jsonString(client.description().friendlyName) << ",\n";
    out << "  \"control_url\": " << jsonString(client.controlUrl()) << ",\n";
    out << "  \"object_id\": " << jsonString(opts.params.objectId) << ",\n";
    out << "  \"number_returned\": " << response.numberReturned << ",\n";
    out << "  \"total_matches\": " << response.totalMatches << ",\n";
    out << "  \"items\": [\n";

    for (size_t i = 0; i < response.objects.size(); ++i) {
        const auto& obj = response.objects[i];
        out << "    {\n";
        const char* type = obj.type == upnp::UpnpObjectType::Container ? "container"
                         : obj.type == upnp::UpnpObjectType::AudioItem ? "item"
                                                                        : "unknown";
        out << "      \"type\": \"" << type << "\",\n";
        out << "      \"id\": " << jsonString(obj.id) << ",\n";
        out << "      \"parent_id\": " << jsonString(obj.parentId) << ",\n";
        out << "      \"title\": " << jsonString(obj.title);

        if (obj.type == upnp::UpnpObjectType::Container) {
            if (obj.childCount) {
                out << ",\n      \"child_count\": " << *obj.childCount;
            }
        } else {
            if (obj.artist) out << ",\n      \"artist\": " << jsonString(*obj.artist);
            if (obj.album) out << ",\n      \"album\": " << jsonString(*obj.album);
            if (obj.genre) out << ",\n      \"genre\": " << jsonString(*obj.genre);
            if (obj.albumArtUri) {
                out << ",\n      \"album_art_uri\": " << jsonString(*obj.albumArtUri);
            }
            if (!obj.resources.empty()) {
                out << ",\n      \"resources\": [\n";
                for (size_t r = 0; r < obj.resources.size(); ++r) {
                    printResourceJson(out, obj.resources[r], "        ");
                    if (r + 1 < obj.resources.size()) out << ",";
                    out << "\n";
                }
                out << "      ]";
                // debug aid (ADR-007): show which res the selector would play
                auto best = upnp::selectBestResource(obj.resources);
                out << ",\n      \"selected_resource\": ";
                if (best) {
                    out << jsonString(best->url);
                } else {
                    out << "null";
                }
            }
        }
        out << "\n    }";
        if (i + 1 < response.objects.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n";
    out << "}\n";
}

// --- table output --------------------------------------------------------------

void printTable(const upnp::BrowseResponse& response) {
    std::printf("%-10s %-24s %-40s %s\n", "TYPE", "ID", "TITLE", "DETAIL");
    for (const auto& obj : response.objects) {
        if (obj.type == upnp::UpnpObjectType::Container) {
            std::string detail =
                obj.childCount ? std::to_string(*obj.childCount) + " children" : "";
            std::printf("%-10s %-24s %-40s %s\n", "container", obj.id.c_str(),
                        obj.title.c_str(), detail.c_str());
        } else {
            auto best = upnp::selectBestResource(obj.resources);
            std::string detail = best ? best->mimeType + " " + best->url : "(unplayable)";
            std::printf("%-10s %-24s %-40s %s\n", "item", obj.id.c_str(),
                        obj.title.c_str(), detail.c_str());
        }
    }
    std::printf("\n%u of %u item(s)\n", response.numberReturned, response.totalMatches);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2 || std::string(argv[1]) != "browse") {
        printUsage();
        return argc < 2 ? 0 : 2;
    }

    auto opts = parseArgs(argc, argv);
    if (!opts) {
        std::cerr << "\nRun without arguments for usage.\n";
        return 2;
    }

    try {
        upnp::HttpClient::Options httpOptions;
        httpOptions.timeoutSeconds = opts->timeoutSeconds;
        upnp::HttpClient http(httpOptions);

        auto client = upnp::ContentDirectoryClient::connect(http, opts->server);
        auto response = client.browse(opts->params);

        if (opts->output == "json") {
            printJson(*opts, client, response);
        } else {
            printTable(response);
        }
        return 0;
    } catch (const upnp::SoapFaultException& e) {
        std::cerr << "Error [SoapFault " << e.faultCode << "]: " << e.faultString << "\n";
    } catch (const upnp::HttpException& e) {
        if (e.statusCode > 0) {
            std::cerr << "Error [Http " << e.statusCode << "]: " << e.what() << "\n";
        } else {
            std::cerr << "Error [Network]: " << e.what() << "\n";
        }
    } catch (const upnp::UpnpException& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
    return 1;
}
