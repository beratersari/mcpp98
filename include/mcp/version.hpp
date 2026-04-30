#ifndef MCP_VERSION_HPP
#define MCP_VERSION_HPP

#define MCP_VERSION_MAJOR 0
#define MCP_VERSION_MINOR 1
#define MCP_VERSION_PATCH 0
#define MCP_VERSION_STRING "0.1.0"

namespace mcp {

inline const char* version()
{
    return MCP_VERSION_STRING;
}

}

#endif
