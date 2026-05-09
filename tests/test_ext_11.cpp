#include "harness.hpp"

#include "mcp/ext/function_tool.hpp"
#include "mcp/server.hpp"

#include <string>

int test_ext_11()
{
    TestContext ctx;
    ctx.failed = 0;
    ctx.suite = "ext11";

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
    CHECK(server.add_tool(&echo));

    std::string out;
    bool has_response = false;
    server.handle_line(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{"
        "\"protocolVersion\":\"2025-03-26\",\"capabilities\":{},"
        "\"clientInfo\":{\"name\":\"ext\",\"version\":\"1\"}}}",
        out,
        has_response);
    CHECK(has_response);
    server.handle_line("{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}", out, has_response);
    CHECK(!has_response);
    server.handle_line(
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{"
        "\"name\":\"echo\",\"arguments\":{\"text\":\"lambda\"}}}",
        out,
        has_response);
    CHECK(has_response);

    mcp::Json response;
    mcp::Error error;
    CHECK(mcp::Json::parse(out, response, error));
    CHECK(response.find("result")->find("content")->at(0)->find("text")->as_string() == "lambda");
    return ctx.failed;
}
