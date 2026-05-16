#!/bin/sh
# Build, test, and zip one static library for a single C++ standard.
# usage: package-lib.sh <98|11|14|17|20|23> <output-directory>
set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: package-lib.sh <cxx-standard> <output-directory>" >&2
    exit 2
fi

std=$1
dest=$2
case $std in
    98|11|14|17|20|23) ;;
    *)
        echo "unsupported standard: $std" >&2
        exit 2
        ;;
esac

root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
version=$(sed -n 's/^project(mcpp98 VERSION \([0-9.][0-9.]*\).*/\1/p' "$root/CMakeLists.txt")
if [ -z "$version" ]; then
    echo "could not read the project version" >&2
    exit 1
fi

case $(uname -s) in
    Darwin) osname=macos ;;
    Linux) osname=linux ;;
    *) osname=$(uname -s | tr '[:upper:]' '[:lower:]') ;;
esac
arch=$(uname -m)
name="mcpp98-${version}-cpp${std}-${osname}-${arch}"

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
build=$tmp/build
prefix=$tmp/prefix
probe=$tmp/probe

cmake -S "$root" -B "$build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD="$std" \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DCMAKE_CXX_EXTENSIONS=OFF \
    -DCMAKE_INSTALL_LIBDIR=lib
cmake --build "$build" --parallel
ctest --test-dir "$build" --output-on-failure
cmake --install "$build" --prefix "$prefix"

mkdir -p "$probe"
if [ "$std" = 98 ]; then
    cat > "$probe/probe.cpp" << 'EOF'
#include "mcp/server.hpp"

int main()
{
    mcp::Server server("probe", "0.1.0");
    return server.add_tool(0) ? 1 : 0;
}
EOF
else
    cat > "$probe/probe.cpp" << 'EOF'
#include "mcp/ext/tool_builder.hpp"
#include "mcp/server.hpp"

int main()
{
    auto echo = mcp::tool("echo", "Echo the text argument")
        .string("text", "Text to echo")
        .handle([](const mcp::Args& in) {
            return in.string("text");
        });
    mcp::Server server("probe", "0.1.0");
    if (!server.add_tool(&echo)) {
        return 1;
    }
    return 0;
}
EOF
fi

cat > "$probe/CMakeLists.txt" << EOF
cmake_minimum_required(VERSION 3.16)
project(mcpp98_probe LANGUAGES CXX)
set(CMAKE_CXX_STANDARD $std)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(mcpp98 REQUIRED)
add_executable(probe probe.cpp)
target_link_libraries(probe PRIVATE mcpp98::mcpp98)
EOF

cmake -S "$probe" -B "$probe/build" -DCMAKE_PREFIX_PATH="$prefix"
cmake --build "$probe/build"
"$probe/build/probe"

stage=$tmp/stage
mkdir -p "$stage/$name"
cp -R "$prefix/." "$stage/$name/"
mkdir -p "$dest"
dest_abs=$(CDPATH= cd -- "$dest" && pwd)
(
    cd "$stage"
    zip -qr "$dest_abs/$name.zip" "$name"
)
echo "$dest_abs/$name.zip"
