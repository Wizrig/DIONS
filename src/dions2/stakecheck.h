// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 - Stake Requirement Checking

#ifndef DIONS2_STAKECHECK_H
#define DIONS2_STAKECHECK_H

#include "anchor.h"
#include <string>

// Forward declarations - __wx__ is the wallet class in this codebase
class __wx__;

namespace dions2 {

/**
 * Check if an address meets DIONS staking requirements
 * Requirements: 1,000 IOC minimum + 24-hour coin age
 *
 * @param address The address to check
 * @param wallet Pointer to wallet (can be nullptr for chain-only check)
 * @param result Output: detailed result of the check
 * @return true if all requirements are met
 */
bool CheckStakeRequirement(
    const std::string& address,
    const __wx__* wallet,
    StakeCheckResult& result);

/**
 * Check if an operation would exceed the user's monthly quota
 *
 * @param address The staker's address
 * @param tier The user's stake tier
 * @param op_type Type of operation being performed
 * @param op_count Number of operations (e.g., messages in batch)
 * @param op_size Size in bytes of the operation
 * @param current_quota Current quota usage
 * @param error Output: error message if quota exceeded
 * @return true if operation is within quota limits
 */
bool CheckQuota(
    const std::string& address,
    DionsTier tier,
    AnchorKind op_type,
    uint32_t op_count,
    uint64_t op_size,
    const DionsQuota& current_quota,
    std::string& error);

/**
 * Get an address's current stake information without full validation
 *
 * @param address The address to query
 * @param wallet Pointer to wallet
 * @param stake_amount Output: total stake amount
 * @param oldest_age Output: age of oldest coin in seconds
 * @return true if address has any stake
 */
bool GetStakeInfo(
    const std::string& address,
    const __wx__* wallet,
    int64_t& stake_amount,
    int64_t& oldest_age);

/**
 * Calculate required fee for an anchor operation
 *
 * @param anchor The anchor being submitted
 * @param tier Submitter's stake tier
 * @return Required fee in satoshis
 */
int64_t CalculateAnchorFee(const CDionsAnchor& anchor, DionsTier tier);

} // namespace dions2

#endif // DIONS2_STAKECHECK_H
