#!/bin/bash
# RPC wrapper for DIONS testing
# Usage: ./rpc.sh <datadir> <method> [params...]

set -e

DATADIR="$1"
METHOD="$2"
shift 2

CONF="$DATADIR/iocoin.conf"
if [ ! -f "$CONF" ]; then
    echo "Error: Config not found: $CONF" >&2
    exit 1
fi

RPC_USER=$(grep "^rpcuser=" "$CONF" | cut -d= -f2)
RPC_PASS=$(grep "^rpcpassword=" "$CONF" | cut -d= -f2)
RPC_PORT=$(grep "^rpcport=" "$CONF" | cut -d= -f2)

# Build params array
PARAMS="["
FIRST=1
for param in "$@"; do
    if [ $FIRST -eq 0 ]; then
        PARAMS="${PARAMS},"
    fi
    # Check if param looks like a number
    if [[ "$param" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
        PARAMS="${PARAMS}${param}"
    else
        PARAMS="${PARAMS}\"${param}\""
    fi
    FIRST=0
done
PARAMS="${PARAMS}]"

curl --silent --user "$RPC_USER:$RPC_PASS" \
     --data-binary "{\"method\":\"$METHOD\",\"params\":$PARAMS,\"id\":1}" \
     http://127.0.0.1:$RPC_PORT/
