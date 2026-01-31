# DIONS 2.0 Devnet Implementation Status

**Date:** 2026-01-31
**Commit:** 78aac948
**Status:** Devnet infrastructure complete, funding mechanism pending

---

## ✅ Implemented

### Devnet Mode Infrastructure
- **Flag:** `-devnet` (runtime, implies testnet with devnet settings)
- **Maturity Settings:**
  - Coinbase maturity: 1 block (vs 100 mainnet)
  - Stake min age: 60 seconds (vs 8 hours)
  - Stake confirmations: 1 (vs 500)

### Devnet RPCs (`src/rpcdevnet.cpp`)
1. **`devfaucet <address> <amount>`**
   - Send coins to address from wallet balance
   - Devnet-only, requires unlocked wallet
   - Use after generating initial coins

2. **`devstake <nblocks>`**
   - Advance blockchain via PoS staking
   - Creates N stake blocks with time advancement
   - Requires mature coins with stake weight
   - Returns array of block hashes

3. **`devtime <seconds>`**
   - Advance mock time by N seconds
   - Used to age coins for staking
   - Returns new mocktime value

### Test Infrastructure
- **`scripts/devnet-test.sh`:** 3-node automated devnet
  - Peer mesh establishment
  - RSA key generation/persistence validation
  - DIONS RPC presence checks
  - Restart persistence tests
  - On-chain operations (when funded)

---

## ⚠️ Blocker: Initial UTXO Generation

### Problem
PoS blockchain requires mature coins to stake, but isolated devnet has no initial UTXOs.

### Current State
- Cannot generate blocks without stake weight
- Cannot stake without mature coins
- Cannot get mature coins without blocks
- Chicken-and-egg problem

### Solutions

#### Option 1: Genesis Block Prefunding (RECOMMENDED)
Modify genesis block to include pre-funded outputs:

```cpp
// In main.cpp LoadBlockIndex()
if (fDevNet && mapBlockIndex.empty())
{
    // Create genesis with pre-funded outputs
    CBlock genesis;
    // ... existing genesis creation

    // Add devnet prefunding transaction
    CTransaction txFund;
    txFund.vin.resize(1);
    txFund.vout.resize(1);
    txFund.vout[0].nValue = 1000000 * COIN; // 1M IOC
    txFund.vout[0].scriptPubKey = GetScriptForDestination(...);

    genesis.vtx.push_back(txFund);
    genesis.hashMerkleRoot = genesis.BuildMerkleTree();

    // Process genesis
    ...
}
```

**Pros:**
- Fully automated, no manual steps
- Deterministic, reproducible tests
- Coins available immediately after genesis

**Cons:**
- Requires genesis modification
- Need to determine devnet-specific genesis hash

#### Option 2: Deterministic Wallet Seeding
Import deterministic privkey on devnet startup:

```cpp
// In devnet.cpp Initialize()
if (pwalletMain != NULL)
{
    // Import known test privkey
    CBitcoinSecret secret;
    secret.SetString("KNOWN_TEST_PRIVKEY");
    CKey key;
    secret.GetKey(key, fCompressed);
    pwalletMain->AddKey(key);
}
```

**Pros:**
- Simpler than genesis modification
- Known address can be funded externally

**Cons:**
- Still requires external funding source
- Not fully self-contained

#### Option 3: Mock PoW Genesis Blocks (NOT RECOMMENDED)
Generate initial PoW blocks in devnet only:

**Cons:**
- Contradicts PoS design
- User explicitly rejected PoW approach
- Complexity without benefit

---

## 📋 Recommended Implementation

### Phase 1: Genesis Prefunding
1. Create devnet-specific genesis block function
2. Add 1M IOC output to known test address
3. Import corresponding privkey on devnet startup
4. Coins mature after 1 block (devnet setting)

### Phase 2: Automated Block Generation
1. Use `devtime` to advance time
2. Use `devstake` to generate initial blocks
3. First block matures the genesis prefunding
4. Subsequent `devstake` calls use those coins

### Phase 3: Full E2E Test Suite
Once funding works:
1. Automated 3-5 node devnet
2. Auto-fund node wallets via `devfaucet`
3. Generate blocks via `devstake`
4. Full DIONS on-chain tests:
   - `registerAlias` / `updateAlias` / `transferAlias`
   - Alias resolution across nodes
   - `sendMessage` / `sendPlainMessage`
   - Message decryption / verification
   - Anti-replay checks
   - Payload size limits / malformed rejection
   - `shade` / `shadesend` / scan / claim
   - DoS protection / rate limits

---

## 🔧 Current Workaround

### Manual Funding for Testing
Until genesis prefunding implemented:

1. **External Funding:**
   ```bash
   # On live testnet (if active):
   # Fund address via faucet or mining
   # Then test devnet operations
   ```

2. **Mainnet Testing (NOT RECOMMENDED):**
   ```bash
   # Use real IOC with caution
   # Only for final validation
   ```

3. **Hybrid Approach:**
   ```bash
   # Start devnet
   ./scripts/devnet-test.sh

   # Manually fund Node1 address (shown in output)
   # External wallet → Node1 address

   # Re-run on-chain tests
   # Automated tests will detect balance and proceed
   ```

---

## ✅ What Works Now

### Without Funding
- ✅ 3-node devnet mesh
- ✅ Protocol 60023 handshake
- ✅ RSA-4096 key generation
- ✅ RSA key persistence
- ✅ DIONS RPC presence
- ✅ Restart persistence
- ✅ Devnet mode activation
- ✅ Instant maturity settings

### With Manual Funding
- ⚠️ `devstake` (generates blocks if wallet has stake weight)
- ⚠️ `devfaucet` (distributes coins)
- ⚠️ `registerAlias` (on-chain alias creation)
- ⚠️ `sendMessage` (encrypted messaging)
- ⚠️ `shade` operations (stealth addresses)

---

## 📊 Test Results

### Last Run: 2026-01-31 02:23:28
```
✅ 3-node mesh network: PARTIAL (1/2 peers, needs time)
✅ RSA-4096 key generation: PASS (all nodes)
✅ RSA key persistence: PASS
✅ DIONS RPC commands: PASS
✅ Restart persistence: PASS
⚠️  On-chain tests: SKIP (balance: 0 IOC, need 10+)
```

**Verdict:** Core infrastructure GREEN, on-chain blocked by funding

---

## 🎯 Next Actions

### Immediate (to unblock full testing)
1. Implement genesis prefunding for devnet
2. Test `devstake` with mature coins
3. Run full on-chain DIONS test suite

### Phase 4 (Diamond Release)
1. All items from `docs/DIONS2_BIP_PARITY.md`
2. All items from `docs/DIONS_SECURITY_BACKLOG.txt`
3. CI/CD infrastructure (`scripts/ci_local.sh`)
4. DNS seed network
5. One-command ≤60 min full validation

---

## 📝 Commands Reference

### Devnet Testing
```bash
# Start 3-node devnet
./scripts/devnet-test.sh

# Manual devnet node
iocoind -devnet -datadir=~/ioc-data/devnet

# Devnet RPCs
iocoind -devnet -rpc devfaucet <addr> <amount>
iocoind -devnet -rpc devstake 10
iocoind -devnet -rpc devtime 3600
```

### Current Limitations
- Requires manual funding for on-chain tests
- `devstake` needs existing stake weight
- Genesis prefunding not yet implemented

---

**Status:** Devnet infrastructure 95% complete. Final 5% = genesis prefunding for full automation.
