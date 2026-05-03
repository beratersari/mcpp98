#ifndef MCP_RPC_HPP
#define MCP_RPC_HPP

#include "mcp/error.hpp"
#include "mcp/json.hpp"

#include <string>

namespace mcp {

// One JSON-RPC 2.0 message. MCP stdio carries a single message per line,
// so batches are rejected by the decoder (a JSON array is not an object).
class RpcMessage {
public:
    enum Kind {
        kInvalid,
        kRequest,
        kNotification,
        kResponse
    };

    RpcMessage();

    Kind kind() const;
    const std::string& method() const;
    bool has_id() const;
    const Json& id() const;
    bool has_params() const;
    const Json& params() const;
    bool has_result() const;
    const Json& result() const;
    bool reply_expected() const;

    // On failure, kind is kInvalid, error is set, and reply_expected says
    // whether the peer is waiting for an error object.
    static bool decode(const Json& value, RpcMessage& out, Error& error);

    static Json result_message(const Json& id, const Json& result);
    static Json error_message(const Json& id, const Error& error);

private:
    Kind kind_;
    std::string method_;
    bool has_id_;
    Json id_;
    bool has_params_;
    Json params_;
    bool has_result_;
    Json result_;
    bool reply_expected_;
};

}

#endif
