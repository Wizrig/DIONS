# DIONS 2.0 Development Handover

## Current Session Status (Feb 6, 2026 - Updated)

### Latest Git Commits (Wizrig/DIONS dions-2.0 branch)
```
f257ab2a Add hybrid signature RPCs
661028bd Add GC module and hybrid PQC signature support
ea76bbba Update HANDOVER.md: PQC module ported successfully
333c9d8e Add Post-Quantum Cryptography (PQC) module for IoT devices
9accf4f4 Wire up DIONS 2.0 RPCs in main RPC table
66288a54 Fix DIONS 2.0 build issues for macOS
e0cddd43 DIONS 2.0 Phase 0: Anchor architecture foundation
```

### Repositories
1. **Wizrig/DIONS** (dions-2.0 branch) - Main development, all commits under Wizrig
2. **Wizrig/Dions-2.0** (main branch) - Cleaned history, production-ready testnet

### Collaboration
- **Derek** (Mac mini at `~/iocoin/derek`) - GPT-4.1 assistant, should help with testnet work
- Commits can be under **Wizrig** or **reed** (Derek's commits)

---

## BUILD STATUS ✅ WORKING

**Binary:** `iocoind` - 17MB arm64 Mach-O executable (macOS Apple Silicon)

**Build command:**
```bash
cd /Users/taino/Desktop/DIONS-2.0-work/src
make -f makefile.osx clean && make -f makefile.osx -j4
```

**Build fixes applied (Feb 5-6, 2026):**
- Fixed boost::filesystem compatibility (namespace alias `fs = boost::filesystem`)
- Fixed serialization using `CDataStream` instead of `std::stringstream`
- Fixed wallet API compatibility (`__wx__` instead of `CWallet`)
- Fixed address class (`cba` instead of `CBitcoinAddress`)
- Added proper includes for `init.h` (pwalletMain extern)

---

## COMPLETED TASKS ✅

### 1. DIONS Phase 0 - Anchor Architecture
Created `src/dions2/` with new architecture:
- `anchor.h/cpp` - TX_DIONS_ANCHOR, MerkleTree, tier system
- `stakecheck.h/cpp` - Stake requirements (1000 IOC, 24hr age)
- `datalayer.h/cpp` - IDataLayer interface, LocalDiskDataLayer
- `dionsdb.h/cpp` - LevelDB schemas for anchors/payloads/quotas
- `anchor_rpc.cpp` - New RPCs (9 commands)

### 2. RPC Commands Wired Up (11 Total)
Added to `bitcoinrpc.cpp`:
- `getdionsanchor <anchor_id>` - Get anchor details
- `getdionsproof <anchor_id> <payload_hash>` - Get Merkle proof
- `verifydionsproof <root> <leaf> <proof_json>` - Verify proof
- `getdionsquota <address> [month]` - Get quota usage
- `getdionstier [stake_amount]` - Get stake tier
- `getpayloadmode` - Get storage mode
- `getdionsstats` - Get DIONS statistics
- `getdionsgcstats` - Get GC statistics
- `forcedionsgc` - Force a GC run
- `gethybridsigschemes` - List hybrid signature schemes **NEW**
- `getrecommendedsigscheme` - Get recommended scheme for device **NEW**

### 3. PQC Module Ported
New files in `src/dions2/crypto/`:
- `pqc.h` - Post-Quantum Cryptography interfaces
- `pqc.cpp` - Implementation (reference, production uses liboqs)

**Algorithms:**
| Algorithm | Type | Size | NIST Level | Use Case |
|-----------|------|------|------------|----------|
| Falcon512 | Signature | 690 bytes | 1 | IoT devices |
| Falcon1024 | Signature | 1330 bytes | 5 | Secure IoT |
| Dilithium2 | Signature | 2420 bytes | 2 | Standard |
| Dilithium3 | Signature | 3293 bytes | 3 | Recommended |
| Dilithium5 | Signature | 4595 bytes | 5 | High security |
| Kyber512 | KEM | 768 bytes | 1 | IoT devices |
| Kyber768 | KEM | 1088 bytes | 3 | Recommended |
| Kyber1024 | KEM | 1568 bytes | 5 | High security |

**Device Profiles:**
- `IOT_MINIMAL`: Falcon512 + Kyber512 (smallest footprint)
- `IOT_STANDARD`: Falcon512 + Kyber768
- `ROBOT_STANDARD`: Dilithium3 + Kyber768
- `ROBOT_PREMIUM`: Dilithium5 + Kyber1024

### 4. GC Module Added ✅ NEW
New files:
- `gc.h` - GC configuration, statistics, and API
- `gc.cpp` - Implementation

**Features:**
- Automatic GC trigger on block connection (every N blocks)
- Configurable via command-line args:
  - `-dions_gc_interval=100` (blocks between GC runs)
  - `-dions_gc_max_prune=1000` (max payloads per run)
  - `-dions_gc_enabled=1` (enable/disable)
- Stats tracking: payloads pruned, bytes reclaimed, run count
- Hooks into `SetBestChain` for automatic triggering
- RPC commands: `getdionsgcstats`, `forcedionsgc`

---

## PENDING TASKS 📋

### High Priority
1. **Test DIONS 2.0 RPCs on Derek's Mac mini**
   - Run daemon with testnet
   - Test all 9 new RPC endpoints
   - Verify stake checking works

2. **Integrate PQC with DIONS message signing**
   - Add PQC signature option for DIONS messages
   - Hybrid mode: Ed25519 + Falcon for transition

### Medium Priority
3. **Review EVM/BPF work** for IoT lightweight contracts
   - eBPF VM embedding (rbpf library)
   - Check Derek's DVM strategy document

4. **Port EVM/SVM executors** from Dions-2.0

---

## CONSTANTS & TIER SYSTEM

```cpp
DIONS_MIN_STAKE = 1000        // IOC minimum
DIONS_MIN_STAKE_AGE = 86400   // 24 hours
DIONS_PAYLOAD_EXPIRY_DAYS = 30
```

| Tier | Stake (IOC) | Messages/Month | Bytes/Month |
|------|-------------|----------------|-------------|
| BASIC | 1,000 | 100 | 10 MB |
| STANDARD | 5,000 | 1,000 | 100 MB |
| PREMIUM | 10,000 | 10,000 | 1 GB |
| ENTERPRISE | 50,000 | 100,000 | 10 GB |
| UNLIMITED | 100,000+ | Unlimited | Unlimited |

---

## GC CONFIGURATION

```cpp
struct GCConfig {
    uint32_t prune_interval_blocks = 100;  // Run GC every 100 blocks
    uint32_t max_prune_per_run = 1000;     // Limit per run
    bool enabled = true;
};
```

**Command-line options:**
```bash
./iocoind -dions_gc_interval=50 -dions_gc_max_prune=500 -dions_gc_enabled=1
```

**RPC commands:**
```bash
./iocoind getdionsgcstats
./iocoind forcedionsgc
```

---

## FILE LOCATIONS

| Path | Description |
|------|-------------|
| `/Users/taino/Desktop/DIONS-2.0-work/` | Main DIONS repo (dions-2.0 branch) |
| `/Users/taino/Desktop/DIONS-2.0-work/src/dions2/` | Phase 0 anchor code |
| `/Users/taino/Desktop/DIONS-2.0-work/src/dions2/crypto/` | PQC module |
| `/Users/taino/Desktop/DIONS-2.0-work/src/dions2/gc.h` | GC configuration & API |
| `/Users/taino/Desktop/DIONS-2.0-work/src/dions2/gc.cpp` | GC implementation |
| `/Users/taino/Desktop/Dions-2.0-cleanup/` | Cleaned Dions-2.0 repo |
| `/Users/taino/Desktop/Derek/DIONS-DVM-Master/` | DVM/EVM code + documentation |
| `/Users/taino/Desktop/ioc-recovery/` | Blockchain data directory |
| `~/iocoin/derek` | Derek's Mac mini workspace |

---

## DEREK'S MAC MINI

- Location: `~/iocoin/derek`
- Assistant: GPT-4.1
- System prompt: `/Users/taino/Desktop/Derek/DEREK_SYSTEM_PROMPT.md`
- **Task file: `/Users/taino/Desktop/Derek/DEREK_TASKS.md`** ← Derek reads this
- Should help with testnet work and testing

### Tasks for Derek (see DEREK_TASKS.md for details):
1. Run iocoind daemon on testnet
2. Test DIONS 2.0 RPC endpoints (9 commands now)
3. Security review of new RPCs
4. PQC module unit testing
5. GC module testing
6. Can commit under "reed"

---

## COMMANDS TO RESUME

```bash
# Check current status
cd /Users/taino/Desktop/DIONS-2.0-work
git log --oneline -5
git status

# Build
cd /Users/taino/Desktop/DIONS-2.0-work/src
make -f makefile.osx clean && make -f makefile.osx -j4

# Run daemon (testnet)
./iocoind -testnet -daemon

# Test RPC
./iocoind getdionsstats
./iocoind getdionstier 5000
./iocoind getpayloadmode
./iocoind getdionsgcstats
./iocoind forcedionsgc
```

---

## KEY ARCHITECTURE DECISIONS

1. **L1 anchors commitments; bulk payload off-L1**
2. **DIONS access = stakers only** (min 1000 IOC)
3. **24-hour stake age** (reduced from 7 days)
4. **30-day payload expiration** (anchors permanent, payloads GC'd)
5. **Hybrid mode** for transition period
6. **PQC ready** for post-quantum transition
7. **Automatic GC** on block connect (configurable interval)

---

## IoT/Lightweight Contracts Strategy

| Device Type | VM | Crypto | Quota Tier |
|------------|-----|--------|------------|
| Sensor nodes | eBPF (future) | Falcon512+Kyber512 | BASIC |
| Edge devices | SVM | Falcon512+Kyber768 | STANDARD |
| Robots/Droids | EVM | Dilithium3+Kyber768 | PREMIUM |
| Gateways | EVM | Dilithium5+Kyber1024 | ENTERPRISE |

### Next Steps for IoT
1. ✅ **Phase 1:** Integrate PQC from Dions-2.0 into DIONS main - DONE
2. **Phase 2:** Add eBPF zone for ultra-lightweight contracts
3. **Phase 3:** Device attestation via stake-weighted signatures

---

## NEXT SESSION SHOULD

1. ~~Fix boost::filesystem build issues~~ ✅ DONE
2. ~~Complete RPC integration~~ ✅ DONE (9 RPCs wired up)
3. ~~Port PQC module from Dions-2.0 to DIONS main~~ ✅ DONE
4. ~~Add GC trigger for expired payloads~~ ✅ DONE
5. **Test DIONS 2.0 RPCs on Derek's Mac mini** (run daemon, test endpoints)
6. **Integrate PQC with DIONS message signing**
7. **Commit GC module to GitHub**

---

*Last updated: Feb 6, 2026*
