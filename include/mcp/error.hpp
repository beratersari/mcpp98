#ifndef MCP_ERROR_HPP
#define MCP_ERROR_HPP

#include <string>

namespace mcp {

// Protocol and tool failures. code 0 means success.
// JSON-RPC reserves -32700..-32600. Tool handlers may set any non-zero code;
// the session turns a tool Error into an MCP isError result, not a JSON-RPC error.
class Error {
public:
    enum Code {
        kOk = 0,
        kParseError = -32700,
        kInvalidRequest = -32600,
        kMethodNotFound = -32601,
        kInvalidParams = -32602,
        kInternalError = -32603
    };

    Error();
    Error(int code, const char* message);

    bool failed() const;
    int code() const;
    const std::string& message() const;

    void clear();
    void set(int code, const char* message);

private:
    int code_;
    std::string message_;
};

}

#endif
