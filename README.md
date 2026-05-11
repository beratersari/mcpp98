# mcpp98

MCP server kütüphanesi. Aynı kaynak, C++98, C++11, C++14, C++17, C++20 ve C++23 ile derlenir. C++21 ve C++24 diye bir ISO standardı yoktur; matris bu altıdır.

Çekirdek, bu standartların ortak alt kümesidir. Lambda ve `std::expected` ayrı header'dadır ve alt standartta include edilmez.

## Katmanlar

```
json       DOM, ayrıştırıcı, yazıcı. Protokol bilmez.
rpc        JSON-RPC 2.0
tool       C++98 sanal Tool ve CallToolResult yardımcıları
transport  satır çerçeveleme. stdio burada.
session    initialize, ping, tools/list, tools/call. Taşımadan habersiz.
server     tool tablosu ve dışarıdan kullanılan yüz
ext/       C++11 FunctionTool, C++23 ExpectedTool
```

Server tool nesnesinin sahibi değildir. Nesne, `run` bitene kadar yaşar. Tool içinden dönen `Error`, modele giden `isError` sonucudur. Bilinmeyen tool ve bozuk istek JSON-RPC hatasıdır.

Konuşulan protokol sürümleri: `2024-11-05`, `2025-03-26`, `2025-06-18`, `2025-11-25`. İstemcininki bunlardansa aynısı döner, değilse `2025-11-25`. `notifications/initialized` gelmeden yalnız `initialize` ve `ping` kabul edilir.

`stdout`'a yalnız MCP satırı yazılır. Log `stderr`'e aittir.

## Derleme

Varsayılan standart C++98'dir.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Bütün standartlar:

```bash
./scripts/build-matrix.sh
```

C++11 örneği `echo_server_11`, C++23 örneği `echo_server_23` yalnız o standartla (ve üstüyle) derlenir.

```cpp
#include "mcp/server.hpp"

class EchoTool : public mcp::Tool {
public:
    virtual const char* name() const { return "echo"; }
    virtual const char* description() const { return "Echo the text argument"; }
    virtual mcp::Json input_schema() const {
        mcp::Json schema = mcp::object_schema();
        mcp::schema_add(schema, "text", mcp::schema_string("Text to echo"), true);
        return schema;
    }
    virtual void call(const mcp::Json& arguments, mcp::Json& result, mcp::Error& error) {
        const mcp::Json* text = arguments.find("text");
        if (text == 0 || !text->is_string()) {
            error.set(mcp::Error::kInvalidParams, "missing string argument \"text\"");
            return;
        }
        result = mcp::text_result(text->as_string());
    }
};

int main() {
    mcp::Server server("echo", "0.1.0");
    EchoTool echo;
    server.add_tool(&echo);
    return server.run_stdio();
}
```

C++11'de aynı kayıt `mcp/ext/function_tool.hpp` içindeki `FunctionTool` ile lambda alır. C++23'te `mcp/ext/expected_tool.hpp`.

## Bu sürümde olmayanlar

HTTP, SSE, OAuth, resources, prompts, pagination, progress, sampling. Bunlar çekirdeği büyütmeden, transport'un yanına eklenebilir. Pagination istenirse `tools/list` cursor'ı yok sayar ve listeyi tek parçada döner.
