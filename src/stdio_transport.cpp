#include "mcp/transport.hpp"

#include <cstdio>

namespace mcp {
namespace {

void strip_carriage_return(std::string& line)
{
    if (!line.empty() && line[line.size() - 1] == '\r') {
        line.erase(line.size() - 1);
    }
}

}

Transport::ReadStatus StdioTransport::read_line(std::string& line)
{
    line.clear();
    bool any = false;
    for (;;) {
        const int ch = std::fgetc(stdin);
        if (ch == EOF) {
            if (std::ferror(stdin)) {
                return kReadError;
            }
            if (!any) {
                return kReadEof;
            }
            strip_carriage_return(line);
            return kReadLine;
        }
        any = true;
        if (ch == '\n') {
            strip_carriage_return(line);
            return kReadLine;
        }
        if (line.size() >= static_cast<std::size_t>(kMaxMessageBytes)) {
            int drain = ch;
            while (drain != '\n' && drain != EOF) {
                drain = std::fgetc(stdin);
            }
            line.clear();
            return kReadTooLong;
        }
        line.push_back(static_cast<char>(ch));
    }
}

bool StdioTransport::write_line(const std::string& line)
{
    for (std::size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '\n' || line[i] == '\r') {
            return false;
        }
    }
    if (!line.empty()) {
        const std::size_t wrote = std::fwrite(line.c_str(), 1, line.size(), stdout);
        if (wrote != line.size()) {
            return false;
        }
    }
    if (std::fputc('\n', stdout) == EOF) {
        return false;
    }
    return std::fflush(stdout) == 0;
}

}
