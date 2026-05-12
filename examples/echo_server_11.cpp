#include "mcp/ext/tool_builder.hpp"
#include "mcp/server.hpp"

int main()
{
    auto echo = mcp::tool("echo", "Echo the text argument")
        .string("text", "Text to echo")
        .handle([](const mcp::Args& in) {
            return in.string("text");
        });

    mcp::Server server("echo", "0.1.0");
    if (!server.add_tool(&echo)) {
        return 1;
    }
    return server.run_stdio();
}
