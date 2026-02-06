// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 - RPC Methods for Anchors, Proofs, and Quotas

#include "anchor.h"
#include "stakecheck.h"
#include "datalayer.h"
#include "dionsdb.h"
#include "gc.h"
#include "../bitcoinrpc.h"
#include "../main.h"
#include "../wallet.h"
#include "../init.h"  // For pwalletMain

#include <sstream>
#include <iomanip>

using namespace json_spirit;
using namespace dions2;

namespace {

// Helper: array to hex string
std::string ToHex(const std::array<uint8_t, 32>& arr) {
    std::ostringstream oss;
    for (uint8_t byte : arr) {
        oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

// Helper: hex string to array
bool FromHex(const std::string& hex, std::array<uint8_t, 32>& out) {
    if (hex.size() != 64) return false;
    for (size_t i = 0; i < 32; i++) {
        unsigned int byte;
        std::istringstream iss(hex.substr(i * 2, 2));
        iss >> std::hex >> byte;
        out[i] = static_cast<uint8_t>(byte);
    }
    return true;
}

// Helper: tier to string
std::string TierToString(DionsTier tier) {
    switch (tier) {
        case DionsTier::NONE: return "none";
        case DionsTier::BASIC: return "basic";
        case DionsTier::STANDARD: return "standard";
        case DionsTier::PREMIUM: return "premium";
        case DionsTier::ENTERPRISE: return "enterprise";
        case DionsTier::UNLIMITED: return "unlimited";
        default: return "unknown";
    }
}

} // anonymous namespace

//-----------------------------------------------------------------------------
// getdionsanchor - Get anchor by ID
//-----------------------------------------------------------------------------
Value getdionsanchor(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw std::runtime_error(
            "getdionsanchor <anchor_id>\n"
            "Get a DIONS 2.0 anchor by its ID.\n"
            "\nArguments:\n"
            "1. anchor_id  (string, required) The anchor ID (64 hex characters)\n"
            "\nResult:\n"
            "{\n"
            "  \"anchor_id\": \"xxx\",\n"
            "  \"version\": n,\n"
            "  \"kind\": \"xxx\",\n"
            "  \"channel_id\": \"xxx\",\n"
            "  \"payload_root\": \"xxx\",\n"
            "  \"payload_count\": n,\n"
            "  \"timestamp\": n,\n"
            "  \"tx_hash\": \"xxx\",\n"
            "  \"block_height\": n\n"
            "}\n"
        );

    std::string anchor_id_hex = params[0].get_str();
    std::array<uint8_t, 32> anchor_id;
    if (!FromHex(anchor_id_hex, anchor_id))
        throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid anchor_id format");

    // TODO: Get actual DB pointer from global state
    // For now, return placeholder
    Object result;
    result.push_back(Pair("error", "Database not initialized - Phase 0 stub"));
    return result;
}

//-----------------------------------------------------------------------------
// getdionsproof - Get Merkle proof for a payload
//-----------------------------------------------------------------------------
Value getdionsproof(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw std::runtime_error(
            "getdionsproof <anchor_id> <payload_hash>\n"
            "Get a Merkle inclusion proof for a payload in an anchor.\n"
            "\nArguments:\n"
            "1. anchor_id    (string, required) The anchor ID\n"
            "2. payload_hash (string, required) The payload hash\n"
            "\nResult:\n"
            "{\n"
            "  \"valid\": true|false,\n"
            "  \"proof\": {\n"
            "    \"siblings\": [\"xxx\", ...],\n"
            "    \"path\": [true|false, ...],\n"
            "    \"leaf_index\": n\n"
            "  }\n"
            "}\n"
        );

    std::string anchor_id_hex = params[0].get_str();
    std::string payload_hash_hex = params[1].get_str();

    std::array<uint8_t, 32> anchor_id, payload_hash;
    if (!FromHex(anchor_id_hex, anchor_id))
        throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid anchor_id format");
    if (!FromHex(payload_hash_hex, payload_hash))
        throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid payload_hash format");

    // TODO: Implement actual proof retrieval
    Object result;
    result.push_back(Pair("error", "Proof retrieval not implemented - Phase 0 stub"));
    return result;
}

//-----------------------------------------------------------------------------
// verifydionsproof - Verify a Merkle proof
//-----------------------------------------------------------------------------
Value verifydionsproof(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw std::runtime_error(
            "verifydionsproof <root> <leaf> <proof_json>\n"
            "Verify a Merkle inclusion proof.\n"
            "\nArguments:\n"
            "1. root       (string, required) The Merkle root\n"
            "2. leaf       (string, required) The leaf hash\n"
            "3. proof_json (string, required) The proof as JSON\n"
            "\nResult:\n"
            "true|false\n"
        );

    std::string root_hex = params[0].get_str();
    std::string leaf_hex = params[1].get_str();
    std::string proof_json = params[2].get_str();

    std::array<uint8_t, 32> root, leaf;
    if (!FromHex(root_hex, root))
        throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid root format");
    if (!FromHex(leaf_hex, leaf))
        throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid leaf format");

    // Parse proof JSON
    Value proof_val;
    if (!read_string(proof_json, proof_val))
        throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid proof JSON");

    Object proof_obj = proof_val.get_obj();
    Array siblings_arr = find_value(proof_obj, "siblings").get_array();
    Array path_arr = find_value(proof_obj, "path").get_array();
    int leaf_index = find_value(proof_obj, "leaf_index").get_int();

    MerkleProof proof;
    proof.leaf_index = leaf_index;

    for (const Value& sib : siblings_arr) {
        std::array<uint8_t, 32> sibling;
        if (!FromHex(sib.get_str(), sibling))
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid sibling hash");
        proof.siblings.push_back(sibling);
    }

    for (const Value& p : path_arr) {
        proof.path.push_back(p.get_bool());
    }

    bool valid = MerkleTree::VerifyProof(root, leaf, proof);
    return valid;
}

//-----------------------------------------------------------------------------
// getdionsquota - Get quota usage for an address
//-----------------------------------------------------------------------------
Value getdionsquota(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw std::runtime_error(
            "getdionsquota <address> [month]\n"
            "Get DIONS 2.0 quota usage for an address.\n"
            "\nArguments:\n"
            "1. address (string, required) The IOC address\n"
            "2. month   (string, optional) Month in YYYY-MM format (default: current)\n"
            "\nResult:\n"
            "{\n"
            "  \"address\": \"xxx\",\n"
            "  \"month\": \"YYYY-MM\",\n"
            "  \"tier\": \"xxx\",\n"
            "  \"messages_used\": n,\n"
            "  \"messages_limit\": n,\n"
            "  \"bytes_used\": n,\n"
            "  \"bytes_limit\": n\n"
            "}\n"
        );

    std::string address = params[0].get_str();
    std::string month = params.size() > 1 ? params[1].get_str() : GetCurrentMonth();

    // Get stake info to determine tier
    StakeCheckResult stake_result;
    CheckStakeRequirement(address, pwalletMain, stake_result);

    TierLimits limits = TierLimits::GetLimits(stake_result.tier);

    Object result;
    result.push_back(Pair("address", address));
    result.push_back(Pair("month", month));
    result.push_back(Pair("tier", TierToString(stake_result.tier)));
    result.push_back(Pair("stake", stake_result.total_stake));
    result.push_back(Pair("stake_age_hours", stake_result.oldest_coin_age / 3600));
    result.push_back(Pair("messages_used", 0));  // TODO: Get from DB
    result.push_back(Pair("messages_limit", static_cast<int64_t>(limits.monthly_messages)));
    result.push_back(Pair("bytes_used", 0));  // TODO: Get from DB
    result.push_back(Pair("bytes_limit", static_cast<int64_t>(limits.monthly_bytes)));
    result.push_back(Pair("max_batch_size", static_cast<int>(limits.max_batch_size)));

    if (!stake_result.has_minimum_stake || !stake_result.has_minimum_age) {
        result.push_back(Pair("warning", stake_result.error));
    }

    return result;
}

//-----------------------------------------------------------------------------
// getdionstier - Get tier info for a stake amount
//-----------------------------------------------------------------------------
Value getdionstier(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw std::runtime_error(
            "getdionstier [stake_amount]\n"
            "Get DIONS 2.0 tier information.\n"
            "\nArguments:\n"
            "1. stake_amount (numeric, optional) IOC amount to check tier for\n"
            "\nResult:\n"
            "{\n"
            "  \"tiers\": [\n"
            "    {\"name\": \"basic\", \"min_stake\": 1000, ...},\n"
            "    ...\n"
            "  ],\n"
            "  \"your_tier\": \"xxx\" (if stake_amount provided)\n"
            "}\n"
        );

    Array tiers;

    // List all tiers
    for (int i = 1; i <= 5; i++) {
        DionsTier tier = static_cast<DionsTier>(i);
        TierLimits limits = TierLimits::GetLimits(tier);

        Object tier_obj;
        tier_obj.push_back(Pair("name", TierToString(tier)));
        tier_obj.push_back(Pair("min_stake", limits.min_stake));
        tier_obj.push_back(Pair("monthly_messages", static_cast<int64_t>(limits.monthly_messages)));
        tier_obj.push_back(Pair("monthly_bytes_mb", static_cast<int64_t>(limits.monthly_bytes / 1024 / 1024)));
        tier_obj.push_back(Pair("max_batch_size", static_cast<int>(limits.max_batch_size)));
        tiers.push_back(tier_obj);
    }

    Object result;
    result.push_back(Pair("tiers", tiers));
    result.push_back(Pair("min_stake_age_hours", 24));

    if (params.size() > 0) {
        int64_t stake = params[0].get_int64();
        DionsTier tier = GetTierFromStake(stake);
        result.push_back(Pair("your_stake", stake));
        result.push_back(Pair("your_tier", TierToString(tier)));
    }

    return result;
}

//-----------------------------------------------------------------------------
// getpayloadmode - Get current payload mode
//-----------------------------------------------------------------------------
Value getpayloadmode(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "getpayloadmode\n"
            "Get the current DIONS 2.0 payload mode.\n"
            "\nResult:\n"
            "{\n"
            "  \"mode\": \"legacy|anchor|hybrid\",\n"
            "  \"data_layer\": \"xxx\"\n"
            "}\n"
        );

    Object result;

    // TODO: Get actual mode from global state
    std::string mode = "hybrid";  // Default during transition
    std::string data_layer_type = g_data_layer ? g_data_layer->GetTypeName() : "not_initialized";

    result.push_back(Pair("mode", mode));
    result.push_back(Pair("data_layer", data_layer_type));

    if (g_data_layer) {
        uint64_t total_payloads, total_bytes, expired_count;
        g_data_layer->GetStats(total_payloads, total_bytes, expired_count);
        result.push_back(Pair("payloads_stored", static_cast<int64_t>(total_payloads)));
        result.push_back(Pair("bytes_stored", static_cast<int64_t>(total_bytes)));
        result.push_back(Pair("expired_pending", static_cast<int64_t>(expired_count)));
    }

    return result;
}

//-----------------------------------------------------------------------------
// getdionsstats - Get DIONS 2.0 statistics
//-----------------------------------------------------------------------------
Value getdionsstats(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "getdionsstats\n"
            "Get DIONS 2.0 system statistics.\n"
            "\nResult:\n"
            "{\n"
            "  \"total_anchors\": n,\n"
            "  \"total_payloads\": n,\n"
            "  \"expired_payloads\": n,\n"
            "  \"payload_mode\": \"xxx\",\n"
            "  \"data_layer\": \"xxx\"\n"
            "}\n"
        );

    Object result;

    result.push_back(Pair("version", "2.0.0-phase0"));
    result.push_back(Pair("min_stake", DIONS_MIN_STAKE));
    result.push_back(Pair("min_stake_age_seconds", DIONS_MIN_STAKE_AGE));
    result.push_back(Pair("payload_expiry_days", DIONS_PAYLOAD_EXPIRY_DAYS));

    // Data layer stats
    if (g_data_layer) {
        uint64_t total_payloads, total_bytes, expired_count;
        g_data_layer->GetStats(total_payloads, total_bytes, expired_count);
        result.push_back(Pair("data_layer_type", g_data_layer->GetTypeName()));
        result.push_back(Pair("payloads_stored", static_cast<int64_t>(total_payloads)));
        result.push_back(Pair("bytes_stored", static_cast<int64_t>(total_bytes)));
        result.push_back(Pair("expired_pending_gc", static_cast<int64_t>(expired_count)));
    } else {
        result.push_back(Pair("data_layer_type", "not_initialized"));
    }

    // TODO: Add DB stats when connected

    return result;
}

//-----------------------------------------------------------------------------
// getdionsgcstats - Get garbage collection statistics
//-----------------------------------------------------------------------------
Value getdionsgcstats(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "getdionsgcstats\n"
            "Get DIONS 2.0 garbage collection statistics.\n"
            "\nResult:\n"
            "{\n"
            "  \"enabled\": true|false,\n"
            "  \"prune_interval_blocks\": n,\n"
            "  \"max_prune_per_run\": n,\n"
            "  \"total_payloads_pruned\": n,\n"
            "  \"total_bytes_reclaimed\": n,\n"
            "  \"last_prune_height\": n,\n"
            "  \"last_prune_time\": n,\n"
            "  \"runs_completed\": n\n"
            "}\n"
        );

    GCConfig config = GetGCConfig();
    GCStats stats = GetGCStats();

    Object result;
    result.push_back(Pair("enabled", config.enabled));
    result.push_back(Pair("prune_interval_blocks", static_cast<int>(config.prune_interval_blocks)));
    result.push_back(Pair("max_prune_per_run", static_cast<int>(config.max_prune_per_run)));
    result.push_back(Pair("total_payloads_pruned", static_cast<int64_t>(stats.total_payloads_pruned)));
    result.push_back(Pair("total_bytes_reclaimed", static_cast<int64_t>(stats.total_bytes_reclaimed)));
    result.push_back(Pair("last_prune_height", static_cast<int>(stats.last_prune_height)));
    result.push_back(Pair("last_prune_time", stats.last_prune_time));
    result.push_back(Pair("runs_completed", static_cast<int>(stats.runs_completed)));

    return result;
}

//-----------------------------------------------------------------------------
// forcedionsgc - Force a garbage collection run
//-----------------------------------------------------------------------------
Value forcedionsgc(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "forcedionsgc\n"
            "Force a DIONS 2.0 garbage collection run.\n"
            "Useful for testing or manual maintenance.\n"
            "\nResult:\n"
            "{\n"
            "  \"payloads_pruned\": n,\n"
            "  \"success\": true|false\n"
            "}\n"
        );

    int64_t current_time = GetTime();
    uint32_t pruned = ForceGC(current_time);

    Object result;
    result.push_back(Pair("payloads_pruned", static_cast<int>(pruned)));
    result.push_back(Pair("success", true));
    result.push_back(Pair("timestamp", current_time));

    return result;
}

//-----------------------------------------------------------------------------
// RPC Registration
//-----------------------------------------------------------------------------

// RPC command table entry structure (matches bitcoinrpc.h)
static const CRPCCommand dions2Commands[] = {
    // DIONS 2.0 Anchor RPCs
    { "getdionsanchor",    &getdionsanchor,    false },
    { "getdionsproof",     &getdionsproof,     false },
    { "verifydionsproof",  &verifydionsproof,  false },
    { "getdionsquota",     &getdionsquota,     false },
    { "getdionstier",      &getdionstier,      false },
    { "getpayloadmode",    &getpayloadmode,    false },
    { "getdionsstats",     &getdionsstats,     false },
    { "getdionsgcstats",   &getdionsgcstats,   false },
    { "forcedionsgc",      &forcedionsgc,      false },
};

void RegisterDions2RPCs()
{
    // Note: In a full implementation, this would add to the global RPC table
    // For Phase 0, the commands are declared but need to be wired into
    // the existing RPC infrastructure in bitcoinrpc.cpp
}
