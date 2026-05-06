#ifndef MCP_SESSION_HPP
#define MCP_SESSION_HPP

#include "mcp/error.hpp"
#include "mcp/json.hpp"
#include "mcp/rpc.hpp"

#include <string>

namespace mcp {

// What the session needs from the process that owns tools. Server implements
// this. A custom host can implement it and drive Session directly.
class ServerContext {
public:
    virtual ~ServerContext() {}

    virtual const std::string& server_name() const = 0;
    virtual const std::string& server_version() const = 0;
    virtual const std::string& instructions() const = 0;
    virtual void list_tools(Json& tools) const = 0;

    // false: tool was not found; error is a protocol error.
    // true: the tool ran. error.failed() is a tool failure, not a protocol failure.
    virtual bool call_tool(const std::string& name,
                           const Json& arguments,
                           Json& result,
                           Error& error) = 0;

protected:
    ServerContext() {}

private:
    ServerContext(const ServerContext&);
    ServerContext& operator=(const ServerContext&);
};

// MCP lifecycle over already-framed text lines. No sockets and no stdio.
// Accepted protocol versions: 2024-11-05, 2025-03-26, 2025-06-18, 2025-11-25.
// The response uses the client version when it is one of those, otherwise
// 2025-11-25. Methods before notifications/initialized: initialize and ping.
class Session {
public:
    explicit Session(ServerContext& context);

    void handle_line(const std::string& line, std::string& response, bool& has_response);

    bool is_ready() const;
    const std::string& protocol_version() const;

private:
    Session(const Session&);
    Session& operator=(const Session&);

    enum Phase {
        kWaitingInitialize,
        kWaitingInitialized,
        kReady
    };

    void dispatch_request(const RpcMessage& message, Json& result, Error& error);
    void handle_initialize(const RpcMessage& message, Json& result, Error& error);
    void handle_tools_list(const RpcMessage& message, Json& result, Error& error);
    void handle_tools_call(const RpcMessage& message, Json& result, Error& error);
    void handle_initialized_notification();

    ServerContext& context_;
    Phase phase_;
    std::string protocol_version_;
};

}

#endif
