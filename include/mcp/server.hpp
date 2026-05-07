#ifndef MCP_SERVER_HPP
#define MCP_SERVER_HPP

#include "mcp/tool.hpp"
#include "mcp/transport.hpp"

#include <string>

namespace mcp {

// Facade. Owns the session and the tool table. Does not own Tool objects.
class Server {
public:
    Server(const char* name, const char* version);
    ~Server();

    void set_instructions(const char* text);

    // false when tool is null, the name is empty, or the name is already registered.
    bool add_tool(Tool* tool);

    void handle_line(const std::string& line, std::string& response, bool& has_response);

    // 0 on clean EOF. 1 when a read or write fails.
    int run(Transport& transport);
    int run_stdio();

private:
    Server(const Server&);
    Server& operator=(const Server&);

    struct Impl;
    Impl* impl_;
};

}

#endif
