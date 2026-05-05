#ifndef MCP_TRANSPORT_HPP
#define MCP_TRANSPORT_HPP

#include <string>

namespace mcp {

// Byte pipe for newline-delimited messages. Implementations must not write
// anything except complete messages, and must flush after each line.
// Logs belong on stderr, never on this stream.
class Transport {
public:
    enum ReadStatus {
        kReadLine,
        kReadEof,
        kReadTooLong,
        kReadError
    };

    enum { kMaxMessageBytes = 8 * 1024 * 1024 };

    virtual ~Transport() {}

    // The line has no trailing newline and no trailing carriage return.
    virtual ReadStatus read_line(std::string& line) = 0;
    virtual bool write_line(const std::string& line) = 0;

protected:
    Transport() {}

private:
    Transport(const Transport&);
    Transport& operator=(const Transport&);
};

class StdioTransport : public Transport {
public:
    StdioTransport() {}

    virtual ReadStatus read_line(std::string& line);
    virtual bool write_line(const std::string& line);
};

}

#endif
