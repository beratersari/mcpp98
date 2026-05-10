#include "mcp/ext/expected_tool.hpp"
#include "mcp/server.hpp"

#include <expected>
#include <string>

namespace {

std::expected<mcp::Json, std::string> echo(const mcp::Json& arguments)
{
    const mcp::Json* text = arguments.find("text");
    if (text == 0 || !text->is_string()) {
        return std::expected<mcp::Json, std::string>(
            std::unexpect, std::string("missing string argument \"text\""));
    }
    return std::expected<mcp::Json, std::string>(mcp::text_result(text->as_string()));
}

}

int main()
{
    mcp::Json schema = mcp::object_schema();
    mcp::schema_add(schema, "text", mcp::schema_string("Text to echo"), true);

    mcp::ExpectedTool tool("echo", "Echo the text argument", schema, echo);
    mcp::Server server("echo", "0.1.0");
    if (!server.add_tool(&tool)) {
        return 1;
    }
    return server.run_stdio();
}
