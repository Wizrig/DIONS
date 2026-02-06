// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 - Anchor and Merkle Tree Implementation

#include "anchor.h"
#include <openssl/sha.h>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace dions2 {

//-----------------------------------------------------------------------------
// TierLimits implementation
//-----------------------------------------------------------------------------
TierLimits TierLimits::GetLimits(DionsTier tier) {
    TierLimits limits;
    switch (tier) {
        case DionsTier::BASIC:
            limits.min_stake = 1000;
            limits.monthly_messages = 100;
            limits.monthly_bytes = 10 * 1024 * 1024;  // 10 MB
            limits.max_batch_size = 10;
            break;
        case DionsTier::STANDARD:
            limits.min_stake = 5000;
            limits.monthly_messages = 1000;
            limits.monthly_bytes = 100 * 1024 * 1024;  // 100 MB
            limits.max_batch_size = 100;
            break;
        case DionsTier::PREMIUM:
            limits.min_stake = 10000;
            limits.monthly_messages = 10000;
            limits.monthly_bytes = 1024 * 1024 * 1024;  // 1 GB
            limits.max_batch_size = 1000;
            break;
        case DionsTier::ENTERPRISE:
            limits.min_stake = 50000;
            limits.monthly_messages = 100000;
            limits.monthly_bytes = 10ULL * 1024 * 1024 * 1024;  // 10 GB
            limits.max_batch_size = 10000;
            break;
        case DionsTier::UNLIMITED:
            limits.min_stake = 100000;
            limits.monthly_messages = UINT32_MAX;
            limits.monthly_bytes = UINT64_MAX;
            limits.max_batch_size = 100000;
            break;
        default:
            limits.min_stake = 0;
            limits.monthly_messages = 0;
            limits.monthly_bytes = 0;
            limits.max_batch_size = 0;
            break;
    }
    return limits;
}

//-----------------------------------------------------------------------------
// Get tier from stake amount
//-----------------------------------------------------------------------------
DionsTier GetTierFromStake(int64_t stake_amount) {
    if (stake_amount >= 100000) return DionsTier::UNLIMITED;
    if (stake_amount >= 50000) return DionsTier::ENTERPRISE;
    if (stake_amount >= 10000) return DionsTier::PREMIUM;
    if (stake_amount >= 5000) return DionsTier::STANDARD;
    if (stake_amount >= 1000) return DionsTier::BASIC;
    return DionsTier::NONE;
}

//-----------------------------------------------------------------------------
// Get current month string (YYYY-MM)
//-----------------------------------------------------------------------------
std::string GetCurrentMonth() {
    time_t now = time(nullptr);
    struct tm* tm_info = gmtime(&now);
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << (tm_info->tm_year + 1900)
        << "-" << std::setw(2) << (tm_info->tm_mon + 1);
    return oss.str();
}

//-----------------------------------------------------------------------------
// CDionsAnchor implementation
//-----------------------------------------------------------------------------
std::array<uint8_t, 32> CDionsAnchor::GetAnchorId() const {
    std::array<uint8_t, 32> result;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, &version, sizeof(version));
    SHA256_Update(&ctx, &kind, sizeof(kind));
    SHA256_Update(&ctx, &epoch_id, sizeof(epoch_id));
    SHA256_Update(&ctx, channel_id.data(), 32);
    SHA256_Update(&ctx, payload_root.data(), 32);
    SHA256_Update(&ctx, &payload_count, sizeof(payload_count));
    SHA256_Update(&ctx, &timestamp, sizeof(timestamp));
    SHA256_Final(result.data(), &ctx);
    return result;
}

//-----------------------------------------------------------------------------
// MerkleTree implementation
//-----------------------------------------------------------------------------
std::array<uint8_t, 32> MerkleTree::HashNodes(
    const std::array<uint8_t, 32>& left,
    const std::array<uint8_t, 32>& right)
{
    std::array<uint8_t, 32> result;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, left.data(), 32);
    SHA256_Update(&ctx, right.data(), 32);
    SHA256_Final(result.data(), &ctx);
    return result;
}

std::array<uint8_t, 32> MerkleTree::Build(
    const std::vector<std::array<uint8_t, 32>>& leaves)
{
    if (leaves.empty()) {
        std::array<uint8_t, 32> empty;
        empty.fill(0);
        return empty;
    }

    auto layers = BuildLayers(leaves);
    return layers.back()[0];
}

std::vector<std::vector<std::array<uint8_t, 32>>> MerkleTree::BuildLayers(
    const std::vector<std::array<uint8_t, 32>>& leaves)
{
    std::vector<std::vector<std::array<uint8_t, 32>>> layers;

    if (leaves.empty()) {
        return layers;
    }

    // First layer is the leaves
    layers.push_back(leaves);

    // Build each subsequent layer
    while (layers.back().size() > 1) {
        const auto& current = layers.back();
        std::vector<std::array<uint8_t, 32>> next_layer;

        for (size_t i = 0; i < current.size(); i += 2) {
            if (i + 1 < current.size()) {
                next_layer.push_back(HashNodes(current[i], current[i + 1]));
            } else {
                // Odd number of nodes - duplicate the last one
                next_layer.push_back(HashNodes(current[i], current[i]));
            }
        }
        layers.push_back(next_layer);
    }

    return layers;
}

MerkleProof MerkleTree::GetProof(
    const std::vector<std::vector<std::array<uint8_t, 32>>>& layers,
    uint32_t leaf_index)
{
    MerkleProof proof;
    proof.leaf_index = leaf_index;

    if (layers.empty() || leaf_index >= layers[0].size()) {
        return proof;
    }

    uint32_t index = leaf_index;
    for (size_t layer = 0; layer < layers.size() - 1; layer++) {
        const auto& current = layers[layer];
        uint32_t sibling_index = (index % 2 == 0) ? index + 1 : index - 1;

        if (sibling_index < current.size()) {
            proof.siblings.push_back(current[sibling_index]);
        } else {
            // No sibling - use the node itself (for odd-sized layers)
            proof.siblings.push_back(current[index]);
        }

        proof.path.push_back(index % 2 == 1);  // true if we're on the right
        index /= 2;
    }

    return proof;
}

bool MerkleTree::VerifyProof(
    const std::array<uint8_t, 32>& root,
    const std::array<uint8_t, 32>& leaf,
    const MerkleProof& proof)
{
    if (proof.siblings.size() != proof.path.size()) {
        return false;
    }

    std::array<uint8_t, 32> current = leaf;

    for (size_t i = 0; i < proof.siblings.size(); i++) {
        if (proof.path[i]) {
            // Current node is on the right
            current = HashNodes(proof.siblings[i], current);
        } else {
            // Current node is on the left
            current = HashNodes(current, proof.siblings[i]);
        }
    }

    return current == root;
}

} // namespace dions2
