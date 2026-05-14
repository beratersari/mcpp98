# mcpp98

MCP server kütüphanesi. Aynı kaynak, C++98, C++11, C++14, C++17, C++20 ve C++23 ile derlenir. C++21 ve C++24 diye bir ISO standardı yoktur; matris bu altıdır.

Çekirdek, bu standartların ortak alt kümesidir. C++11 kaydı ve `std::expected` ayrı header'dadır ve alt standartta include edilmez.

## Katmanlar

```
json       DOM, ayrıştırıcı, yazıcı. Protokol bilmez.
rpc        JSON-RPC 2.0
tool       C++98 sanal Tool ve CallToolResult yardımcıları
transport  satır çerçeveleme. stdio burada.
session    initialize, ping, tools/list, tools/call. Taşımadan habersiz.
server     tool tablosu ve dışarıdan kullanılan yüz
ext/       C++11 tool() kaydı, C++23 ExpectedTool
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

C++11 ve sonrasında araç şöyle yazılır. Örnek program `echo_server_11`, yalnız o standartla ve üstüyle derlenir.

```cpp
#include "mcp/ext/tool_builder.hpp"
#include "mcp/server.hpp"

int main() {
    auto echo = mcp::tool("echo", "Echo the text argument")
        .string("text", "Text to echo")
        .handle([](const mcp::Args& in) {
            return in.string("text");
        });

    mcp::Server server("echo", "0.1.0");
    server.add_tool(&echo);
    return server.run_stdio();
}
```

`string`, `number` ve `boolean` birer argümandır. Üçüncü değer `false` ise argüman isteğe bağlıdır. Gövde değeri `Args` üzerinden okur. `in.number("limit", 20.0)` yazılmamışsa 20 döner. Eksik ya da yanlış tipli argüman gövdeye girmez; model `isError` görür.

Gövde hem metin hem hata döndürecekse dönüş tipi `mcp::Reply` olur.

```cpp
.handle([](const mcp::Args& in) -> mcp::Reply {
    if (in.string("customer").empty()) {
        return mcp::fail("customer is empty");
    }
    return in.string("customer");
});
```

Araç nesnesi `run` bitene kadar yaşar. C++98'de aynı iş `examples/echo_server.cpp` içindeki `Tool` alt sınıfıdır. Ham JSON görmek için `mcp/ext/function_tool.hpp`, `std::expected` döndürmek için `mcp/ext/expected_tool.hpp`.

## Bu sürümde olmayanlar

HTTP, SSE, OAuth, resources, prompts, pagination, progress, sampling. Bunlar çekirdeği büyütmeden, transport'un yanına eklenebilir. Pagination istenirse `tools/list` cursor'ı yok sayar ve listeyi tek parçada döner.
