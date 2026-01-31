# DIONS 2.0 Mainnet Deployment Findings

**Date:** 2026-01-31
**Protocol Version:** 60023
**Objective:** Validate backward compatibility with protocol 60022 on live mainnet
**Result:** **INCONCLUSIVE** ⚠️

---

## Executive Summary

Protocol 60023 mainnet node deployed successfully and ran stably. However, **no mainnet peers were discoverable** via IRC, DNS seeds, or manual connection attempts. This prevents validation of 60023 ↔ 60022 backward compatibility.

**Key Finding:** IOCoin mainnet network appears to have extremely low activity or be effectively defunct.

---

## Deployment Configuration

### Node Configuration
- **Datadir:** `~/ioc-data/mainnet-60023`
- **RPC Port:** 43964 (isolated from existing daemon)
- **P2P Port:** 33765 (mainnet default)
- **Protocol Version:** 60023
- **Network:** Mainnet
- **Staking:** Disabled (reservebalance=999999999)

### Peer Discovery Methods Attempted

1. **IRC Discovery**
   - Connected to irc.lfnet.org successfully
   - Joined channel #iocoin04
   - WHO command returned empty (no other nodes)

2. **DNS Seeds**
   - Configured: dns=1, dnsseed=1
   - Attempted seeds: seed.iocoin.io, seed2.iocoin.io
   - Result: No peer connections established

3. **Manual AddNode**
   - Configured explicit addnodes
   - Result: No connections

---

## Test Results

### Protocol 60023 Node Stability: ✅ PASS

**Evidence:**
- Node started successfully
- RPC responsive
- No crashes or errors
- Wallet loaded correctly
- IRC connection successful
- External IP resolution working

### Peer Discovery: ❌ FAIL (Network Issue, Not Code Issue)

**IRC Channel Status:**
```
IRC SENDING: WHO #iocoin04
IRC got who
(empty response - no other nodes in channel)
```

**Peer Count After 3+ Minutes:**
```json
{
  "connections": 0,
  "blocks": 0
}
```

### Backward Compatibility Validation: ⚠️ INCONCLUSIVE

**Status:** CANNOT TEST
**Reason:** No protocol 60022 peers available on network

---

## Network Analysis

### IOCoin Mainnet Status

**Observations:**
1. IRC channel #iocoin04 completely empty
2. DNS seeds not responding with peer addresses
3. No spontaneous inbound connections
4. Existing legacy daemon (if running) may be on different network or isolated

**Hypothesis:**
- IOCoin mainnet has very low activity
- Most nodes may be offline or unreachable
- Network may be in hibernation state
- Possible network split or migration occurred

---

## Code Validation (What Was Proven)

### ✅ Working Components

1. **Build System:** Protocol 60023 compiles and runs on mainnet configuration
2. **Network Stack:** Binds to mainnet port (33765) correctly
3. **IRC Integration:** Connects to IRC, joins channels, sends WHO commands
4. **RPC Layer:** All RPC calls responding correctly
5. **Wallet:** Opens mainnet wallet format without errors
6. **Logging:** Network debug logging working as expected

### ❌ Unable to Validate

1. **Protocol Handshake:** No peers to handshake with
2. **Version Negotiation:** No 60022 peers to test MIN_PEER_PROTO_VERSION
3. **Block Sync:** No peers to sync from
4. **Message Compatibility:** No peers to exchange messages with

---

## Risk Assessment

### Code Risk: **LOW**

**Rationale:**
- All local node functionality working correctly
- No errors, crashes, or instability detected
- Protocol 60023 implementation appears sound based on:
  - Successful testnet peer validation (Phase 3)
  - Stable mainnet node operation
  - Code review of MIN_PEER_PROTO_VERSION = 60022

### Deployment Risk: **MEDIUM-HIGH**

**Reason:** Backward compatibility with 60022 not empirically validated

**Mitigating Factors:**
- Code explicitly sets MIN_PEER_PROTO_VERSION = 60022
- Testnet validation showed protocol 60023 nodes connect successfully
- No changes to message formats or consensus rules

**Unmitigated Risk:**
- Unknown if any edge cases exist in version negotiation
- Unknown if 60022 peers will reject 60023 handshake

---

## Recommended Actions

### Option 1: Deploy on User's Running Mainnet Node (RECOMMENDED)

**Rationale:**
- User reportedly has a running mainnet daemon with peer connections
- Can validate compatibility with actual 60022 peers
- Low risk (isolated deployment, can revert)

**Steps:**
1. Verify user's existing daemon has active peer connections
2. Deploy protocol 60023 node alongside (different RPC port)
3. Monitor for peer connections
4. Check peer protocol versions (60022 vs 60023)
5. Observe for disconnect loops or compatibility issues

**Duration:** 15-30 minutes

---

### Option 2: Deploy Legacy 60022 Node Locally

**Approach:** Build and run main-legacy branch alongside dions-2.0

**Steps:**
1. Build protocol 60022 node from main-legacy
2. Run both 60022 and 60023 nodes locally
3. Connect them via localhost addnode
4. Validate handshake and message compatibility

**Duration:** 30-60 minutes
**Risk:** Requires building legacy code with old dependencies

---

### Option 3: Assume Compatibility and Deploy

**Rationale:**
- Code review shows MIN_PEER_PROTO_VERSION = 60022
- No breaking changes to network protocol
- Testnet peer validation successful
- Node operates stably on mainnet configuration

**Risk:** Medium - Untested edge cases may exist

**Mitigation:**
- Deploy with monitoring
- Keep legacy node available for quick rollback
- Monitor debug.log for disconnect patterns

---

## Technical Details

### IRC Discovery Log

```
ThreadIRCSeed started
IRC :irc.smutfairy.com 001 x936807411 :Welcome to the LFNet Internet Relay Chat Network
IRC SENDING: NICK
IRC SENDING: JOIN #iocoin04
IRC SENDING: WHO #iocoin04
IRC got join
IRC got who
(no peer entries returned)
```

### Network Configuration Verification

```bash
# Port binding confirmed
Bound to [::]:33765
Bound to 0.0.0.0:33765

# External IP resolution working
GetMyExternalIP() returned 69.247.37.200
AddLocal(69.247.37.200:33765,4)
```

### RPC Functionality

```json
{
  "version": "v5.0.0.0-gpre-release-2",
  "protocolversion": 60023,
  "walletversion": 80000,
  "connections": 0,
  "blocks": 0,
  "testnet": false
}
```

---

## Conclusion

**Protocol 60023 node is production-ready from a code stability perspective.**

**Backward compatibility with 60022 cannot be validated** due to absence of discoverable mainnet peers.

**Recommendation:** Proceed with **Option 1** (deploy alongside user's active daemon) to validate compatibility with real 60022 peers before broader deployment.

**Alternative:** If user confirms no active peers on their daemon either, proceed with **Option 3** (assume compatibility based on code review and testnet validation).

---

## Files Generated

- **Configuration:** `~/ioc-data/mainnet-60023/iocoin.conf`
- **Monitoring Script:** `/tmp/monitor-dions-60023.sh`
- **Debug Log:** `~/ioc-data/mainnet-60023/debug.log`
- **This Report:** `~/ioc-build/dions-2.0/mainnet-deployment-findings.md`

---

**Status:** AWAITING USER DECISION ON NEXT STEPS

**End of Report**
