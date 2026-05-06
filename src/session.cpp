#include "mcp/session.hpp"

#include "mcp/tool.hpp"

namespace mcp {
namespace {

const char* kSupportedVersions[] = {
    "2024-11-05",
    "2025-03-26",
    "2025-06-18",
    "2025-11-25",
    0
};

const char* kLatestProtocolVersion = "2025-11-25";

bool is_blank(const std::string& line)
{
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (c != ' ' && c != '\t' && c != '\r') {
            return false;
        }
    }
    return true;
}

bool version_supported(const std::string& version)
{
    for (int i = 0; kSupportedVersions[i] != 0; ++i) {
        if (version == kSupportedVersions[i]) {
            return true;
        }
    }
    return false;
}

bool require_object_params(const RpcMessage& message, Error& error, const char* method)
{
    if (!message.has_params() || !message.params().is_object()) {
        std::string text = method;
        text.append(" params must be an object");
        error.set(Error::kInvalidParams, text.c_str());
        return false;
    }
    return true;
}

const Json* object_member(const Json& object, const char* key, Error& error, const char* message)
{
    const Json* value = object.find(key);
    if (value == 0 || !value->is_object()) {
        error.set(Error::kInvalidParams, message);
        return 0;
    }
    return value;
}

const Json* string_member(const Json& object, const char* key, Error& error, const char* message)
{
    const Json* value = object.find(key);
    if (value == 0 || !value->is_string()) {
        error.set(Error::kInvalidParams, message);
        return 0;
    }
    return value;
}

}

Session::Session(ServerContext& context)
    : context_(context)
    , phase_(kWaitingInitialize)
    , protocol_version_()
{
}

bool Session::is_ready() const
{
    return phase_ == kReady;
}

const std::string& Session::protocol_version() const
{
    return protocol_version_;
}

void Session::handle_line(const std::string& line, std::string& response, bool& has_response)
{
    has_response = false;
    response.clear();
    if (is_blank(line)) {
        return;
    }

    Json value;
    Error error;
    if (!Json::parse(line, value, error)) {
        response = RpcMessage::error_message(Json::null(), error).dump();
        has_response = true;
        return;
    }

    RpcMessage message;
    if (!RpcMessage::decode(value, message, error)) {
        if (!message.reply_expected()) {
            return;
        }
        Json id = Json::null();
        if (message.has_id()) {
            id = message.id();
        }
        response = RpcMessage::error_message(id, error).dump();
        has_response = true;
        return;
    }

    if (message.kind() == RpcMessage::kNotification) {
        if (message.method() == "notifications/initialized") {
            handle_initialized_notification();
        }
        return;
    }

    if (message.kind() != RpcMessage::kRequest) {
        return;
    }

    Json result;
    Error request_error;
    dispatch_request(message, result, request_error);
    if (request_error.failed()) {
        response = RpcMessage::error_message(message.id(), request_error).dump();
    } else {
        response = RpcMessage::result_message(message.id(), result).dump();
    }
    has_response = true;
}

void Session::dispatch_request(const RpcMessage& message, Json& result, Error& error)
{
    if (message.method() == "ping") {
        if (message.has_params() && !message.params().is_object() && !message.params().is_array()) {
            error.set(Error::kInvalidRequest, "params must be an object or array");
            return;
        }
        result = Json::object();
        return;
    }

    if (message.method() == "initialize") {
        handle_initialize(message, result, error);
        return;
    }

    if (phase_ != kReady) {
        error.set(Error::kInvalidRequest, "server not initialized");
        return;
    }

    if (message.method() == "tools/list") {
        handle_tools_list(message, result, error);
        return;
    }

    if (message.method() == "tools/call") {
        handle_tools_call(message, result, error);
        return;
    }

    std::string name = message.method();
    if (name.size() > 128) {
        name.resize(128);
    }
    std::string text = "method not found: ";
    text.append(name);
    error.set(Error::kMethodNotFound, text.c_str());
}

void Session::handle_initialize(const RpcMessage& message, Json& result, Error& error)
{
    if (phase_ != kWaitingInitialize) {
        error.set(Error::kInvalidRequest, "initialize already completed");
        return;
    }
    if (!require_object_params(message, error, "initialize")) {
        return;
    }
    const Json& params = message.params();
    const Json* protocol = string_member(params, "protocolVersion", error, "protocolVersion must be a string");
    if (protocol == 0) {
        return;
    }
    if (object_member(params, "capabilities", error, "capabilities must be an object") == 0) {
        return;
    }
    const Json* client_info = object_member(params, "clientInfo", error, "clientInfo must be an object");
    if (client_info == 0) {
        return;
    }
    if (string_member(*client_info, "name", error, "clientInfo.name must be a string") == 0) {
        return;
    }
    if (string_member(*client_info, "version", error, "clientInfo.version must be a string") == 0) {
        return;
    }

    const std::string requested = protocol->as_string();
    const char* selected = version_supported(requested) ? requested.c_str() : kLatestProtocolVersion;

    Json tools_capability = Json::object();
    tools_capability.set("listChanged", Json::boolean(false));
    Json capabilities = Json::object();
    capabilities.set("tools", tools_capability);

    Json info = Json::object();
    info.set("name", Json::string(context_.server_name()));
    info.set("version", Json::string(context_.server_version()));

    result = Json::object();
    result.set("protocolVersion", Json::string(selected));
    result.set("capabilities", capabilities);
    result.set("serverInfo", info);
    if (!context_.instructions().empty()) {
        result.set("instructions", Json::string(context_.instructions()));
    }

    protocol_version_ = selected;
    phase_ = kWaitingInitialized;
}

void Session::handle_initialized_notification()
{
    if (phase_ == kWaitingInitialized || phase_ == kReady) {
        phase_ = kReady;
    }
}

void Session::handle_tools_list(const RpcMessage& message, Json& result, Error& error)
{
    if (message.has_params() && !message.params().is_object()) {
        error.set(Error::kInvalidParams, "tools/list params must be an object");
        return;
    }
    Json tools;
    context_.list_tools(tools);
    result = Json::object();
    result.set("tools", tools);
}

void Session::handle_tools_call(const RpcMessage& message, Json& result, Error& error)
{
    if (!require_object_params(message, error, "tools/call")) {
        return;
    }
    const Json& params = message.params();
    const Json* name = string_member(params, "name", error, "tools/call requires a tool name");
    if (name == 0) {
        return;
    }
    if (name->as_string().empty()) {
        error.set(Error::kInvalidParams, "tools/call requires a tool name");
        return;
    }

    Json empty_arguments = Json::object();
    const Json* arguments = params.find("arguments");
    if (arguments == 0) {
        arguments = &empty_arguments;
    } else if (!arguments->is_object()) {
        error.set(Error::kInvalidParams, "tools/call arguments must be an object");
        return;
    }

    Json tool_result;
    Error tool_error;
    if (!context_.call_tool(name->as_string(), *arguments, tool_result, tool_error)) {
        error = tool_error;
        if (!error.failed()) {
            error.set(Error::kInvalidParams, "unknown tool");
        }
        return;
    }
    if (tool_error.failed()) {
        std::string text = tool_error.message();
        if (text.empty()) {
            text = "tool failed";
        }
        result = error_result(text);
        return;
    }
    if (tool_result.is_null()) {
        result = text_result("");
        return;
    }
    if (!tool_result.is_object()) {
        result = text_result(tool_result.dump());
        return;
    }
    result = tool_result;
}

}
