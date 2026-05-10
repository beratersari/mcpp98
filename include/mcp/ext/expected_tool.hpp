#ifndef MCP_EXT_EXPECTED_TOOL_HPP
#define MCP_EXT_EXPECTED_TOOL_HPP

#if __cplusplus < 202302L
#error "mcp/ext/expected_tool.hpp requires C++23"
#endif

#include "mcp/tool.hpp"

#include <expected>
#include <functional>
#include <string>
#include <utility>

namespace mcp {

// Handler returns a CallToolResult, or an error string the server shows
// to the model as isError. The object must outlive the Server.
class ExpectedTool : public Tool {
public:
    typedef std::function<std::expected<Json, std::string>(const Json&)> Handler;

    ExpectedTool(std::string name, std::string description, Json schema, Handler handler)
        : name_(std::move(name))
        , description_(std::move(description))
        , schema_(std::move(schema))
        , handler_(std::move(handler))
    {
    }

    virtual const char* name() const
    {
        return name_.c_str();
    }

    virtual const char* description() const
    {
        return description_.c_str();
    }

    virtual Json input_schema() const
    {
        return schema_;
    }

    virtual void call(const Json& arguments, Json& result, Error& error)
    {
        if (!handler_) {
            error.set(Error::kInternalError, "tool has no handler");
            return;
        }
        std::expected<Json, std::string> got = handler_(arguments);
        if (!got) {
            const std::string& message = got.error();
            error.set(Error::kInternalError, message.c_str());
            return;
        }
        result = *got;
    }

private:
    std::string name_;
    std::string description_;
    Json schema_;
    Handler handler_;
};

}

#endif
