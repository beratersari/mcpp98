#include "harness.hpp"

#include "mcp/server.hpp"

#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

class EchoTool : public mcp::Tool {
public:
    virtual const char* name() const
    {
        return "echo";
    }

    virtual const char* description() const
    {
        return "Echo the text argument";
    }

    virtual mcp::Json input_schema() const
    {
        mcp::Json schema = mcp::object_schema();
        mcp::schema_add(schema, "text", mcp::schema_string("Text to echo"), true);
        return schema;
    }

    virtual void call(const mcp::Json& arguments, mcp::Json& result, mcp::Error& error)
    {
        const mcp::Json* text = arguments.find("text");
        if (text == 0 || !text->is_string()) {
            error.set(mcp::Error::kInvalidParams, "missing string argument \"text\"");
            return;
        }
        result = mcp::text_result(text->as_string());
    }
};

class FailTool : public mcp::Tool {
public:
    virtual const char* name() const
    {
        return "fail";
    }

    virtual const char* description() const
    {
        return "Always fails";
    }

    virtual mcp::Json input_schema() const
    {
        return mcp::object_schema();
    }

    virtual void call(const mcp::Json&, mcp::Json&, mcp::Error& error)
    {
        error.set(mcp::Error::kInternalError, "nope");
    }
};

#if defined(__EXCEPTIONS)
class BoomTool : public mcp::Tool {
public:
    virtual const char* name() const
    {
        return "boom";
    }

    virtual const char* description() const
    {
        return "Throws";
    }

    virtual mcp::Json input_schema() const
    {
        return mcp::object_schema();
    }

    virtual void call(const mcp::Json&, mcp::Json&, mcp::Error&)
    {
        throw std::runtime_error("boom");
    }
};
#endif

const char* kInitialize =
    "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{"
    "\"protocolVersion\":\"2025-11-25\",\"capabilities\":{},"
    "\"clientInfo\":{\"name\":\"test\",\"version\":\"0\"}}}";

const char* kInitialized =
    "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}";

bool exchange(mcp::Server& server, const char* line, mcp::Json& response, bool expect_response)
{
    std::string out;
    bool has_response = false;
    server.handle_line(line, out, has_response);
    if (has_response != expect_response) {
        return false;
    }
    if (!has_response) {
        response = mcp::Json();
        return true;
    }
    mcp::Error error;
    return mcp::Json::parse(out, response, error);
}

void ready(mcp::Server& server, TestContext& ctx)
{
    mcp::Json response;
    CHECK(exchange(server, kInitialize, response, true));
    CHECK(exchange(server, kInitialized, response, false));
}

const mcp::Json* member(const mcp::Json& value, const char* key)
{
    return value.find(key);
}

}

int test_session()
{
    TestContext ctx;
    ctx.failed = 0;
    ctx.suite = "session";

    mcp::Server server("echo", "0.1.0");
    server.set_instructions("say it back");
    EchoTool echo;
    FailTool fail;
    CHECK(server.add_tool(&echo));
    CHECK(server.add_tool(&fail));
    CHECK(!server.add_tool(&echo));
    CHECK(!server.add_tool(0));

    mcp::Json response;
    CHECK(exchange(server, "   ", response, false));
    CHECK(exchange(server,
                   "{\"jsonrpc\":\"2.0\",\"id\":7,\"method\":\"tools/list\"}",
                   response,
                   true));
    CHECK(member(response, "error") != 0);
    CHECK(member(response, "error")->find("code")->as_number() == -32600.0);

    CHECK(exchange(server, "{\"jsonrpc\":\"2.0\",\"id\":8,\"method\":\"ping\"}", response, true));
    CHECK(member(response, "result") != 0);
    CHECK(member(response, "result")->is_object());
    CHECK(member(response, "id")->as_number() == 8.0);

    CHECK(exchange(server, kInitialize, response, true));
    const mcp::Json* result = member(response, "result");
    CHECK(result != 0);
    CHECK(result->find("protocolVersion")->as_string() == "2025-11-25");
    CHECK(result->find("serverInfo")->find("name")->as_string() == "echo");
    CHECK(result->find("capabilities")->find("tools")->find("listChanged")->as_bool() == false);
    CHECK(result->find("instructions")->as_string() == "say it back");
    CHECK(member(response, "id")->dump() == "1");

    CHECK(exchange(server, kInitialize, response, true));
    CHECK(member(response, "error") != 0);

    CHECK(exchange(server, kInitialized, response, false));

    CHECK(exchange(server, "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\"}", response, true));
    const mcp::Json* tools = member(response, "result")->find("tools");
    CHECK(tools != 0);
    CHECK(tools->size() == 2);
    CHECK(tools->at(0)->find("name")->as_string() == "echo");
    CHECK(tools->at(0)->find("inputSchema")->find("type")->as_string() == "object");

    CHECK(exchange(server,
                   "{\"jsonrpc\":\"2.0\",\"id\":\"abc\",\"method\":\"tools/call\",\"params\":{"
                   "\"name\":\"echo\",\"arguments\":{\"text\":\"merhaba\"}}}",
                   response,
                   true));
    CHECK(member(response, "error") == 0);
    CHECK(member(response, "id")->as_string() == "abc");
    const mcp::Json* content = member(response, "result")->find("content");
    CHECK(content != 0);
    CHECK(content->at(0)->find("text")->as_string() == "merhaba");

    CHECK(exchange(server,
                   "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/call\",\"params\":{\"name\":\"missing\"}}",
                   response,
                   true));
    CHECK(member(response, "error")->find("code")->as_number() == -32602.0);

    CHECK(exchange(server,
                   "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tools/call\",\"params\":{\"name\":\"fail\"}}",
                   response,
                   true));
    CHECK(member(response, "error") == 0);
    CHECK(member(response, "result")->find("isError")->as_bool());
    CHECK(member(response, "result")->find("content")->at(0)->find("text")->as_string() == "nope");

    CHECK(exchange(server, "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/cancelled\"}", response, false));
    CHECK(exchange(server, "{", response, true));
    CHECK(member(response, "id")->is_null());
    CHECK(member(response, "error")->find("code")->as_number() == -32700.0);

    CHECK(exchange(server, "{\"jsonrpc\":\"2.0\",\"method\":\"nope\"}", response, false));
    CHECK(exchange(server, "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"nope\"}", response, true));
    CHECK(member(response, "error")->find("code")->as_number() == -32601.0);

    CHECK(exchange(server,
                   "{\"jsonrpc\":\"2.0\",\"id\":9,\"result\":{}}",
                   response,
                   false));

    mcp::Server old_client("old", "0");
    EchoTool echo2;
    CHECK(old_client.add_tool(&echo2));
    CHECK(exchange(old_client,
                   "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{"
                   "\"protocolVersion\":\"1999-01-01\",\"capabilities\":{},"
                   "\"clientInfo\":{\"name\":\"c\",\"version\":\"0\"}}}",
                   response,
                   true));
    CHECK(member(response, "result")->find("protocolVersion")->as_string() == "2025-11-25");

    mcp::Server broken("broken", "0");
    CHECK(exchange(broken,
                   "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{"
                   "\"protocolVersion\":\"2025-11-25\",\"capabilities\":{}}}",
                   response,
                   true));
    CHECK(member(response, "error") != 0);
    CHECK(exchange(broken, kInitialized, response, false));
    CHECK(exchange(broken, "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"ping\"}", response, true));
    CHECK(exchange(broken, "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/list\"}", response, true));
    CHECK(member(response, "error") != 0);

#if defined(__EXCEPTIONS)
    mcp::Server throwing("throw", "0");
    BoomTool boom;
    CHECK(throwing.add_tool(&boom));
    ready(throwing, ctx);
    CHECK(exchange(throwing,
                   "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\",\"params\":{\"name\":\"boom\"}}",
                   response,
                   true));
    CHECK(member(response, "error") == 0);
    CHECK(member(response, "result")->find("isError")->as_bool());
    CHECK(member(response, "result")->find("content")->at(0)->find("text")->as_string() == "boom");
#endif

    return ctx.failed;
}
