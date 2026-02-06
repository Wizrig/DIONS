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

### Build Status
The base iocoin codebase has pre-existing boost::filesystem compatibility issues on modern macOS. The DIONS 2.0 code itself is complete but needs the base build issues fixed first.

## Pending Tasks

### High Priority
1. **Fix boost::filesystem compatibility** in db.cpp and init.cpp
   - Change `filesystem::` to `boost::filesystem::`
   - Remove deprecated `boost/filesystem/convenience.hpp` include

2. **Wire up DIONS 2.0 RPCs** in bitcoinrpc.cpp
   - Call `RegisterDions2RPCs()` during server initialization

3. **Add GC trigger** for expired payloads during block processing

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

## Next Session Should
1. Fix boost::filesystem build issues
2. Complete RPC integration
3. Test DIONS 2.0 RPCs
4. Review IoT/lightweight contract strategy
