#include "mcp/server.hpp"

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

}

int main()
{
    mcp::Server server("echo", "0.1.0");
    EchoTool echo;
    if (!server.add_tool(&echo)) {
        return 1;
    }
    return server.run_stdio();
}
