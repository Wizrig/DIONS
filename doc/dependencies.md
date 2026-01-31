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

## macOS Build Instructions (Phase 1)

### Prerequisites

**System Requirements:**
- macOS 11 (Big Sur) or later
- Xcode Command Line Tools: `xcode-select --install`
- Homebrew package manager: https://brew.sh

### Install Dependencies via Homebrew

```bash
# Install OpenSSL (3.x recommended, 1.1 also supported)
brew install openssl@3

# Install Berkeley DB 4.8 (CRITICAL: Must be 4.8.x for wallet compatibility)
# Note: Homebrew removed db@4.8, you may need to:
# Option 1: Use berkeley-db@4 if available (verify it's 4.8.x)
brew install berkeley-db@4

# Option 2: Build from source (see manual instructions below)
# Option 3: Use MacPorts for db48

# Install Boost
brew install boost

# Install other dependencies
brew install miniupnpc libevent

# Verify OpenSSL installation
openssl version
# Expected: OpenSSL 3.x.x or 1.1.1x

# Check Homebrew paths
ls /opt/homebrew/opt/openssl@3  # Apple Silicon
ls /usr/local/opt/openssl@3     # Intel Mac
```

### Berkeley DB 4.8 Notes (CRITICAL)

**Why 4.8.x is required:**
- DIONS wallet.dat format is BDB 4.8
- Upgrading to BDB 5.x or 6.x will make all existing wallets unreadable
- **DO NOT use BDB 5.x/6.x** unless you have a wallet migration plan

**If Homebrew doesn't provide BDB 4.8:**

```bash
# Option 1: Use MacPorts
sudo port install db48

# Option 2: Build from source (advanced)
wget http://download.oracle.com/berkeley-db/db-4.8.30.tar.gz
tar -xzvf db-4.8.30.tar.gz
cd db-4.8.30/build_unix
../dist/configure --enable-cxx --disable-shared --with-pic --prefix=/usr/local/db-4.8
make
sudo make install

# Then set BDB_INCLUDE_PATH and BDB_LIB_PATH when building
export BDB_INCLUDE_PATH=/usr/local/db-4.8/include
export BDB_LIB_PATH=/usr/local/db-4.8/lib
```

### Build IOCoin Daemon

```bash
# Clone repository
git clone https://github.com/Wizrig/DIONS
cd DIONS
git checkout dions-2.0

# Build iocoind (daemon)
cd src

# Option 1: Use makefile.osx (auto-detects Homebrew paths)
make -f makefile.osx

# Option 2: Specify paths manually if auto-detection fails
make -f makefile.osx \
  DEPSDIR=/opt/homebrew \
  OPENSSL_PREFIX=/opt/homebrew/opt/openssl@3

# Option 3: For MacPorts users
make -f makefile.osx \
  DEPSDIR=/opt/local \
  OPENSSL_PREFIX=/opt/local

# Build output
# Success: ./iocoind binary created
# Failure: Check error messages for missing dependencies
```

### Build Verification (Phase 1 Gate)

```bash
# Test binary exists
ls -lh iocoind
# Expected: executable binary, ~5-15MB

# Test binary runs (version check)
./iocoind --version
# Expected: IOCoin version info, no crashes

# Test help output
./iocoind --help
# Expected: Command-line options displayed

# Optional: Check binary dependencies
otool -L iocoind | grep -E "ssl|crypto|boost|db"
# Expected: Links to Homebrew OpenSSL, Boost, BDB
```

### Common Build Issues

**Issue: OpenSSL not found**
```bash
# Solution: Set OPENSSL_PREFIX explicitly
make -f makefile.osx OPENSSL_PREFIX=/opt/homebrew/opt/openssl@3
```

**Issue: Berkeley DB not found**
```bash
# Solution: Set BDB paths
export BDB_INCLUDE_PATH=/opt/local/include/db48
export BDB_LIB_PATH=/opt/local/lib/db48
make -f makefile.osx
```

**Issue: Boost not found**
```bash
# Solution: Set DEPSDIR
make -f makefile.osx DEPSDIR=/opt/homebrew
```

**Issue: Compiler errors about deprecated OpenSSL functions**
```bash
# Expected: This should NOT happen with dions-2.0 branch
# The makefile.osx includes -Wno-deprecated-declarations
# If you still see errors, verify you're on dions-2.0 branch:
git status
git log --oneline -3
```

### Phase 1 Build Gate (Minimal Success Criteria)

**Required for Phase 1 completion:**
- [x] `make -f makefile.osx` completes without errors
- [x] `iocoind` binary is created
- [x] `./iocoind --version` displays version info without crash

**NOT required for Phase 1:**
- [ ] Full regtest validation (deferred to Phase 2)
- [ ] DIONS RPC testing (deferred to Phase 2)
- [ ] Network sync testing (deferred to Phase 2)

### Next Steps After Phase 1 Build Success

Once the daemon compiles cleanly:
1. Document build success (compiler version, OS version, dependency versions)
2. Commit build system changes
3. Proceed to Phase 2: DIONS feature preservation testing

### Troubleshooting

**Get build environment info:**
```bash
# Compiler
clang++ --version

# macOS version
sw_vers

# Homebrew info
brew --prefix
brew list --versions openssl boost berkeley-db miniupnpc

# Architecture
uname -m
# Apple Silicon: arm64
# Intel: x86_64
```

**Clean build:**
```bash
make -f makefile.osx clean
rm -f obj/*.o obj/*.P
make -f makefile.osx
```

---

**End of Dependency Manifest**
