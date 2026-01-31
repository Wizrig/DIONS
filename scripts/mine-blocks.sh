#!/bin/bash
# Generate blocks on isolated testnet for DIONS testing
set -e

NODE_DIR="$1"
NUM_BLOCKS="${2:-120}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Mining $NUM_BLOCKS blocks on node at $NODE_DIR..."

for i in $(seq 1 $NUM_BLOCKS); do
    RESULT=$("$SCRIPT_DIR/rpc.sh" "$NODE_DIR" "setgenerate" true 1 2>&1)
    if [ $((i % 10)) -eq 0 ]; then
        echo "  Generated $i blocks..."
    fi
    sleep 0.1
done

BALANCE=$("$SCRIPT_DIR/rpc.sh" "$NODE_DIR" "getbalance" 2>&1)
echo "Final balance: $BALANCE"
echo "Mining complete"
