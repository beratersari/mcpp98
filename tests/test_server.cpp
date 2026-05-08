#include "harness.hpp"

#include "mcp/server.hpp"
#include "mcp/version.hpp"

#include <string>
#include <vector>

namespace {

class ScriptTransport : public mcp::Transport {
public:
    ScriptTransport()
        : index_(0)
        , fail_write_(false)
    {
    }

    virtual ReadStatus read_line(std::string& line)
    {
        if (index_ >= inbound.size()) {
            return kReadEof;
        }
        line = inbound[index_];
        ++index_;
        return kReadLine;
    }

    virtual bool write_line(const std::string& line)
    {
        if (fail_write_) {
            return false;
        }
        outbound.push_back(line);
        return true;
    }

    std::vector<std::string> inbound;
    std::vector<std::string> outbound;
    std::size_t index_;
    bool fail_write_;
};

class TooLongTransport : public mcp::Transport {
public:
    TooLongTransport()
        : reads_(0)
    {
    }

    virtual ReadStatus read_line(std::string& line)
    {
        line.clear();
        if (reads_ == 0) {
            ++reads_;
            return kReadTooLong;
        }
        return kReadEof;
    }

    virtual bool write_line(const std::string& line)
    {
        outbound.push_back(line);
        return true;
    }

    std::vector<std::string> outbound;
    int reads_;
};

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

}

int test_server()
{
    TestContext ctx;
    ctx.failed = 0;
    ctx.suite = "server";

    CHECK(std::string(mcp::version()) == "0.1.0");

    EchoTool echo;
    mcp::Server server("echo", "0.1.0");
    CHECK(server.add_tool(&echo));

    ScriptTransport transport;
    transport.inbound.push_back(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{"
        "\"protocolVersion\":\"2024-11-05\",\"capabilities\":{},"
        "\"clientInfo\":{\"name\":\"script\",\"version\":\"1\"}}}");
    transport.inbound.push_back("{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}");
    transport.inbound.push_back(
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{"
        "\"name\":\"echo\",\"arguments\":{\"text\":\"merhaba\"}}}");

    CHECK(server.run(transport) == 0);
    CHECK(transport.outbound.size() == 2);

    mcp::Json init_response;
    mcp::Error error;
    CHECK(mcp::Json::parse(transport.outbound[0], init_response, error));
    CHECK(init_response.find("result")->find("protocolVersion")->as_string() == "2024-11-05");

    mcp::Json call_response;
    CHECK(mcp::Json::parse(transport.outbound[1], call_response, error));
    CHECK(call_response.find("result")->find("content")->at(0)->find("text")->as_string() == "merhaba");

    TooLongTransport too_long;
    CHECK(server.run(too_long) == 0);
    CHECK(too_long.outbound.size() == 1);
    mcp::Json limited;
    CHECK(mcp::Json::parse(too_long.outbound[0], limited, error));
    CHECK(limited.find("error")->find("code")->as_number() == -32700.0);

    ScriptTransport broken;
    broken.fail_write_ = true;
    broken.inbound.push_back("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}");
    CHECK(server.run(broken) == 1);

    return ctx.failed;
}
