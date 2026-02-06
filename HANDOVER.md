# DIONS 2.0 Development Handover

## Current Session Status (Feb 5, 2026)

### Repositories
1. **Wizrig/DIONS** (dions-2.0 branch) - Legacy codebase with Phase 0 anchor additions
2. **Wizrig/Dions-2.0** (main branch) - Cleaned history, production-ready testnet

### Recent Work Completed

#### DIONS Phase 0 - Anchor Architecture (Wizrig/DIONS)
Created `src/dions2/` with new architecture:
- `anchor.h/cpp` - TX_DIONS_ANCHOR, MerkleTree, tier system
- `stakecheck.h/cpp` - Stake requirements (1000 IOC, 24hr age)
- `datalayer.h/cpp` - IDataLayer interface, LocalDiskDataLayer
- `dionsdb.h/cpp` - LevelDB schemas for anchors/payloads/quotas
- `anchor_rpc.cpp` - New RPCs (getdionsanchor, getdionsquota, etc)

#### Constants
```cpp
DIONS_MIN_STAKE = 1000        // IOC minimum
DIONS_MIN_STAKE_AGE = 86400   // 24 hours
DIONS_PAYLOAD_EXPIRY_DAYS = 30
```

#### Tier System
- BASIC: 1K IOC - 100 msgs/month
- STANDARD: 5K IOC - 1000 msgs/month
- PREMIUM: 10K IOC - 10000 msgs/month
- ENTERPRISE: 50K IOC - 100000 msgs/month
- UNLIMITED: 100K+ IOC - unlimited

#### Command Line Options Added
```
-dions_payload_mode=<legacy|anchor|hybrid>
-dions_data_layer=<null|local>
-dions_prune_interval=<blocks>
```

### Git History Cleaned
- Removed all Claude co-author trailers from Dions-2.0 repo
- Removed CLAUDE.md file from history
- Renamed references to AI_GUIDELINES.md
- Force-pushed clean history

### Build Status ✅ FIXED
The DIONS 2.0 codebase now builds successfully on macOS arm64.

**Build fixes applied (Feb 5, 2026):**
- Fixed boost::filesystem compatibility (namespace alias `fs = boost::filesystem`)
- Fixed serialization using `CDataStream` instead of `std::stringstream`
- Fixed wallet API compatibility (`__wx__` instead of `CWallet`)
- Fixed address class (`cba` instead of `CBitcoinAddress`)
- Added proper includes for `init.h` (pwalletMain extern)

**Build command:**
```bash
cd /Users/taino/Desktop/DIONS-2.0-work/src
make -f makefile.osx clean && make -f makefile.osx -j4
```

**Binary:** `iocoind` - 17MB arm64 Mach-O executable

## Completed Tasks (Feb 5-6, 2026)
- ✅ Wire up DIONS 2.0 RPCs in bitcoinrpc.cpp (added 7 new RPC commands)
- ✅ Port PQC module from Dions-2.0 (Dilithium, Falcon, Kyber algorithms)

### PQC Module Added
New files in `src/dions2/crypto/`:
- `pqc.h` - Post-Quantum Cryptography interfaces
- `pqc.cpp` - Implementation (reference, production uses liboqs)

Device Profiles for IoT:
- `IOT_MINIMAL`: Falcon512 + Kyber512 (690-byte signatures)
- `IOT_STANDARD`: Falcon512 + Kyber768
- `ROBOT_STANDARD`: Dilithium3 + Kyber768
- `ROBOT_PREMIUM`: Dilithium5 + Kyber1024

## Pending Tasks

### High Priority
1. **Add GC trigger** for expired payloads during block processing

2. **Test DIONS 2.0 RPCs on Derek's Mac mini** - Run daemon and verify new endpoints work:
   - `getdionsanchor <anchor_id>`
   - `getdionsproof <anchor_id> <payload_hash>`
   - `verifydionsproof <root> <leaf> <proof_json>`
   - `getdionsquota <address> [month]`
   - `getdionstier [stake_amount]`
   - `getpayloadmode`
   - `getdionsstats`

### Medium Priority
4. **Review EVM/BPF work** for IoT lightweight contracts
   - Derek mentioned smaller smart contracts for IoT/droid devices
   - Check `/Users/taino/Desktop/Derek/DIONS-DVM-Master/IOCOIN_MULTICHAIN_DVM_STRATEGY.md`
   - eBPF VM embedding discussed (rbpf library)

### Derek's Location
- Mac mini at `~/iocoin/derek`
- Set up with GPT-4.1
- System prompt at `/Users/taino/Desktop/Derek/DEREK_SYSTEM_PROMPT.md`

## File Locations

| Path | Description |
|------|-------------|
| `/Users/taino/Desktop/DIONS-2.0-work/` | Main DIONS repo (dions-2.0 branch) |
| `/Users/taino/Desktop/Dions-2.0-cleanup/` | Cleaned Dions-2.0 repo |
| `/Users/taino/Desktop/Derek/DIONS-DVM-Master/` | DVM/EVM code + documentation |
| `/Users/taino/Desktop/ioc-recovery/` | Blockchain data directory |

## Key Architecture Decisions
1. **L1 anchors commitments; bulk payload off-L1**
2. **DIONS access = stakers only** (min 1000 IOC)
3. **24-hour stake age** (reduced from 7 days)
4. **30-day payload expiration** (anchors permanent, payloads GC'd)
5. **Hybrid mode** for transition period

## Commands to Resume

```bash
# Check DIONS repo status
cd /Users/taino/Desktop/DIONS-2.0-work
git log --oneline -5
git status

# Check Dions-2.0 repo
cd /Users/taino/Desktop/Dions-2.0-cleanup
git log --oneline -5

# Build (will fail until boost fix)
cd /Users/taino/Desktop/DIONS-2.0-work/src
make -f makefile.osx clean && make -f makefile.osx -j4
```

## IoT/Lightweight Contracts Analysis

### Existing Work in Dions-2.0 Repo
The **Wizrig/Dions-2.0** repo already has significant IoT infrastructure:

#### 1. Post-Quantum Cryptography (PQC) for IoT
File: `include/dions/pqc.h`, `src/crypto/pqc.cpp`

**Device Profiles:**
- `IOT_MINIMAL` - Falcon512 + Kyber512 (smallest footprint)
- `IOT_STANDARD` - Falcon512 + Kyber768
- `ROBOT_STANDARD` - Dilithium3 + Kyber768
- `ROBOT_PREMIUM` - Dilithium5 + Kyber1024

**Falcon512 advantages for IoT:**
- 690-byte signatures (vs 3293 for Dilithium3)
- NIST Level 1 security
- Optimized for constrained devices

#### 2. Dual VM Architecture
- **EVM Executor** (`include/dions/evm.h`) - Full Ethereum compatibility
- **SVM Executor** (`include/dions/svm.h`) - Solana VM support

#### 3. eBPF Strategy (From Derek's Analysis)
File: `/Users/taino/Desktop/Derek/DIONS-DVM-Master/IOCOIN_MULTICHAIN_DVM_STRATEGY.md`

**Option 3: eBPF VM Embedding**
- Use `rbpf` library for lightweight contract execution
- 2 weeks to embed basic eBPF VM
- Ideal for IoT devices that can't run full EVM

**Derek's IoT Recommendation:**
> "Autonomous agents (AI, robots, IoT) cannot adapt to 100× fee spikes...
> NTR: Agent knows quota, can autonomously manage usage"

This aligns with the tier-based quota system in Phase 0.

### IoT Contract Strategy

| Device Type | VM | Crypto | Quota Tier |
|------------|-----|--------|------------|
| Sensor nodes | eBPF (future) | Falcon512+Kyber512 | BASIC |
| Edge devices | SVM | Falcon512+Kyber768 | STANDARD |
| Robots/Droids | EVM | Dilithium3+Kyber768 | PREMIUM |
| Gateways | EVM | Dilithium5+Kyber1024 | ENTERPRISE |

### Next Steps for IoT
1. **Phase 1:** Integrate PQC from Dions-2.0 into DIONS main
2. **Phase 2:** Add eBPF zone for ultra-lightweight contracts
3. **Phase 3:** Device attestation via stake-weighted signatures

## Next Session Should
1. ~~Fix boost::filesystem build issues~~ ✅ DONE
2. ~~Complete RPC integration~~ ✅ DONE (7 RPCs wired up)
3. ~~Port PQC module from Dions-2.0 to DIONS main~~ ✅ DONE
4. Test DIONS 2.0 RPCs on Derek's Mac mini (run daemon, test endpoints)
5. Add GC trigger for expired payloads
6. Integrate PQC with DIONS message signing
