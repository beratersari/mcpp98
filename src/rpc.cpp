#include "mcp/rpc.hpp"

namespace mcp {
namespace {

bool id_type_ok(const Json& id)
{
    return id.is_string() || id.is_number() || id.is_null();
}

}

RpcMessage::RpcMessage()
    : kind_(kInvalid)
    , method_()
    , has_id_(false)
    , id_()
    , has_params_(false)
    , params_()
    , has_result_(false)
    , result_()
    , reply_expected_(false)
{
}

RpcMessage::Kind RpcMessage::kind() const
{
    return kind_;
}

const std::string& RpcMessage::method() const
{
    return method_;
}

bool RpcMessage::has_id() const
{
    return has_id_;
}

const Json& RpcMessage::id() const
{
    return id_;
}

bool RpcMessage::has_params() const
{
    return has_params_;
}

const Json& RpcMessage::params() const
{
    return params_;
}

bool RpcMessage::has_result() const
{
    return has_result_;
}

const Json& RpcMessage::result() const
{
    return result_;
}

bool RpcMessage::reply_expected() const
{
    return reply_expected_;
}

bool RpcMessage::decode(const Json& value, RpcMessage& out, Error& error)
{
    out = RpcMessage();
    error.clear();

    if (!value.is_object()) {
        error.set(Error::kInvalidRequest, "request must be a JSON object");
        out.reply_expected_ = true;
        out.id_ = Json::null();
        return false;
    }

    const Json* jsonrpc = value.find("jsonrpc");
    const Json* method = value.find("method");
    const Json* id = value.find("id");
    const Json* params = value.find("params");
    const Json* result = value.find("result");
    const Json* rpc_error = value.find("error");

    const bool jsonrpc_ok = jsonrpc != 0 && jsonrpc->is_string() && jsonrpc->as_string() == "2.0";
    const bool method_is_string = method != 0 && method->is_string();
    const bool has_id = id != 0;
    const bool id_ok = !has_id || id_type_ok(*id);
    const bool has_params = params != 0;
    const bool params_ok = !has_params || params->is_object() || params->is_array();

    if (has_id && id_ok) {
        out.has_id_ = true;
        out.id_ = *id;
    } else {
        out.id_ = Json::null();
    }
    if (method_is_string) {
        out.method_ = method->as_string();
    }
    if (has_params && params_ok) {
        out.has_params_ = true;
        out.params_ = *params;
    }

    // Notification: string method, no id. Never reply, even when the rest is wrong.
    if (method_is_string && !has_id) {
        out.reply_expected_ = false;
        if (!jsonrpc_ok || !params_ok) {
            error.set(Error::kInvalidRequest, "invalid notification");
            out.kind_ = kInvalid;
            return false;
        }
        out.kind_ = kNotification;
        return true;
    }

    if (!jsonrpc_ok) {
        error.set(Error::kInvalidRequest, "jsonrpc must be \"2.0\"");
        out.reply_expected_ = true;
        out.kind_ = kInvalid;
        return false;
    }

    if (method != 0) {
        if (!method_is_string) {
            error.set(Error::kInvalidRequest, "method must be a string");
            out.reply_expected_ = true;
            out.kind_ = kInvalid;
            return false;
        }
        if (!id_ok) {
            error.set(Error::kInvalidRequest, "id must be a string, number, or null");
            out.reply_expected_ = true;
            out.has_id_ = true;
            out.id_ = Json::null();
            out.kind_ = kInvalid;
            return false;
        }
        if (!params_ok) {
            error.set(Error::kInvalidRequest, "params must be an object or array");
            out.reply_expected_ = true;
            out.kind_ = kInvalid;
            return false;
        }
        out.kind_ = kRequest;
        out.reply_expected_ = true;
        out.has_id_ = true;
        return true;
    }

    if (result != 0 && rpc_error != 0) {
        error.set(Error::kInvalidRequest, "response has both result and error");
        out.reply_expected_ = has_id;
        out.kind_ = kInvalid;
        return false;
    }
    if (result != 0 || rpc_error != 0) {
        out.kind_ = kResponse;
        out.reply_expected_ = false;
        if (result != 0) {
            out.has_result_ = true;
            out.result_ = *result;
        }
        return true;
    }

    error.set(Error::kInvalidRequest, "message is missing method");
    out.reply_expected_ = true;
    out.kind_ = kInvalid;
    return false;
}

Json RpcMessage::result_message(const Json& id, const Json& result)
{
    Json message = Json::object();
    message.set("jsonrpc", Json::string("2.0"));
    message.set("id", id);
    message.set("result", result);
    return message;
}

Json RpcMessage::error_message(const Json& id, const Error& error)
{
    std::string text = error.message();
    if (text.empty()) {
        text = "error";
    }
    Json body = Json::object();
    body.set("code", Json::number(static_cast<double>(error.code())));
    body.set("message", Json::string(text));

    Json message = Json::object();
    message.set("jsonrpc", Json::string("2.0"));
    message.set("id", id);
    message.set("error", body);
    return message;
}

}
