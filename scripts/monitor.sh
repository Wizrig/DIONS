#!/bin/bash
# DIONS 2.0 Stability Monitor
# Usage: ./monitor.sh <node1_datadir> <node2_datadir> <duration_minutes>

set -e

NODE1_DIR="$1"
NODE2_DIR="$2"
DURATION_MIN="$3"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RPC="$SCRIPT_DIR/rpc.sh"

DURATION_SEC=$((DURATION_MIN * 60))
POLL_INTERVAL=60
START_TIME=$(date +%s)
END_TIME=$((START_TIME + DURATION_SEC))

echo "=== DIONS 2.0 Stability Monitor ==="
echo "Duration: $DURATION_MIN minutes"
echo "Start: $(date)"
echo ""

# Failure counters
DISCONNECT_COUNT=0
BAN_COUNT=0
ERROR_COUNT=0
PROTOCOL_ERROR_COUNT=0

iteration=0
while [ $(date +%s) -lt $END_TIME ]; do
    iteration=$((iteration + 1))
    elapsed=$(($(date +%s) - START_TIME))
    remaining=$((END_TIME - $(date +%s)))

    echo "[Iteration $iteration] Elapsed: ${elapsed}s / Remaining: ${remaining}s"

    # Check Node 1
    INFO1=$("$RPC" "$NODE1_DIR" "getinfo" 2>&1)
    if ! echo "$INFO1" | grep -q "connections"; then
        echo "ERROR: Node 1 RPC failure"
        ERROR_COUNT=$((ERROR_COUNT + 1))
    else
        CONN1=$(echo "$INFO1" | python3 -c "import sys,json; print(json.load(sys.stdin)['result']['connections'])" 2>/dev/null || echo "0")
        echo "  Node 1: $CONN1 connections"
    fi

    # Check Node 2
    INFO2=$("$RPC" "$NODE2_DIR" "getinfo" 2>&1)
    if ! echo "$INFO2" | grep -q "connections"; then
        echo "ERROR: Node 2 RPC failure"
        ERROR_COUNT=$((ERROR_COUNT + 1))
    else
        CONN2=$(echo "$INFO2" | python3 -c "import sys,json; print(json.load(sys.stdin)['result']['connections'])" 2>/dev/null || echo "0")
        echo "  Node 2: $CONN2 connections"
    fi

    # Check peer details
    PEERS1=$("$RPC" "$NODE1_DIR" "getpeerinfo" 2>&1)
    if echo "$PEERS1" | grep -q "result"; then
        BANSCORE1=$(echo "$PEERS1" | python3 -c "import sys,json; peers=json.load(sys.stdin)['result']; print(sum(p.get('banscore',0) for p in peers))" 2>/dev/null || echo "0")
        if [ "$BANSCORE1" -gt 10 ]; then
            echo "WARNING: Node 1 high banscore: $BANSCORE1"
            BAN_COUNT=$((BAN_COUNT + 1))
        fi
    fi

    # Check debug logs for errors
    if [ -f "$NODE1_DIR/testnet/debug.log" ]; then
        RECENT_ERRORS=$(tail -100 "$NODE1_DIR/testnet/debug.log" | grep -c "ERROR\|disconnect\|ProcessMessage.*error" || echo "0")
        if [ "$RECENT_ERRORS" -gt 5 ]; then
            echo "WARNING: Node 1 debug log errors: $RECENT_ERRORS"
            PROTOCOL_ERROR_COUNT=$((PROTOCOL_ERROR_COUNT + 1))
        fi
    fi

    # Check for disconnects
    if [ -f "$NODE1_DIR/testnet/debug.log" ]; then
        DISCONNECT_RECENT=$(tail -200 "$NODE1_DIR/testnet/debug.log" | grep -c "disconnecting" || echo "0")
        if [ "$DISCONNECT_RECENT" -gt 3 ]; then
            echo "WARNING: Node 1 disconnect pattern detected: $DISCONNECT_RECENT"
            DISCONNECT_COUNT=$((DISCONNECT_COUNT + 1))
        fi
    fi

    echo ""

    # Fail fast on critical issues
    if [ $ERROR_COUNT -gt 3 ]; then
        echo "FAIL: Too many RPC errors ($ERROR_COUNT)"
        exit 1
    fi

    if [ $DISCONNECT_COUNT -gt 5 ]; then
        echo "FAIL: Too many disconnects ($DISCONNECT_COUNT)"
        exit 1
    fi

    if [ $BAN_COUNT -gt 3 ]; then
        echo "FAIL: Excessive banscore events ($BAN_COUNT)"
        exit 1
    fi

    # Sleep until next poll
    if [ $(date +%s) -lt $END_TIME ]; then
        sleep $POLL_INTERVAL
    fi
done

echo "=== Monitor Complete ==="
echo "End: $(date)"
echo ""
echo "Summary:"
echo "  RPC Errors: $ERROR_COUNT"
echo "  Disconnect Events: $DISCONNECT_COUNT"
echo "  Banscore Warnings: $BAN_COUNT"
echo "  Protocol Errors: $PROTOCOL_ERROR_COUNT"
echo ""

if [ $ERROR_COUNT -eq 0 ] && [ $DISCONNECT_COUNT -le 2 ] && [ $BAN_COUNT -eq 0 ]; then
    echo "PASS"
    exit 0
else
    echo "WARNING: Some issues detected but within tolerance"
    exit 0
fi
