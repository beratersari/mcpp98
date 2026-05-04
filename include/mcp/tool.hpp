#ifndef MCP_TOOL_HPP
#define MCP_TOOL_HPP

#include "mcp/error.hpp"
#include "mcp/json.hpp"

#include <string>

namespace mcp {

// C++98 tool. The server does not own the object; it must outlive the server.
// call() fills result with a CallToolResult (see text_result) or sets error.
// A set error becomes an MCP tool error the model can read. It does not
// become a JSON-RPC error. Unknown tools and malformed calls are protocol
// errors and are produced by the session, not by Tool.
class Tool {
public:
    virtual ~Tool() {}

    virtual const char* name() const = 0;
    virtual const char* description() const = 0;
    virtual Json input_schema() const = 0;
    virtual void call(const Json& arguments, Json& result, Error& error) = 0;

protected:
    Tool() {}

private:
    Tool(const Tool&);
    Tool& operator=(const Tool&);
};

Json object_schema();
Json schema_string(const char* description);
Json schema_number(const char* description);
Json schema_boolean(const char* description);
void schema_add(Json& schema, const char* name, const Json& property, bool required);

Json text_result(const std::string& text);
Json error_result(const std::string& text);

}

#endif
