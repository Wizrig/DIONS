# DIONS 2.0 Phase 1 Build Gate

**Purpose:** Minimal build validation for Phase 1 (L1 foundation only)
**Branch:** dions-2.0
**Target:** Compile iocoind and iocoin-cli on modern toolchains
**Scope:** Build system only - NO runtime/regtest validation required

---

## Phase 1 Objectives

✅ **IN SCOPE:**
- Compile with OpenSSL 1.1.1+ or 3.x (eliminates 147 CVEs)
- Support Homebrew dependencies on macOS
- Support modern compilers (Clang 10+, GCC 9+)
- Maintain backward compatibility with existing build paths

❌ **OUT OF SCOPE (Phase 2):**
- Runtime testing (regtest, mainnet sync)
- DIONS RPC validation (messaging, aliases, shade)
- Crypto behavior verification
- Full integration tests

---

## Success Criteria

### Required (MUST PASS):
1. ✅ `make -f makefile.osx` completes without compilation errors
2. ✅ `make -f makefile.unix` completes without compilation errors (Linux)
3. ✅ Binaries created: `iocoind` and/or `iocoin-cli`
4. ✅ Binary runs: `./iocoind --version` shows version without crash

### Optional (NICE TO HAVE):
- [ ] `./iocoind --help` displays command-line options
- [ ] Binary dependencies check: `otool -L iocoind` shows correct libs (macOS)
- [ ] Binary dependencies check: `ldd iocoind` shows correct libs (Linux)

### Explicitly NOT Required:
- ❌ Node startup (deferred to Phase 2)
- ❌ Wallet operations (deferred to Phase 2)
- ❌ Network connectivity (deferred to Phase 2)
- ❌ DIONS RPC functionality (deferred to Phase 2)

---

## Build Commands (Quick Reference)

### macOS (Homebrew OpenSSL)

```bash
cd /path/to/DIONS
git checkout dions-2.0
git pull
cd src

# Auto-detect Homebrew paths
make -f makefile.osx

# Or specify paths explicitly
make -f makefile.osx \
  DEPSDIR=/opt/homebrew \
  OPENSSL_PREFIX=/opt/homebrew/opt/openssl@3
```

### macOS (MacPorts)

```bash
cd /path/to/DIONS/src

make -f makefile.osx \
  DEPSDIR=/opt/local \
  OPENSSL_PREFIX=/opt/local
```

### Linux (System OpenSSL)

```bash
cd /path/to/DIONS/src

# Set dependency paths via environment
export BOOST_INCLUDE_PATH=/usr/include
export BOOST_LIB_PATH=/usr/lib/x86_64-linux-gnu
export BDB_INCLUDE_PATH=/usr/include
export BDB_LIB_PATH=/usr/lib/x86_64-linux-gnu
export OPENSSL_INCLUDE_PATH=/usr/include/openssl
export OPENSSL_LIB_PATH=/usr/lib/x86_64-linux-gnu

make -f makefile.unix
```

### Linux (Custom OpenSSL 3.x)

```bash
# If you built OpenSSL from source in /opt/openssl-3.x
export OPENSSL_INCLUDE_PATH=/opt/openssl-3.x/include
export OPENSSL_LIB_PATH=/opt/openssl-3.x/lib

make -f makefile.unix
```

---

## Validation Steps

### Step 1: Clean Build

```bash
cd src
make -f makefile.osx clean  # or makefile.unix
rm -f obj/*.o obj/*.P obj/build.h
```

### Step 2: Compile

```bash
# macOS
make -f makefile.osx 2>&1 | tee build.log

# Linux
make -f makefile.unix 2>&1 | tee build.log

# Check for errors
grep -i "error:" build.log
# Expected: No output (zero errors)

# Check for critical warnings (optional)
grep -i "warning:" build.log | grep -v "deprecated"
# Expected: Minimal warnings (deprecated warnings are suppressed)
```

### Step 3: Verify Binary

```bash
# Check binary created
ls -lh iocoind
# Expected: Executable, ~5-15MB depending on build type

# Check it's actually executable
file iocoind
# Expected: Mach-O 64-bit executable (macOS) or ELF 64-bit executable (Linux)

# Test version command
./iocoind --version
# Expected: Displays version info, exits cleanly
# Example output:
#   IOCoin version v1.x.x.x-...
#   Protocol version: 60023
```

### Step 4: Dependency Check (Optional)

**macOS:**
```bash
otool -L iocoind | grep -E "ssl|crypto|boost|db"
# Expected: Shows links to OpenSSL, Boost, BDB in correct locations
# Example:
#   /opt/homebrew/opt/openssl@3/lib/libssl.3.dylib
#   /opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib
```

**Linux:**
```bash
ldd iocoind | grep -E "ssl|crypto|boost|db"
# Expected: Shows links to OpenSSL, Boost, BDB
# Example:
#   libssl.so.3 => /usr/lib/x86_64-linux-gnu/libssl.so.3
#   libcrypto.so.3 => /usr/lib/x86_64-linux-gnu/libcrypto.so.3
```

---

## Expected Build Output

### Success Output:

```
Building LevelDB ...
[LevelDB compilation messages...]
clang++ -c -Wall -Wextra -Wformat -Wformat-security -Wno-unused-parameter ...
[Many .cpp -> .o compilations...]
clang++ -o iocoind ...
```

**Key indicators of success:**
- No lines containing "error:"
- Final line creates iocoind binary
- All object files compiled without fatal errors

### Common Warnings (EXPECTED, can be ignored):

```
warning: 'ECDH_compute_key' is deprecated [-Wdeprecated-declarations]
```
**Status:** ✅ Expected with OpenSSL 3.x, suppressed by -Wno-deprecated-declarations

```
warning: unused parameter 'xyz' [-Wunused-parameter]
```
**Status:** ✅ Expected, suppressed by -Wno-unused-parameter

### Failure Indicators:

```
fatal error: 'openssl/xyz.h' file not found
```
**Problem:** OpenSSL headers not found
**Solution:** Set OPENSSL_PREFIX or install OpenSSL dev headers

```
undefined reference to `SSL_xyz'
```
**Problem:** OpenSSL libraries not linked
**Solution:** Set OPENSSL_LIB_PATH or fix LIBPATHS in makefile

```
fatal error: 'db_cxx.h' file not found
```
**Problem:** Berkeley DB headers not found
**Solution:** Install BDB 4.8 or set BDB_INCLUDE_PATH

---

## Phase 1 Gate Checklist

**Before declaring Phase 1 build success:**

- [ ] Verified on dions-2.0 branch (`git status`)
- [ ] Clean build completed (`make clean && make`)
- [ ] Zero compilation errors (`grep error: build.log` is empty)
- [ ] Binary created (`ls -lh iocoind` shows file)
- [ ] Binary runs (`./iocoind --version` works)
- [ ] OpenSSL version confirmed (`otool -L` or `ldd` shows 1.1+ or 3.x)
- [ ] Build log saved for reference (`build.log` committed or documented)

**Once all checkboxes pass:**
- ✅ Phase 1 build gate PASSED
- ➡️ Ready to commit build system changes
- ➡️ Ready to proceed to Phase 2 (runtime validation)

---

## Troubleshooting

### Issue: "OpenSSL not found"

**Symptoms:**
```
fatal error: 'openssl/opensslv.h' file not found
```

**Solutions:**
```bash
# Verify OpenSSL installed
brew list openssl@3  # macOS Homebrew
apt list --installed libssl-dev  # Ubuntu/Debian

# Set explicit path
make OPENSSL_PREFIX=/opt/homebrew/opt/openssl@3  # macOS
export OPENSSL_INCLUDE_PATH=/usr/include/openssl  # Linux
```

### Issue: "Berkeley DB not found"

**Symptoms:**
```
fatal error: 'db_cxx.h' file not found
```

**Solutions:**
```bash
# macOS: Use MacPorts for db48
sudo port install db48
make DEPSDIR=/opt/local

# Linux: Install BDB 4.8 dev package
sudo apt-get install libdb4.8++-dev  # Ubuntu/Debian
```

### Issue: "Boost not found"

**Symptoms:**
```
fatal error: 'boost/filesystem.hpp' file not found
```

**Solutions:**
```bash
# macOS
brew install boost

# Linux
sudo apt-get install libboost-all-dev
export BOOST_INCLUDE_PATH=/usr/include
```

### Issue: Linker errors with OpenSSL 3.x

**Symptoms:**
```
undefined reference to `EVP_MD_CTX_create'
```

**Root Cause:** Code not using openssl_compat.h

**Solution:**
- Verify you're on dions-2.0 branch (has compatibility layer)
- Check src/key.cpp, src/util.cpp, src/crypter.cpp include openssl_compat.h
- Rebuild clean: `make clean && make`

---

## Build Environment Documentation

**When build succeeds, document:**

```bash
# Save build environment info
cat > build-success-info.txt <<EOF
Build Date: $(date)
Branch: $(git rev-parse --abbrev-ref HEAD)
Commit: $(git rev-parse HEAD)
OS: $(uname -s -r)
Compiler: $(clang++ --version | head -1)  # or g++
OpenSSL: $(openssl version)
Homebrew Prefix: $(brew --prefix 2>/dev/null || echo "N/A")
Make Command: make -f makefile.osx
Build Time: ~X minutes
Binary Size: $(ls -lh iocoind | awk '{print $5}')
Dependencies:
$(otool -L iocoind 2>/dev/null || ldd iocoind 2>/dev/null)
EOF

cat build-success-info.txt
```

---

## Next Steps After Phase 1 Success

1. **Commit build system changes:**
   ```bash
   git add src/makefile.osx src/makefile.unix doc/
   git commit -m "DIONS 2.0 Phase 1: Build system modernization for OpenSSL 3.x"
   ```

2. **Push to dions-2.0 branch:**
   ```bash
   git push origin dions-2.0
   ```

3. **Document success:**
   - Update this file with actual build results
   - Commit build-success-info.txt if useful

4. **Proceed to Phase 2:**
   - Runtime validation (node startup, wallet open)
   - DIONS feature preservation tests
   - L1 transaction signing verification

---

**Phase 1 Gate Status:** ⏸️ PENDING (awaiting build attempt)

**Update this section after build:**
- Date: YYYY-MM-DD
- Builder: [name]
- Platform: macOS X.Y / Ubuntu X.Y
- Result: PASS / FAIL
- Notes: [any issues encountered]
