#ifndef MCP_EXT_TOOL_BUILDER_HPP
#define MCP_EXT_TOOL_BUILDER_HPP

#if __cplusplus < 201103L
#error "mcp/ext/tool_builder.hpp requires C++11"
#endif

#include "mcp/tool.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace mcp {

class Args;

// Text the model reads, or a failure made with fail().
// A handler that returns both needs a trailing return type of Reply.
class Reply {
public:
    Reply(const char* text)
        : failed_(false)
        , text_(text == 0 ? std::string() : std::string(text))
    {
    }

    Reply(const std::string& text)
        : failed_(false)
        , text_(text)
    {
    }

    static Reply failure(const char* message)
    {
        Reply reply(message == 0 || message[0] == '\0' ? "tool failed" : message);
        reply.failed_ = true;
        return reply;
    }

    static Reply failure(const std::string& message)
    {
        return failure(message.c_str());
    }

    bool failed() const
    {
        return failed_;
    }

    const std::string& text() const
    {
        return text_;
    }

private:
    bool failed_;
    std::string text_;
};

inline Reply fail(const char* message)
{
    return Reply::failure(message);
}

inline Reply fail(const std::string& message)
{
    return Reply::failure(message);
}

// View of the object the model sent. Valid only for the duration of the handler.
// Required arguments are checked before the handler runs.
class Args {
public:
    explicit Args(const Json& arguments)
        : arguments_(arguments)
    {
    }

    bool has(const char* name) const
    {
        return find(name) != 0;
    }

    // fallback is used when the argument was omitted. Pass 0 to get an empty string.
    std::string string(const char* name, const char* fallback = 0) const
    {
        const Json* value = find(name);
        if (value == 0 || !value->is_string()) {
            return fallback == 0 ? std::string() : std::string(fallback);
        }
        return value->as_string();
    }

    double number(const char* name, double fallback = 0.0) const
    {
        const Json* value = find(name);
        if (value == 0 || !value->is_number()) {
            return fallback;
        }
        return value->as_number();
    }

    bool boolean(const char* name, bool fallback = false) const
    {
        const Json* value = find(name);
        if (value == 0 || !value->is_bool()) {
            return fallback;
        }
        return value->as_bool();
    }

private:
    const Json* find(const char* name) const
    {
        if (name == 0 || !arguments_.is_object()) {
            return 0;
        }
        return arguments_.find(name);
    }

    const Json& arguments_;
};

namespace tool_detail {

enum ArgType {
    kString,
    kNumber,
    kBoolean
};

struct ArgSpec {
    ArgType type;
    std::string name;
    std::string description;
    bool required;
};

inline const char* type_name(ArgType type)
{
    if (type == kString) {
        return "string";
    }
    if (type == kNumber) {
        return "number";
    }
    return "boolean";
}

inline bool type_matches(ArgType type, const Json& value)
{
    if (type == kString) {
        return value.is_string();
    }
    if (type == kNumber) {
        return value.is_number();
    }
    return value.is_bool();
}

inline Json property_schema(const ArgSpec& spec)
{
    const char* description = spec.description.c_str();
    if (spec.type == kString) {
        return schema_string(description);
    }
    if (spec.type == kNumber) {
        return schema_number(description);
    }
    return schema_boolean(description);
}

inline Json make_schema(const std::vector<ArgSpec>& params)
{
    Json schema = object_schema();
    for (std::size_t i = 0; i < params.size(); ++i) {
        schema_add(schema, params[i].name.c_str(), property_schema(params[i]), params[i].required);
    }
    return schema;
}

inline std::string missing_message(const ArgSpec& spec)
{
    std::string message = "missing ";
    message += type_name(spec.type);
    message += " argument \"";
    message += spec.name;
    message += "\"";
    return message;
}

inline std::string type_message(const ArgSpec& spec)
{
    std::string message = "argument \"";
    message += spec.name;
    message += "\" must be a ";
    message += type_name(spec.type);
    return message;
}

}

// Tool built by tool(). It must outlive the Server, same as Tool.
class BoundTool : public Tool {
public:
    template<typename Handler>
    BoundTool(std::string name,
              std::string description,
              std::vector<tool_detail::ArgSpec> params,
              Handler handler)
        : name_(std::move(name))
        , description_(std::move(description))
        , params_(std::move(params))
        , schema_(tool_detail::make_schema(params_))
        , handler_(handler)
    {
    }

    BoundTool(BoundTool&& other)
        : Tool()
        , name_(std::move(other.name_))
        , description_(std::move(other.description_))
        , params_(std::move(other.params_))
        , schema_(std::move(other.schema_))
        , handler_(std::move(other.handler_))
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
        if (!arguments.is_object()) {
            error.set(Error::kInvalidParams, "arguments must be an object");
            return;
        }
        for (std::size_t i = 0; i < params_.size(); ++i) {
            const tool_detail::ArgSpec& spec = params_[i];
            const Json* value = arguments.find(spec.name.c_str());
            if (value == 0) {
                if (spec.required) {
                    error.set(Error::kInvalidParams, tool_detail::missing_message(spec).c_str());
                    return;
                }
                continue;
            }
            if (!tool_detail::type_matches(spec.type, *value)) {
                error.set(Error::kInvalidParams, tool_detail::type_message(spec).c_str());
                return;
            }
        }
        Reply reply = handler_(Args(arguments));
        if (reply.failed()) {
            const std::string& message = reply.text();
            error.set(Error::kInternalError, message.empty() ? "tool failed" : message.c_str());
            return;
        }
        result = text_result(reply.text());
    }

private:
    std::string name_;
    std::string description_;
    std::vector<tool_detail::ArgSpec> params_;
    Json schema_;
    std::function<Reply(const Args&)> handler_;
};

// Name and description first, then arguments, then handle().
// string(), number(), and boolean() are required unless the third value is false.
class ToolBuilder {
public:
    ToolBuilder(std::string name, std::string description)
        : name_(std::move(name))
        , description_(std::move(description))
    {
    }

    ToolBuilder& string(const char* name, const char* description, bool required = true)
    {
        add(tool_detail::kString, name, description, required);
        return *this;
    }

    ToolBuilder& number(const char* name, const char* description, bool required = true)
    {
        add(tool_detail::kNumber, name, description, required);
        return *this;
    }

    ToolBuilder& boolean(const char* name, const char* description, bool required = true)
    {
        add(tool_detail::kBoolean, name, description, required);
        return *this;
    }

    template<typename Handler>
    BoundTool handle(Handler handler) const
    {
        return BoundTool(name_, description_, params_, handler);
    }

private:
    void add(tool_detail::ArgType type, const char* name, const char* description, bool required)
    {
        if (name == 0 || name[0] == '\0') {
            return;
        }
        for (std::size_t i = 0; i < params_.size(); ++i) {
            if (params_[i].name == name) {
                params_[i].type = type;
                params_[i].description = description == 0 ? std::string() : std::string(description);
                params_[i].required = required;
                return;
            }
        }
        tool_detail::ArgSpec spec;
        spec.type = type;
        spec.name = name;
        spec.description = description == 0 ? std::string() : std::string(description);
        spec.required = required;
        params_.push_back(spec);
    }

    std::string name_;
    std::string description_;
    std::vector<tool_detail::ArgSpec> params_;
};

inline ToolBuilder tool(const char* name, const char* description)
{
    return ToolBuilder(
        name == 0 ? std::string() : std::string(name),
        description == 0 ? std::string() : std::string(description));
}

}

#endif
