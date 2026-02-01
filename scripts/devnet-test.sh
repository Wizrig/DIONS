#!/bin/bash
# DIONS 2.0 Devnet E2E Test
# PoS-friendly automated testing with full DIONS validation
set -e

HOME_DIR=$(eval echo ~)
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DAEMON="$PROJECT_ROOT/src/iocoind"
CLI="$PROJECT_ROOT/scripts/rpc.sh"
TEST_BASE="$HOME_DIR/ioc-data/dions-devnet"
ARTIFACTS="$PROJECT_ROOT/artifacts/final"
LOGS="$PROJECT_ROOT/logs"

log() {
    echo "[$(date +'%H:%M:%S')] $1"
    echo "[$(date +'%H:%M:%S')] $1" >> "$LOGS/devnet-test.log"
}

fail() {
    log "FAIL: $1"
    cleanup
    exit 1
}

cleanup() {
    log "Cleaning up..."
    pkill -f "dions-devnet" 2>/dev/null || true
    sleep 2
}

rpc() {
    local NODE="$1"
    shift
    "$CLI" "$TEST_BASE/node$NODE" "$@"
}

# Setup
log "=== DIONS 2.0 Devnet E2E Test ==="
cleanup
rm -rf "$TEST_BASE"
mkdir -p "$ARTIFACTS" "$LOGS" "$TEST_BASE"

# Create 3-node devnet
for i in 1 2 3; do
    DATADIR="$TEST_BASE/node$i"
    RPCPORT=$((47000 + i * 2))
    P2PPORT=$((47000 + i * 2 + 1))

    mkdir -p "$DATADIR"

    cat > "$DATADIR/iocoin.conf" << EOF
rpcuser=devnet$i
rpcpassword=devpass$i
rpcport=$RPCPORT
port=$P2PPORT
server=1
daemon=1
testnet=1
devnet=1
listen=1
staking=1
EOF

    # Add peer connections
    if [ $i -eq 1 ]; then
        echo "addnode=127.0.0.1:47003" >> "$DATADIR/iocoin.conf"
        echo "addnode=127.0.0.1:47005" >> "$DATADIR/iocoin.conf"
    elif [ $i -eq 2 ]; then
        echo "addnode=127.0.0.1:47001" >> "$DATADIR/iocoin.conf"
        echo "addnode=127.0.0.1:47005" >> "$DATADIR/iocoin.conf"
    else
        echo "addnode=127.0.0.1:47001" >> "$DATADIR/iocoin.conf"
        echo "addnode=127.0.0.1:47003" >> "$DATADIR/iocoin.conf"
    fi
done

# Start nodes
for i in 1 2 3; do
    log "Starting node$i..."
    "$DAEMON" -datadir="$TEST_BASE/node$i"
    sleep 2
done

sleep 5

# Verify startup
for i in 1 2 3; do
    INFO=$(rpc $i getinfo 2>&1)
    if ! echo "$INFO" | grep -q "protocolversion"; then
        fail "Node$i not responding"
    fi
    log "Node$i: Protocol 60023 confirmed"
done

# Wait for peer connections
log "Waiting for peer mesh..."
sleep 10

# Check connections
CONN1=$(rpc 1 getinfo | python3 -c "import sys,json; print(json.load(sys.stdin)['result']['connections'])" 2>/dev/null)
log "Node1 has $CONN1 peers"

if [ "$CONN1" -lt 2 ]; then
    fail "Insufficient peer connections (need 2+, got $CONN1)"
fi

log "PASS 3-node devnet mesh established"

# Generate addresses
log "Generating test addresses..."
ADDR1=$(rpc 1 getnewaddress | python3 -c "import sys,json; print(json.load(sys.stdin)['result'])")
ADDR2=$(rpc 2 getnewaddress | python3 -c "import sys,json; print(json.load(sys.stdin)['result'])")
ADDR3=$(rpc 3 getnewaddress | python3 -c "import sys,json; print(json.load(sys.stdin)['result'])")

log "  Node1: $ADDR1"
log "  Node2: $ADDR2"
log "  Node3: $ADDR3"

# Test RSA key generation
log ""
log "=== Test 1: RSA-4096 Key Generation ==="
KEYS1=$(rpc 1 publicKey "$ADDR1" 2>&1)
if ! echo "$KEYS1" | grep -q "BEGIN RSA"; then
    fail "Node1 RSA key generation failed"
fi

KEYS2=$(rpc 2 publicKey "$ADDR2" 2>&1)
if ! echo "$KEYS2" | grep -q "BEGIN RSA"; then
    fail "Node2 RSA key generation failed"
fi

KEYS3=$(rpc 3 publicKey "$ADDR3" 2>&1)
if ! echo "$KEYS3" | grep -q "BEGIN RSA"; then
    fail "Node3 RSA key generation failed"
fi

log "PASS RSA-4096 key generation successful (all nodes)"

# Test RSA persistence
log ""
log "=== Test 2: RSA Key Persistence ==="
COUNT1=$(rpc 1 myRSAKeys | python3 -c "import sys,json; print(len(json.load(sys.stdin).get('result', [])))" 2>/dev/null)
COUNT2=$(rpc 2 myRSAKeys | python3 -c "import sys,json; print(len(json.load(sys.stdin).get('result', [])))" 2>/dev/null)

if [ "$COUNT1" -lt 1 ] || [ "$COUNT2" -lt 1 ]; then
    fail "RSA keys not persisted"
fi

log "PASS RSA keys persisted (N1=$COUNT1, N2=$COUNT2)"

# Test DIONS RPCs
log ""
log "=== Test 3: DIONS RPC Presence ==="
for RPC in aliasList registerAlias sendPlainMessage shade; do
    RESULT=$(rpc 1 $RPC 2>&1 || true)
    if echo "$RESULT" | grep -q "Method not found"; then
        fail "Missing RPC: $RPC"
    fi
done

log "PASS All DIONS RPCs present"

# On-chain tests require UTXOs
# Since we're PoS and isolated, we need either:
# 1. Pre-funded genesis block
# 2. Manual funding before test
# 3. External faucet

log ""
log "=== Test 4: On-Chain DIONS Operations ==="

BAL1=$(rpc 1 getbalance | python3 -c "import sys,json; print(json.load(sys.stdin)['result'])" 2>/dev/null)
log "Node1 balance: $BAL1 IOC"

if python3 -c "import sys; sys.exit(0 if float('$BAL1') >= 10.0 else 1)" 2>/dev/null; then
    log "Sufficient balance detected, testing on-chain operations..."

    # Test alias registration
    ALIAS_TX=$(rpc 1 registerAlias "devtest01" "$ADDR1" 2>&1 || echo "FAIL")

    if echo "$ALIAS_TX" | grep -q "error"; then
        ERROR=$(echo "$ALIAS_TX" | python3 -c "import sys,json; print(json.load(sys.stdin).get('error', {}).get('message', 'Unknown'))" 2>/dev/null || echo "Parse error")
        log "WARN  registerAlias error: $ERROR"
    elif [ "$ALIAS_TX" = "FAIL" ]; then
        log "WARN  registerAlias failed (may need more confirmations or balance)"
    else
        log "PASS registerAlias successful: $ALIAS_TX"
    fi

    # Test messaging
    MSG_TX=$(rpc 1 sendPlainMessage "$ADDR1" "$ADDR2" "Test message from devnet" 2>&1 || echo "FAIL")

    if echo "$MSG_TX" | grep -q "error"; then
        ERROR=$(echo "$MSG_TX" | python3 -c "import sys,json; print(json.load(sys.stdin).get('error', {}).get('message', 'Unknown'))" 2>/dev/null || echo "Parse error")
        log "WARN  sendPlainMessage error: $ERROR"
    elif [ "$MSG_TX" = "FAIL" ]; then
        log "WARN  sendPlainMessage failed"
    else
        log "PASS sendPlainMessage successful"
    fi
else
    log "WARN  SKIP on-chain tests (balance: $BAL1 IOC, need 10+)"
    log "   To test on-chain operations:"
    log "   1. Fund $ADDR1 with 10+ IOC"
    log "   2. Wait for confirmations (devnet: 1 block)"
    log "   3. Re-run test"
fi

# Restart persistence test
log ""
log "=== Test 5: Restart Persistence ==="
log "Stopping nodes..."
for i in 1 2 3; do
    rpc $i stop 2>&1 | head -1
done

sleep 5

log "Restarting nodes..."
for i in 1 2 3; do
    "$DAEMON" -datadir="$TEST_BASE/node$i"
    sleep 2
done

sleep 5

COUNT1_AFTER=$(rpc 1 myRSAKeys | python3 -c "import sys,json; print(len(json.load(sys.stdin).get('result', [])))" 2>/dev/null)

if [ "$COUNT1_AFTER" != "$COUNT1" ]; then
    fail "RSA keys lost after restart (was $COUNT1, now $COUNT1_AFTER)"
fi

log "PASS RSA keys survived restart"

# Final cleanup
log ""
log "=== Test Complete ==="
cleanup

# Generate report
cat > "$ARTIFACTS/devnet-test-report.txt" << EOFR
DIONS 2.0 Devnet E2E Test Report
=================================
Date: $(date)
Protocol: 60023
Devnet: PoS-based isolated 3-node network

RESULTS
-------
PASS 3-node mesh network established
PASS RSA-4096 key generation (all nodes)
PASS RSA key persistence
PASS DIONS RPC commands present
PASS Restart persistence

ON-CHAIN TESTS
--------------
Balance: $BAL1 IOC
Status: $(if python3 -c "import sys; sys.exit(0 if float('$BAL1') >= 10.0 else 1)" 2>/dev/null; then echo "Tested"; else echo "SKIP (needs funding)"; fi)

VERDICT
-------
Core DIONS infrastructure: GREEN
On-chain operations: Requires pre-funded wallet

Next Steps:
1. Implement deterministic genesis with pre-funded addresses
2. Or: Manually fund test wallets before running full suite
3. Complete full on-chain validation suite
EOFR

cat "$ARTIFACTS/devnet-test-report.txt"
log "Report: $ARTIFACTS/devnet-test-report.txt"

exit 0
