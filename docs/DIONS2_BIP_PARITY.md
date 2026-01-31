# DIONS 2.0 Bitcoin Improvement Proposal (BIP) Parity

**Status:** Phase 4 - Diamond Release
**Last Updated:** 2026-01-31
**Author:** wizrig

## Overview

This document tracks Bitcoin Core BIP implementation status in DIONS 2.0, focusing on security, standardness, and interoperability improvements applicable to our UTXO/script/mempool/transaction model.

## Implementation Status

### ✅ Implemented

**BIP 16: Pay to Script Hash (P2SH)**
- Status: IMPLEMENTED (inherited from Bitcoin Core fork)
- Files: `src/script.cpp`, `src/main.cpp`
- Tests: Standard P2SH validation
- Notes: Foundational feature, fully functional

**BIP 30: Duplicate Transactions**
- Status: IMPLEMENTED
- Files: `src/main.cpp`
- Tests: Duplicate coinbase rejection
- Notes: Prevents duplicate transaction IDs

**BIP 34: Block v2, Height in Coinbase**
- Status: IMPLEMENTED
- Files: `src/main.cpp`, `src/miner.cpp`
- Tests: Block height encoding
- Notes: Enforced for all blocks

### 🔄 Partial / Policy-Level

**BIP 62: Dealing with Malleability (Strict DER, Low-S)**
- Status: PARTIAL (policy-level enforcement)
- Components:
  - ✅ Strict DER encoding: ENABLED (OpenSSL validation)
  - ⚠️ Low-S signatures: POLICY (not consensus)
  - ⚠️ Minimal PUSH operations: NOT ENFORCED
- Files: `src/script.cpp`, `src/key.cpp`
- Priority: HIGH - Upgrade to consensus rules
- Action Item: Implement LOW_S consensus rule with activation height

**BIP 66: Strict DER Signatures**
- Status: POLICY (should be consensus)
- Files: `src/script.cpp`
- Implementation: OpenSSL DER validation active
- Priority: HIGH
- Action Item: Make strict DER a consensus rule with activation

**BIP 113: Median Time-Past**
- Status: IMPLEMENTED
- Files: `src/main.cpp`
- Tests: MTP used for lock time validation
- Notes: Reduces timestamp manipulation

### ❌ Not Implemented (Recommended)

**BIP 65: OP_CHECKLOCKTIMEVERIFY**
- Status: NOT IMPLEMENTED
- Priority: MEDIUM
- Rationale: Enables time-locked contracts
- Risk: Consensus change required
- Action Item: Design activation plan for CLTV

**BIP 112: OP_CHECKSEQUENCEVERIFY**
- Status: NOT IMPLEMENTED
- Priority: MEDIUM
- Rationale: Enables relative time locks
- Risk: Consensus change required
- Action Item: Design activation plan for CSV

**BIP 141/143/144: Segregated Witness**
- Status: NOT APPLICABLE
- Priority: LOW
- Rationale: Major architectural change
- Notes: Future consideration for DIONS 3.0

**BIP 152: Compact Block Relay**
- Status: NOT IMPLEMENTED
- Priority: MEDIUM
- Rationale: Bandwidth optimization
- Risk: Network protocol change
- Action Item: Evaluate for DIONS 2.1

### 🔒 Security & Standardness (Priority Implementation)

**BIP-style Policy Rules (Custom)**
- Status: IN PROGRESS
- Components:
  1. ✅ Canonical signature encoding
  2. ⚠️ Low-S requirement (policy, needs consensus)
  3. ⚠️ Minimal push operations
  4. ✅ Standard transaction types
  5. ✅ Dust relay prevention
  6. ⚠️ Script size limits (needs hardening)

**DIONS-Specific Standardness**
- Status: NEEDS IMPLEMENTATION
- Components:
  1. ❌ Alias payload size limits (consensus)
  2. ❌ Message payload size limits (consensus)
  3. ❌ Shade OP_RETURN size limits (consensus)
  4. ❌ DIONS operation rate limits (policy)
  5. ❌ Anti-spam fee requirements (policy)

## Implementation Plan

### Phase 4A: Critical Security (Immediate)

1. **Strict DER Consensus Rule**
   - File: `src/script.cpp`
   - Add: `SCRIPT_VERIFY_STRICTENC` consensus flag
   - Activation: Block height TBD
   - Test: Reject non-DER signatures in consensus

2. **Low-S Consensus Rule**
   - File: `src/script.cpp`, `src/key.cpp`
   - Add: `SCRIPT_VERIFY_LOW_S` consensus flag
   - Activation: Same as strict DER
   - Test: Reject high-S signatures

3. **DIONS Payload Limits (Consensus)**
   - Files: `src/dions.cpp`, `src/main.cpp`
   - Add: `MAX_ALIAS_LENGTH`, `MAX_MESSAGE_SIZE`, `MAX_SHADE_PAYLOAD`
   - Consensus: Enforce in block validation
   - Test: Reject oversized DIONS operations

### Phase 4B: Network Standardness (Next)

4. **Minimal PUSH Policy**
   - File: `src/script.cpp`
   - Add: Reject non-minimal pushes in mempool
   - Policy: Relay filtering
   - Test: Standard transaction templates

5. **DIONS Anti-Spam Policy**
   - Files: `src/dions.cpp`, `src/main.cpp`
   - Add: Rate limiting per address
   - Policy: Mempool acceptance
   - Test: Flood protection

### Phase 4C: Advanced Features (Future)

6. **OP_CHECKLOCKTIMEVERIFY (BIP 65)**
   - Activation: DIONS 2.1 soft fork
   - Requires: Activation height coordination
   - Benefit: Time-locked contracts

7. **OP_CHECKSEQUENCEVERIFY (BIP 112)**
   - Activation: DIONS 2.1 soft fork
   - Requires: Activation height coordination
   - Benefit: Relative time locks, payment channels

## Testing Requirements

### Unit Tests
- ✅ Strict DER validation
- ✅ Signature verification
- ⚠️ Low-S enforcement (add tests)
- ❌ DIONS payload limits (add tests)
- ❌ Anti-spam policy (add tests)

### Integration Tests
- ✅ Standard transaction relay
- ⚠️ Non-standard rejection (add cases)
- ❌ DIONS operation limits (add tests)
- ❌ Mempool policy enforcement (add tests)

### Consensus Tests
- ✅ Block validation
- ⚠️ Strict encoding consensus (add activation tests)
- ❌ DIONS consensus limits (add tests)

## Activation Strategy

### Soft Fork Activation (for consensus changes)

1. **Preparation Phase**
   - Implement consensus rule behind flag
   - Add unit/integration tests
   - Document activation plan

2. **Signal Phase**
   - Miner signaling via block version
   - 95% threshold over 2016 blocks
   - Grace period: 2016 blocks after lock-in

3. **Activation Phase**
   - Consensus rule enforced
   - Monitor for chain splits
   - Emergency rollback procedure documented

### Policy Changes (non-consensus)

- Can be deployed immediately
- Node upgrade recommended
- No network-wide coordination required

## Files Modified

### Core Script Validation
- `src/script.h` - Add `SCRIPT_VERIFY_*` flags
- `src/script.cpp` - Implement strict validation
- `src/key.cpp` - Low-S signature creation/validation

### DIONS Consensus
- `src/dions.h` - Define payload limits
- `src/dions.cpp` - Enforce limits
- `src/main.cpp` - Block validation integration

### Tests
- `src/test/script_tests.cpp` - Script validation tests
- `src/test/dions_tests.cpp` - DIONS limit tests (new)
- `tests/integration/` - End-to-end tests (new)

## References

- BIP Repository: https://github.com/bitcoin/bips
- Bitcoin Core Implementation: https://github.com/bitcoin/bitcoin
- DIONS Security Backlog: `docs/DIONS_SECURITY_BACKLOG.txt`

## Change Log

- 2026-01-31: Initial BIP parity assessment
- TBD: Strict DER consensus implementation
- TBD: Low-S consensus implementation
- TBD: DIONS payload limits implementation

---

**Next Steps:**
1. Implement strict DER + Low-S consensus (Phase 4A item 1-2)
2. Add DIONS payload limits (Phase 4A item 3)
3. Write comprehensive test suite
4. Document activation plan
5. Deploy to testnet for validation
