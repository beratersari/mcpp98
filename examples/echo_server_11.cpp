#include "mcp/ext/function_tool.hpp"
#include "mcp/server.hpp"

int main()
{
    mcp::Json schema = mcp::object_schema();
    mcp::schema_add(schema, "text", mcp::schema_string("Text to echo"), true);

    mcp::FunctionTool echo(
        "echo",
        "Echo the text argument",
        schema,
        [](const mcp::Json& arguments, mcp::Json& result, mcp::Error& error) {
            const mcp::Json* text = arguments.find("text");
            if (text == 0 || !text->is_string()) {
                error.set(mcp::Error::kInvalidParams, "missing string argument \"text\"");
                return;
            }
            result = mcp::text_result(text->as_string());
        });

    mcp::Server server("echo", "0.1.0");
    if (!server.add_tool(&echo)) {
        return 1;
    }
    return server.run_stdio();
}
