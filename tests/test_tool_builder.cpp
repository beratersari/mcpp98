#include "harness.hpp"

#include "mcp/ext/tool_builder.hpp"
#include "mcp/server.hpp"

#include <sstream>
#include <string>

namespace {

void exchange(mcp::Server& server, const char* line, std::string& out, bool& has_response)
{
    server.handle_line(line, out, has_response);
}

bool ready(mcp::Server& server, TestContext& ctx)
{
    std::string out;
    bool has_response = false;
    exchange(
        server,
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{"
        "\"protocolVersion\":\"2025-03-26\",\"capabilities\":{},"
        "\"clientInfo\":{\"name\":\"builder\",\"version\":\"1\"}}}",
        out,
        has_response);
    CHECK(has_response);
    exchange(server, "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}", out, has_response);
    CHECK(!has_response);
    return ctx.failed == 0;
}

const mcp::Json* tool_text(const mcp::Json& response)
{
    const mcp::Json* result = response.find("result");
    if (result == 0 || result->find("content") == 0 || result->find("content")->at(0) == 0) {
        return 0;
    }
    return result->find("content")->at(0)->find("text");
}

}

int test_tool_builder()
{
    TestContext ctx;
    ctx.failed = 0;
    ctx.suite = "tool_builder";

    int hits = 0;
    auto echo = mcp::tool("echo", "Echo the text argument")
        .string("text", "Text to echo")
        .number("repeat", "Repeat count", false)
        .boolean("upper", "Uppercase the text", false)
        .handle([&hits](const mcp::Args& in) -> mcp::Reply {
            ++hits;
            if (in.string("text") == "fail") {
                return mcp::fail("nope");
            }
            const bool upper = in.boolean("upper", true);
            const int repeat = static_cast<int>(in.number("repeat", 1.0));
            std::ostringstream out;
            out << in.string("text") << ":" << repeat << ":" << (upper ? "U" : "u")
                << ":" << (in.has("repeat") ? "Y" : "N");
            return out.str();
        });
    auto ping = mcp::tool("ping", "Liveness check")
        .handle([](const mcp::Args&) {
            return "ok";
        });

    mcp::Server server("echo", "0.1.0");
    CHECK(server.add_tool(&echo));
    CHECK(server.add_tool(&ping));
    if (!ready(server, ctx)) {
        return ctx.failed;
    }

    std::string out;
    bool has_response = false;
    exchange(
        server,
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\"}",
        out,
        has_response);
    CHECK(has_response);
    mcp::Json response;
    mcp::Error error;
    CHECK(mcp::Json::parse(out, response, error));
    const mcp::Json* tools = response.find("result") == 0 ? 0 : response.find("result")->find("tools");
    CHECK(tools != 0 && tools->size() == 2);
    CHECK(tools->at(0)->find("name")->as_string() == "echo");
    CHECK(tools->at(0)->find("description")->as_string() == "Echo the text argument");
    const mcp::Json* schema = tools->at(0)->find("inputSchema");
    CHECK(schema != 0);
    CHECK(schema->find("properties")->find("text")->find("description")->as_string() == "Text to echo");
    CHECK(schema->find("properties")->find("repeat")->find("description")->as_string() == "Repeat count");
    CHECK(schema->find("properties")->find("upper")->find("type")->as_string() == "boolean");
    CHECK(schema->find("required") != 0);
    CHECK(schema->find("required")->size() == 1);
    CHECK(schema->find("required")->at(0)->as_string() == "text");
    CHECK(tools->at(1)->find("name")->as_string() == "ping");

    exchange(
        server,
        "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/call\",\"params\":{"
        "\"name\":\"echo\",\"arguments\":{\"text\":\"lambda\"}}}",
        out,
        has_response);
    CHECK(has_response);
    CHECK(mcp::Json::parse(out, response, error));
    CHECK(response.find("error") == 0);
    CHECK(tool_text(response) != 0 && tool_text(response)->as_string() == "lambda:1:U:N");
    CHECK(hits == 1);

    exchange(
        server,
        "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tools/call\",\"params\":{"
        "\"name\":\"echo\",\"arguments\":{\"text\":\"lambda\",\"repeat\":2,\"upper\":false}}}",
        out,
        has_response);
    CHECK(mcp::Json::parse(out, response, error));
    CHECK(tool_text(response) != 0 && tool_text(response)->as_string() == "lambda:2:u:Y");
    CHECK(hits == 2);

    exchange(
        server,
        "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"tools/call\",\"params\":{"
        "\"name\":\"echo\",\"arguments\":{}}}",
        out,
        has_response);
    CHECK(mcp::Json::parse(out, response, error));
    CHECK(response.find("error") == 0);
    CHECK(response.find("result")->find("isError")->as_bool());
    CHECK(tool_text(response) != 0 && tool_text(response)->as_string() == "missing string argument \"text\"");
    CHECK(hits == 2);

    exchange(
        server,
        "{\"jsonrpc\":\"2.0\",\"id\":6,\"method\":\"tools/call\",\"params\":{"
        "\"name\":\"echo\",\"arguments\":{\"text\":\"lambda\",\"repeat\":\"nope\"}}}",
        out,
        has_response);
    CHECK(mcp::Json::parse(out, response, error));
    CHECK(response.find("result")->find("isError")->as_bool());
    CHECK(tool_text(response) != 0 && tool_text(response)->as_string() == "argument \"repeat\" must be a number");
    CHECK(hits == 2);

    exchange(
        server,
        "{\"jsonrpc\":\"2.0\",\"id\":7,\"method\":\"tools/call\",\"params\":{"
        "\"name\":\"echo\",\"arguments\":{\"text\":\"fail\"}}}",
        out,
        has_response);
    CHECK(mcp::Json::parse(out, response, error));
    CHECK(response.find("result")->find("isError")->as_bool());
    CHECK(tool_text(response) != 0 && tool_text(response)->as_string() == "nope");
    CHECK(hits == 3);

    exchange(
        server,
        "{\"jsonrpc\":\"2.0\",\"id\":8,\"method\":\"tools/call\",\"params\":{\"name\":\"ping\"}}",
        out,
        has_response);
    CHECK(mcp::Json::parse(out, response, error));
    CHECK(response.find("error") == 0);
    CHECK(tool_text(response) != 0 && tool_text(response)->as_string() == "ok");

    return ctx.failed;
}
