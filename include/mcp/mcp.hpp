#ifndef MCP_MCP_HPP
#define MCP_MCP_HPP

// Layers, bottom to top. Each layer includes only what is below it.
//
//   version    library version
//   error      failure codes shared by every layer
//   json       DOM, parser, serializer. No protocol knowledge.
//   rpc        JSON-RPC 2.0 codec
//   tool       tool contract and CallToolResult helpers
//   transport  newline framing
//   session    MCP lifecycle and method dispatch. No stdio.
//   server     tool registry and the facade most hosts use
//
// Sources under src/ are the C++98 and C++23 intersection. Optional headers
// under mcp/ext/ are compiled only by the standard they require:
//   ext/function_tool.hpp   C++11   std::function / lambda tools
//   ext/expected_tool.hpp   C++23   std::expected tools
// The umbrella header does not include ext/.

#include "mcp/version.hpp"
#include "mcp/error.hpp"
#include "mcp/json.hpp"
#include "mcp/rpc.hpp"
#include "mcp/tool.hpp"
#include "mcp/transport.hpp"
#include "mcp/session.hpp"
#include "mcp/server.hpp"

#endif
