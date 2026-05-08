#!/bin/sh
set -eu
if [ "$#" -ne 1 ]; then
    echo "usage: stdio_smoke.sh <echo-server>" >&2
    exit 2
fi
bin=$1
out=$(
    {
        printf '%s\n' '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"smoke","version":"0"}}}'
        printf '%s\n' '{"jsonrpc":"2.0","method":"notifications/initialized"}'
        printf '%s\n' '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"echo","arguments":{"text":"merhaba"}}}'
    } | "$bin"
)
printf '%s\n' "$out" | grep -q 'merhaba' || {
    printf '%s\n' "$out" >&2
    exit 1
}
