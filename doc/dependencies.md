# DIONS 2.0 Phase 1 Dependencies

**Last Updated:** 2026-01-30
**Branch:** dions-2.0
**Purpose:** L1 Foundation - Security & Build Modernization

---

## Current Dependencies (main-legacy baseline)

### Critical Security Issue
- **OpenSSL:** 1.0.1h (April 2014) - **147 known CVEs, EOL since 2016**
  - Used for: RSA-4096, ECDSA (secp256k1), ECDH, AES-256-CBC, SHA256
  - Critical vulnerabilities: Heartbleed family, padding oracles, side channels

### Core Dependencies
- **Berkeley DB:** 4.8.x (required for wallet.dat compatibility)
  - **HARD CONSTRAINT:** Cannot upgrade to 5.x/6.x without wallet migration
  - Used for: wallet.dat storage (keys, metadata, transactions)

- **Boost:** ~1.54-1.58 (circa 2013-2015)
  - Used for: filesystem, serialization, threads, program_options, iostreams
  - Components: system, filesystem, iostreams, serialization, program_options, thread

- **LevelDB:** (bundled, version unknown)
  - Used for: blockchain index, UTXO database

- **libevent:** (version unknown, likely 2.0.x)
  - Used for: HTTP RPC server

- **miniupnpc:** (version unknown)
  - Used for: UPnP port mapping

- **zlib:** (system provided)
  - Used for: compression

### Build Toolchain
- **Compiler:** Expects GCC/Clang with C++11 support
- **Build System:** qmake (Qt) + platform-specific makefiles
- **macOS:** Expects llvm-g++, MacPorts dependencies in /opt/local
- **Linux:** Standard GCC/Make
- **Windows:** MinGW cross-compile

---

## Phase 1 Target Dependencies

### Priority 1: OpenSSL Upgrade
**Target:** OpenSSL 1.1.1w (LTS, maintained until 2023-09-11)
**Alternative:** OpenSSL 3.0.13+ (current LTS, requires testing)

**Rationale:**
- 1.1.1w: Last 1.1.x release, well-tested, minimal API changes
- 3.0.x: Current LTS, requires legacy provider testing
- Both eliminate 147 CVEs from 1.0.1h

**API Migration Required:**
- Replace deprecated functions (see OpenSSL Compatibility Layer below)
- Test ECDH, RSA, AES behavior unchanged
- Verify DIONS crypto (messaging, shade, alias) unchanged

**Risk Level:** MEDIUM
- Well-documented migration path
- Behavior preservation critical for consensus
- Extensive testing required

### Priority 2: Boost Upgrade
**Current:** ~1.54-1.58
**Target:** 1.71.0-1.74.0

**Rationale:**
- Avoid 1.80+ (major filesystem changes)
- 1.71-1.74: Stable, C++14/17 compatible
- Security patches, modern compiler support

**Migration Required:**
- Boost.Filesystem v2 → v3 (minor)
- Update deprecated serialization macros
- Test all Boost components used

**Risk Level:** LOW-MEDIUM

### Priority 3: Berkeley DB (NO UPGRADE)
**Current:** 4.8.x
**Target:** 4.8.30 (latest 4.8 release)

**CRITICAL CONSTRAINT:**
- **DO NOT upgrade to 5.x or 6.x**
- Wallet format incompatibility will break all existing wallets
- If Oracle BDB 4.8 unavailable, document alternative sources

**Risk Level:** HIGH if violated

### Priority 4: Other Dependencies
**libevent:** 2.1.12-stable
**miniupnpc:** 2.2.4
**zlib:** 1.3.1 (or system provided)
**LevelDB:** Keep bundled version (low risk)

**Risk Level:** LOW

---

## OpenSSL Compatibility Layer

### Deprecated API Mapping (1.0.1 → 1.1.1/3.0)

| OpenSSL 1.0.1h (Deprecated) | OpenSSL 1.1.1+ Replacement | Status |
|-----------------------------|----------------------------|--------|
| `RSA_generate_key()` | `RSA_generate_key_ex()` | ✅ Already done (src/util.cpp) |
| `ECDH_compute_key()` | `EVP_PKEY_derive()` | ⚠️ Needs verification |
| `EVP_MD_CTX_create()` | `EVP_MD_CTX_new()` | ⚠️ Check usage |
| `EVP_MD_CTX_destroy()` | `EVP_MD_CTX_free()` | ⚠️ Check usage |
| Direct `RSA*` field access | `RSA_get0_*()` accessors | ⚠️ Check usage |
| Direct `EC_KEY*` field access | `EC_KEY_get0_*()` accessors | ⚠️ Check usage |
| `EC_KEY_new()` + set group | `EC_KEY_new_by_curve_name()` | ⚠️ Check usage |

### Files Requiring Audit
1. **src/key.cpp** - ECDSA, ECDH (secp256k1)
2. **src/util.cpp** - RSA-4096, AES-256-CBC
3. **src/crypter.cpp** - Wallet encryption (AES-256-CBC)
4. **src/dions.cpp** - DIONS crypto (if any direct OpenSSL calls)

---

## Build System Modernization

### Current Issues
- Hardcoded MacPorts paths (`/opt/local`)
- Expects llvm-g++ (obsolete)
- No autotools/CMake
- Platform-specific makefiles

### Phase 1 Goals
- Update compiler detection (support modern Clang/GCC)
- Make dependency paths configurable
- Add reproducible build support
- Maintain qmake compatibility (for Qt GUI)

### NOT in Phase 1
- Full CMake migration (risky, out of scope)
- Major build system rewrite

---

## Testing Requirements (Phase 1)

### Minimum Gates
1. **Compilation:** Clean build on macOS 14+, Ubuntu 22.04+, Windows MSVC 2022
2. **Node Startup:** `iocoind` starts without errors
3. **Wallet Open:** Existing wallet.dat opens without migration
4. **Basic RPC:** `getinfo`, `getblockcount` work
5. **Transaction Signing:** Standard P2PKH transaction signs correctly
6. **Block Validation:** Historical block validation works

### Excluded from Phase 1 Testing
- ❌ DIONS RPCs (`registerAlias`, `sendMessage`, `shade`, etc.)
- ❌ DIONS transaction validation
- ❌ DIONS crypto roundtrips (messaging, shade)
- ❌ Full regtest harness (deferred to Phase 2)

---

## Risk Assessment

| Component | Change Type | Risk Level | Mitigation |
|-----------|-------------|------------|------------|
| OpenSSL 1.0.1h → 1.1.1w | Major | MEDIUM | Extensive testing, behavior verification |
| OpenSSL 1.0.1h → 3.0.x | Major | MEDIUM-HIGH | Test with default provider first |
| Boost upgrade | Minor | LOW-MEDIUM | Test serialization, filesystem |
| BDB 4.8 → 4.8.30 | Patch | LOW | Same major version |
| Build system updates | Minor | LOW | Incremental changes |
| Other dependencies | Minor | LOW | Standard upgrades |

### Critical Failure Modes
1. **OpenSSL behavior change** → Consensus fork, wallet incompatibility
2. **BDB major upgrade** → All wallets become unreadable
3. **Boost serialization change** → Network message incompatibility
4. **Compiler optimization** → Non-deterministic behavior

### Mitigation Strategy
- One dependency at a time
- Behavior verification tests
- Rollback plan for each change
- No consensus changes in Phase 1

---

## Version Pins (SHA256 - TBD)

```
# To be added after dependency source verification
OpenSSL 1.1.1w: cf3098950cb4d853ad95c0841f1f9c6d3dc102dccfcacd521d93925208b76ac8
BerkeleyDB 4.8.30: 12edc0df75bf9abd7f82f821795bcee50f42cb2e5f76a6a281b85732798364ef
Boost 1.71.0: d73a8da01e8bf8c7eda40b4c84915071a8c8a0df4a6734537ddde4a8580524ee
libevent 2.1.12: 92e6de1be9ec176428fd2367677e61ceffc2ee1cb119035037a27d346b0403bb
miniupnpc 2.2.4: (hash TBD)
zlib 1.3.1: (hash TBD)
```

---

## Changelog Entry (Draft)

```
DIONS 2.0 Phase 1 - L1 Foundation Upgrade

Security Fixes:
- Upgraded OpenSSL 1.0.1h → 1.1.1w (eliminates 147 CVEs)
- Added OpenSSL compatibility layer for 1.1.1+ and 3.0+

Dependency Updates:
- Boost: 1.54 → 1.71 (security patches, modern compiler support)
- libevent: → 2.1.12-stable
- miniupnpc: → 2.2.4
- BerkeleyDB: 4.8 → 4.8.30 (patch only, wallet format unchanged)

Build System:
- Updated for macOS 14+, Ubuntu 22.04+, Windows MSVC 2022
- Fixed C++14 compatibility warnings
- Made dependency paths configurable

Protocol:
- NO CHANGES (version 60023 reserved, not active)

Consensus:
- NO CHANGES

DIONS Features:
- NO CHANGES (tested to ensure no breakage)

Breaking Changes:
- NONE (backward compatible)
```

---

**End of Dependency Manifest**
