#include "harness.hpp"

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

int test_ext_23()
{
    TestContext ctx;
    ctx.failed = 0;
    ctx.suite = "ext23";

    mcp::Json schema = mcp::object_schema();
    mcp::schema_add(schema, "text", mcp::schema_string("Text to echo"), true);
    mcp::ExpectedTool tool("echo", "Echo the text argument", schema, echo);

    mcp::Server server("echo", "0.1.0");
    CHECK(server.add_tool(&tool));

    std::string out;
    bool has_response = false;
    server.handle_line(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{"
        "\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},"
        "\"clientInfo\":{\"name\":\"ext\",\"version\":\"1\"}}}",
        out,
        has_response);
    CHECK(has_response);
    server.handle_line("{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}", out, has_response);
    CHECK(!has_response);
    server.handle_line(
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{\"name\":\"echo\"}}",
        out,
        has_response);
    CHECK(has_response);

    mcp::Json response;
    mcp::Error error;
    CHECK(mcp::Json::parse(out, response, error));
    CHECK(response.find("error") == 0);
    CHECK(response.find("result")->find("isError")->as_bool());

    server.handle_line(
        "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/call\",\"params\":{"
        "\"name\":\"echo\",\"arguments\":{\"text\":\"expected\"}}}",
        out,
        has_response);
    CHECK(mcp::Json::parse(out, response, error));
    CHECK(response.find("result")->find("content")->at(0)->find("text")->as_string() == "expected");
    return ctx.failed;
}
