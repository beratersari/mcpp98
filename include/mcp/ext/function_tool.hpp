#ifndef MCP_EXT_FUNCTION_TOOL_HPP
#define MCP_EXT_FUNCTION_TOOL_HPP

#if __cplusplus < 201103L
#error "mcp/ext/function_tool.hpp requires C++11"
#endif

#include "mcp/tool.hpp"

#include <functional>
#include <string>
#include <utility>

namespace mcp {

// Type-erased tool for lambdas. The object must outlive the Server, same as Tool.
class FunctionTool : public Tool {
public:
    typedef std::function<void(const Json&, Json&, Error&)> Handler;

    FunctionTool(std::string name, std::string description, Json schema, Handler handler)
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
        handler_(arguments, result, error);
    }

private:
    std::string name_;
    std::string description_;
    Json schema_;
    Handler handler_;
};

}

#endif
