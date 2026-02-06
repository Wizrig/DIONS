// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 - Stake Requirement Implementation

#include "stakecheck.h"
#include "../wallet.h"
#include "../main.h"
#include "../base58.h"  // For cba (iocoin address)
#include <ctime>

namespace dions2 {

bool CheckStakeRequirement(
    const std::string& address,
    const __wx__* wallet,
    StakeCheckResult& result)
{
    result.has_minimum_stake = false;
    result.has_minimum_age = false;
    result.total_stake = 0;
    result.oldest_coin_age = 0;
    result.tier = DionsTier::NONE;
    result.error.clear();

    if (!wallet) {
        result.error = "Wallet not available";
        return false;
    }

    // Get stake info from wallet
    int64_t stake_amount = 0;
    int64_t oldest_age = 0;

    if (!GetStakeInfo(address, wallet, stake_amount, oldest_age)) {
        result.error = "Failed to get stake information for address";
        return false;
    }

    result.total_stake = stake_amount;
    result.oldest_coin_age = oldest_age;

    // Check minimum stake
    if (stake_amount < DIONS_MIN_STAKE) {
        result.error = "Insufficient stake: " + std::to_string(stake_amount) +
                       " IOC (minimum " + std::to_string(DIONS_MIN_STAKE) + " IOC required)";
        return false;
    }
    result.has_minimum_stake = true;

    // Check minimum age (24 hours)
    if (oldest_age < DIONS_MIN_STAKE_AGE) {
        result.error = "Stake too young: " + std::to_string(oldest_age / 3600) +
                       " hours (minimum 24 hours required)";
        return false;
    }
    result.has_minimum_age = true;

    // Determine tier
    result.tier = GetTierFromStake(stake_amount);

    return true;
}

bool GetStakeInfo(
    const std::string& address,
    const __wx__* wallet,
    int64_t& stake_amount,
    int64_t& oldest_age)
{
    stake_amount = 0;
    oldest_age = 0;

    if (!wallet) {
        return false;
    }

    // TODO: Integrate with actual wallet UTXO scanning
    // This is a placeholder that needs to be connected to wallet.cpp

    // For now, we'll use a simplified approach:
    // Iterate through wallet transactions and sum up balance for address

    LOCK(wallet->cs_wallet);

    int64_t current_time = GetTime();
    int64_t oldest_time = current_time;  // Track oldest coin time

    for (const auto& item : wallet->mapWallet) {
        const __wx__Tx& wtx = item.second;

        // Skip non-confirmed transactions
        if (!wtx.IsTrusted())
            continue;

        // Check each output
        for (unsigned int i = 0; i < wtx.vout.size(); i++) {
            // Check if this output belongs to the address
            CTxDestination dest;
            if (!ExtractDestination(wtx.vout[i].scriptPubKey, dest))
                continue;

            std::string out_address = cba(dest).ToString();
            if (out_address != address)
                continue;

            // Check if output is still unspent
            if (wtx.IsSpent(i))
                continue;

            // Add to stake
            stake_amount += wtx.vout[i].nValue / COIN;  // Convert to IOC

            // Track coin age
            int64_t coin_time = wtx.nTime;
            if (coin_time > 0) {
                int64_t age = current_time - coin_time;
                if (coin_time < oldest_time) {
                    oldest_time = coin_time;
                    oldest_age = age;
                }
            }
        }
    }

    return stake_amount > 0;
}

bool CheckQuota(
    const std::string& address,
    DionsTier tier,
    AnchorKind op_type,
    uint32_t op_count,
    uint64_t op_size,
    const DionsQuota& current_quota,
    std::string& error)
{
    if (tier == DionsTier::NONE) {
        error = "No stake tier - staking required for DIONS access";
        return false;
    }

    TierLimits limits = TierLimits::GetLimits(tier);

    // Check message count
    uint32_t new_message_count = current_quota.messages_used + op_count;
    if (new_message_count > limits.monthly_messages) {
        error = "Monthly message quota exceeded: " +
                std::to_string(current_quota.messages_used) + "/" +
                std::to_string(limits.monthly_messages);
        return false;
    }

    // Check byte quota
    uint64_t new_bytes = current_quota.bytes_used + op_size;
    if (new_bytes > limits.monthly_bytes) {
        error = "Monthly byte quota exceeded: " +
                std::to_string(current_quota.bytes_used / 1024 / 1024) + "MB/" +
                std::to_string(limits.monthly_bytes / 1024 / 1024) + "MB";
        return false;
    }

    // Check batch size for batch operations
    if (op_type == AnchorKind::BATCH && op_count > limits.max_batch_size) {
        error = "Batch size exceeds tier limit: " +
                std::to_string(op_count) + "/" +
                std::to_string(limits.max_batch_size);
        return false;
    }

    return true;
}

int64_t CalculateAnchorFee(const CDionsAnchor& anchor, DionsTier tier) {
    // Base fee: 0.001 IOC per anchor
    int64_t base_fee = 100000;  // In satoshis (0.001 IOC = 100000 sat at 8 decimals)

    // Additional fee per payload in batch
    int64_t per_payload_fee = 1000;  // 0.00001 IOC per payload

    int64_t total_fee = base_fee + (anchor.payload_count * per_payload_fee);

    // Tier discounts
    switch (tier) {
        case DionsTier::PREMIUM:
            total_fee = total_fee * 90 / 100;  // 10% discount
            break;
        case DionsTier::ENTERPRISE:
            total_fee = total_fee * 75 / 100;  // 25% discount
            break;
        case DionsTier::UNLIMITED:
            total_fee = total_fee * 50 / 100;  // 50% discount
            break;
        default:
            break;
    }

    return total_fee;
}

} // namespace dions2
