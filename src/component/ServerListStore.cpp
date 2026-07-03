#include "component/ServerListStore.hpp"

#include <cstdint>

namespace component {

bool operator==(const ServerEntry& a, const ServerEntry& b) {
    return a.name == b.name && a.url == b.url;
}

namespace {

// Minimal strict parser for the subset we serialize: an array of flat
// objects with string values. Anything unexpected fails the parse.
class Parser {
public:
    explicit Parser(const std::string& text) : text_(text) {}

    bool parse(std::vector<ServerEntry>& out) {
        skipWs();
        if (!consume('[')) return false;
        skipWs();
        if (consume(']')) return true;
        while (true) {
            ServerEntry entry;
            if (!parseObject(entry)) return false;
            out.push_back(std::move(entry));
            skipWs();
            if (consume(']')) return true;
            if (!consume(',')) return false;
            skipWs();
        }
    }

private:
    bool parseObject(ServerEntry& entry) {
        skipWs();
        if (!consume('{')) return false;
        skipWs();
        if (consume('}')) return true;
        while (true) {
            std::string key, value;
            if (!parseString(key)) return false;
            skipWs();
            if (!consume(':')) return false;
            skipWs();
            if (!parseString(value)) return false;
            if (key == "name") entry.name = value;
            else if (key == "url") entry.url = value;
            // unknown keys ignored (forward compatibility, ADR-015)
            skipWs();
            if (consume('}')) return true;
            if (!consume(',')) return false;
            skipWs();
        }
    }

    bool parseString(std::string& out) {
        if (!consume('"')) return false;
        while (pos_ < text_.size()) {
            char c = text_[pos_++];
            if (c == '"') return true;
            if (c == '\\') {
                if (pos_ >= text_.size()) return false;
                char esc = text_[pos_++];
                switch (esc) {
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'n': out += '\n'; break;
                    case 'r': out += '\r'; break;
                    case 't': out += '\t'; break;
                    case 'u': {
                        if (!appendUnicodeEscape(out)) return false;
                        break;
                    }
                    default: return false;
                }
            } else {
                out += c;
            }
        }
        return false; // unterminated
    }

    bool appendUnicodeEscape(std::string& out) {
        if (pos_ + 4 > text_.size()) return false;
        uint32_t cp = 0;
        for (int i = 0; i < 4; ++i) {
            char c = text_[pos_++];
            cp <<= 4;
            if (c >= '0' && c <= '9') cp |= static_cast<uint32_t>(c - '0');
            else if (c >= 'a' && c <= 'f') cp |= static_cast<uint32_t>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') cp |= static_cast<uint32_t>(c - 'A' + 10);
            else return false;
        }
        // Surrogate pair for non-BMP code points.
        if (cp >= 0xD800 && cp <= 0xDBFF) {
            if (pos_ + 6 > text_.size() || text_[pos_] != '\\' || text_[pos_ + 1] != 'u')
                return false;
            pos_ += 2;
            uint32_t low = 0;
            for (int i = 0; i < 4; ++i) {
                char c = text_[pos_++];
                low <<= 4;
                if (c >= '0' && c <= '9') low |= static_cast<uint32_t>(c - '0');
                else if (c >= 'a' && c <= 'f') low |= static_cast<uint32_t>(c - 'a' + 10);
                else if (c >= 'A' && c <= 'F') low |= static_cast<uint32_t>(c - 'A' + 10);
                else return false;
            }
            if (low < 0xDC00 || low > 0xDFFF) return false;
            cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
        } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
            return false; // lone low surrogate
        }
        appendUtf8(out, cp);
        return true;
    }

    static void appendUtf8(std::string& out, uint32_t cp) {
        if (cp < 0x80) {
            out += static_cast<char>(cp);
        } else if (cp < 0x800) {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }

    void skipWs() {
        while (pos_ < text_.size()) {
            char c = text_[pos_];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') ++pos_;
            else break;
        }
    }

    bool consume(char expected) {
        if (pos_ < text_.size() && text_[pos_] == expected) {
            ++pos_;
            return true;
        }
        return false;
    }

    const std::string& text_;
    size_t pos_ = 0;
};

void appendJsonString(std::string& out, const std::string& value) {
    out += '"';
    for (char c : value) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    static const char* hex = "0123456789abcdef";
                    out += "\\u00";
                    out += hex[(c >> 4) & 0xF];
                    out += hex[c & 0xF];
                } else {
                    out += c; // UTF-8 bytes pass through
                }
        }
    }
    out += '"';
}

} // namespace

std::string trimWhitespace(const std::string& value) {
    const char* whitespace = " \t\r\n";
    const auto begin = value.find_first_not_of(whitespace);
    if (begin == std::string::npos) return {};
    const auto end = value.find_last_not_of(whitespace);
    return value.substr(begin, end - begin + 1);
}

std::vector<ServerEntry> parseServerList(const std::string& json) {
    std::vector<ServerEntry> entries;
    Parser parser(json);
    if (!parser.parse(entries)) return {};
    for (auto& entry : entries) {
        entry.name = trimWhitespace(entry.name);
        entry.url = trimWhitespace(entry.url);
    }
    return entries;
}

std::string serializeServerList(const std::vector<ServerEntry>& entries) {
    std::string out = "[";
    bool first = true;
    for (const auto& entry : entries) {
        if (!first) out += ',';
        first = false;
        out += "{\"name\":";
        appendJsonString(out, entry.name);
        out += ",\"url\":";
        appendJsonString(out, entry.url);
        out += '}';
    }
    out += ']';
    return out;
}

} // namespace component
