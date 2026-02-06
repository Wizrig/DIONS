// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 - RPC Methods for Anchors, Proofs, and Quotas

#include "anchor.h"
#include "stakecheck.h"
#include "datalayer.h"
#include "dionsdb.h"
#include "gc.h"
#include "hybrid_sig.h"
#include "evm.h"
#include "svm.h"
#include "evmc_host.h"
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
        int64_t stake;
        if (params[0].type() == str_type) {
            stake = std::stoll(params[0].get_str());
        } else {
            stake = params[0].get_int64();
        }
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
// gethybridsigschemes - List available hybrid signature schemes
//-----------------------------------------------------------------------------
Value gethybridsigschemes(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "gethybridsigschemes\n"
            "List available DIONS 2.0 hybrid signature schemes.\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"id\": n,\n"
            "    \"name\": \"xxx\",\n"
            "    \"type\": \"classical|pqc|hybrid\",\n"
            "    \"quantum_resistant\": true|false\n"
            "  },\n"
            "  ...\n"
            "]\n"
        );

    Array schemes;

    // List all signature schemes
    struct SchemeInfo {
        SignatureScheme scheme;
        const char* name;
        const char* type;
    };

    static const SchemeInfo scheme_list[] = {
        { SignatureScheme::ECDSA_SECP256K1, "ecdsa_secp256k1", "classical" },
        { SignatureScheme::ED25519, "ed25519", "classical" },
        { SignatureScheme::FALCON512, "falcon512", "pqc" },
        { SignatureScheme::FALCON1024, "falcon1024", "pqc" },
        { SignatureScheme::DILITHIUM2, "dilithium2", "pqc" },
        { SignatureScheme::DILITHIUM3, "dilithium3", "pqc" },
        { SignatureScheme::DILITHIUM5, "dilithium5", "pqc" },
        { SignatureScheme::HYBRID_ED25519_FALCON512, "hybrid_ed25519_falcon512", "hybrid" },
        { SignatureScheme::HYBRID_ED25519_DILITHIUM3, "hybrid_ed25519_dilithium3", "hybrid" },
        { SignatureScheme::HYBRID_ECDSA_FALCON512, "hybrid_ecdsa_falcon512", "hybrid" },
        { SignatureScheme::HYBRID_ECDSA_DILITHIUM3, "hybrid_ecdsa_dilithium3", "hybrid" },
    };

    for (const auto& s : scheme_list) {
        Object scheme_obj;
        scheme_obj.push_back(Pair("id", static_cast<int>(s.scheme)));
        scheme_obj.push_back(Pair("name", s.name));
        scheme_obj.push_back(Pair("type", s.type));
        scheme_obj.push_back(Pair("quantum_resistant", HybridSigner::IsQuantumResistant(s.scheme)));
        schemes.push_back(scheme_obj);
    }

    return schemes;
}

//-----------------------------------------------------------------------------
// getrecommendedsigscheme - Get recommended scheme for device profile
//-----------------------------------------------------------------------------
Value getrecommendedsigscheme(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw std::runtime_error(
            "getrecommendedsigscheme [device_profile]\n"
            "Get recommended hybrid signature scheme for a device profile.\n"
            "\nArguments:\n"
            "1. device_profile (string, optional) One of: iot_minimal, iot_standard,\n"
            "                   robot_standard, robot_premium, server, paranoid\n"
            "                   Default: server\n"
            "\nResult:\n"
            "{\n"
            "  \"profile\": \"xxx\",\n"
            "  \"recommended_scheme\": \"xxx\",\n"
            "  \"scheme_id\": n,\n"
            "  \"quantum_resistant\": true|false\n"
            "}\n"
        );

    std::string profile_name = "server";
    if (params.size() > 0)
        profile_name = params[0].get_str();

    pqc::DeviceProfile profile = pqc::DeviceProfile::SERVER;
    if (profile_name == "iot_minimal") profile = pqc::DeviceProfile::IOT_MINIMAL;
    else if (profile_name == "iot_standard") profile = pqc::DeviceProfile::IOT_STANDARD;
    else if (profile_name == "robot_standard") profile = pqc::DeviceProfile::ROBOT_STANDARD;
    else if (profile_name == "robot_premium") profile = pqc::DeviceProfile::ROBOT_PREMIUM;
    else if (profile_name == "server") profile = pqc::DeviceProfile::SERVER;
    else if (profile_name == "paranoid") profile = pqc::DeviceProfile::PARANOID;

    SignatureScheme scheme = HybridSigner::GetRecommendedScheme(profile);

    Object result;
    result.push_back(Pair("profile", profile_name));
    result.push_back(Pair("recommended_scheme", HybridSigner::SchemeName(scheme)));
    result.push_back(Pair("scheme_id", static_cast<int>(scheme)));
    result.push_back(Pair("quantum_resistant", HybridSigner::IsQuantumResistant(scheme)));

    return result;
}

//-----------------------------------------------------------------------------
// EVM Zone RPCs
//-----------------------------------------------------------------------------

// Global EVM executor instance (Phase 0 - in-memory only)
static std::unique_ptr<EVMExecutor> g_evm_executor;

Value getevmstats(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "getevmstats\n"
            "Get EVM zone statistics.\n"
            "\nResult:\n"
            "{\n"
            "  \"enabled\": true|false,\n"
            "  \"account_count\": n,\n"
            "  \"status\": \"xxx\"\n"
            "}\n"
        );

    Object result;
    result.push_back(Pair("enabled", true));

    if (!g_evm_executor) {
        g_evm_executor = std::make_unique<EVMExecutor>();
    }

    result.push_back(Pair("account_count", static_cast<int>(g_evm_executor->GetAccountCount())));
    result.push_back(Pair("status", "phase0_state_only"));
    result.push_back(Pair("evmone_integrated", false));
    result.push_back(Pair("note", "EVM bytecode execution requires evmone integration"));

    return result;
}

Value createevmaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw std::runtime_error(
            "createevmaccount <address> [balance_hex]\n"
            "Create an EVM account in the DIONS 2.0 EVM zone.\n"
            "\nArguments:\n"
            "1. address     (string, required) 20-byte EVM address (hex with 0x prefix)\n"
            "2. balance_hex (string, optional) Initial balance as hex (default: 0)\n"
            "\nResult:\n"
            "{\n"
            "  \"success\": true|false,\n"
            "  \"address\": \"0x...\"\n"
            "}\n"
        );

    if (!g_evm_executor) {
        g_evm_executor = std::make_unique<EVMExecutor>();
    }

    std::string addr_str = params[0].get_str();
    std::vector<uint8_t> address = HexToBytes(addr_str);

    std::vector<uint8_t> balance;
    if (params.size() > 1) {
        balance = HexToBytes(params[1].get_str());
    }

    bool success = g_evm_executor->CreateAccount(address, balance);

    Object result;
    result.push_back(Pair("success", success));
    result.push_back(Pair("address", BytesToHex(address)));

    return result;
}

Value getevmbalance(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw std::runtime_error(
            "getevmbalance <address>\n"
            "Get EVM account balance.\n"
            "\nArguments:\n"
            "1. address (string, required) 20-byte EVM address (hex with 0x prefix)\n"
            "\nResult:\n"
            "{\n"
            "  \"address\": \"0x...\",\n"
            "  \"balance\": \"0x...\"\n"
            "}\n"
        );

    if (!g_evm_executor) {
        g_evm_executor = std::make_unique<EVMExecutor>();
    }

    std::vector<uint8_t> address = HexToBytes(params[0].get_str());
    std::vector<uint8_t> balance = g_evm_executor->GetBalance(address);

    Object result;
    result.push_back(Pair("address", BytesToHex(address)));
    result.push_back(Pair("balance", BytesToHex(balance)));
    result.push_back(Pair("exists", g_evm_executor->AccountExists(address)));

    return result;
}

//-----------------------------------------------------------------------------
// SVM Zone RPCs
//-----------------------------------------------------------------------------

// Global SVM executor instance (Phase 0 - in-memory only)
static std::unique_ptr<SVMExecutor> g_svm_executor;

Value getsvmstats(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "getsvmstats\n"
            "Get SVM (Solana VM) zone statistics.\n"
            "\nResult:\n"
            "{\n"
            "  \"enabled\": true|false,\n"
            "  \"account_count\": n,\n"
            "  \"slot\": n,\n"
            "  \"compute_budget\": n,\n"
            "  \"status\": \"xxx\"\n"
            "}\n"
        );

    Object result;
    result.push_back(Pair("enabled", true));

    if (!g_svm_executor) {
        g_svm_executor = std::make_unique<SVMExecutor>();
    }

    result.push_back(Pair("account_count", static_cast<int>(g_svm_executor->GetAccountCount())));
    result.push_back(Pair("slot", static_cast<int64_t>(g_svm_executor->GetSlot())));
    result.push_back(Pair("compute_budget", static_cast<int64_t>(g_svm_executor->GetComputeBudget())));
    result.push_back(Pair("status", "phase0_state_only"));
    result.push_back(Pair("bpf_runtime_integrated", false));
    result.push_back(Pair("note", "SVM bytecode execution requires BPF runtime integration"));

    return result;
}

Value createsvmaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw std::runtime_error(
            "createsvmaccount <pubkey_hex> [lamports]\n"
            "Create an SVM (Solana-style) account in the DIONS 2.0 SVM zone.\n"
            "\nArguments:\n"
            "1. pubkey_hex (string, required) 32-byte public key (hex with 0x prefix)\n"
            "2. lamports   (numeric, optional) Initial balance in lamports (default: 0)\n"
            "\nResult:\n"
            "{\n"
            "  \"success\": true|false,\n"
            "  \"pubkey\": \"0x...\"\n"
            "}\n"
        );

    if (!g_svm_executor) {
        g_svm_executor = std::make_unique<SVMExecutor>();
    }

    std::string pubkey_str = params[0].get_str();
    std::vector<uint8_t> pubkey_bytes = HexToBytes(pubkey_str);

    if (pubkey_bytes.size() != 32) {
        throw JSONRPCError(RPC_INVALID_PARAMS, "Public key must be 32 bytes");
    }

    SolanaPublicKey pubkey = BytesToPublicKey(pubkey_bytes);

    uint64_t lamports = 0;
    if (params.size() > 1) {
        if (params[1].type() == str_type) {
            lamports = std::stoull(params[1].get_str());
        } else {
            lamports = params[1].get_int64();
        }
    }

    bool success = g_svm_executor->CreateAccount(pubkey, lamports);

    Object result;
    result.push_back(Pair("success", success));
    result.push_back(Pair("pubkey", PublicKeyToString(pubkey)));
    result.push_back(Pair("lamports", static_cast<int64_t>(lamports)));

    return result;
}

Value getsvmbalance(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw std::runtime_error(
            "getsvmbalance <pubkey_hex>\n"
            "Get SVM account balance in lamports.\n"
            "\nArguments:\n"
            "1. pubkey_hex (string, required) 32-byte public key (hex with 0x prefix)\n"
            "\nResult:\n"
            "{\n"
            "  \"pubkey\": \"0x...\",\n"
            "  \"lamports\": n,\n"
            "  \"exists\": true|false\n"
            "}\n"
        );

    if (!g_svm_executor) {
        g_svm_executor = std::make_unique<SVMExecutor>();
    }

    std::vector<uint8_t> pubkey_bytes = HexToBytes(params[0].get_str());

    if (pubkey_bytes.size() != 32) {
        throw JSONRPCError(RPC_INVALID_PARAMS, "Public key must be 32 bytes");
    }

    SolanaPublicKey pubkey = BytesToPublicKey(pubkey_bytes);
    uint64_t lamports = g_svm_executor->GetBalance(pubkey);

    Object result;
    result.push_back(Pair("pubkey", PublicKeyToString(pubkey)));
    result.push_back(Pair("lamports", static_cast<int64_t>(lamports)));
    result.push_back(Pair("exists", g_svm_executor->AccountExists(pubkey)));

    return result;
}

Value getsvmrentexemption(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw std::runtime_error(
            "getsvmrentexemption <data_size>\n"
            "Calculate rent exemption amount for given data size.\n"
            "\nArguments:\n"
            "1. data_size (numeric, required) Account data size in bytes\n"
            "\nResult:\n"
            "{\n"
            "  \"data_size\": n,\n"
            "  \"rent_exemption_lamports\": n\n"
            "}\n"
        );

    if (!g_svm_executor) {
        g_svm_executor = std::make_unique<SVMExecutor>();
    }

    size_t data_size;
    if (params[0].type() == str_type) {
        data_size = static_cast<size_t>(std::stoull(params[0].get_str()));
    } else {
        data_size = static_cast<size_t>(params[0].get_int64());
    }
    uint64_t rent_exemption = g_svm_executor->CalculateRentExemption(data_size);

    Object result;
    result.push_back(Pair("data_size", static_cast<int64_t>(data_size)));
    result.push_back(Pair("rent_exemption_lamports", static_cast<int64_t>(rent_exemption)));

    return result;
}

//-----------------------------------------------------------------------------
// executeevm - Execute EVM bytecode using evmone
//-----------------------------------------------------------------------------
Value executeevm(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 3)
        throw std::runtime_error(
            "executeevm <bytecode_hex> [gas_limit] [sender_address]\n"
            "Execute EVM bytecode using evmone.\n"
            "\nArguments:\n"
            "1. bytecode_hex    (string, required) EVM bytecode as hex (with 0x prefix)\n"
            "2. gas_limit       (numeric, optional) Gas limit (default: 1000000)\n"
            "3. sender_address  (string, optional) Sender address (default: zero address)\n"
            "\nResult:\n"
            "{\n"
            "  \"success\": true|false,\n"
            "  \"status\": \"xxx\",\n"
            "  \"gas_used\": n,\n"
            "  \"output\": \"0x...\",\n"
            "  \"error\": \"xxx\" (if failed)\n"
            "}\n"
        );

    // Check if evmone is available
    if (!IsEvmoneAvailable()) {
        Object result;
        result.push_back(Pair("success", false));
        result.push_back(Pair("error", "evmone not available - compile with HAVE_EVMONE"));
        return result;
    }

    // Initialize EVM executor if needed
    if (!g_evm_executor) {
        g_evm_executor = std::make_unique<EVMExecutor>();
    }

    // Parse bytecode
    std::vector<uint8_t> bytecode = HexToBytes(params[0].get_str());

    // Parse gas limit
    int64_t gas_limit = 1000000;
    if (params.size() > 1) {
        gas_limit = params[1].get_int64();
    }

    // Parse sender address
    std::vector<uint8_t> sender(20, 0);
    if (params.size() > 2) {
        sender = HexToBytes(params[2].get_str());
    }

    // Create EVM context
    EVMCHostContext ctx(g_evm_executor.get());

    // Create message
    evmc_message msg;
    std::memset(&msg, 0, sizeof(msg));
    msg.kind = EVMC_CALL;
    msg.gas = gas_limit;
    std::memcpy(msg.sender.bytes, sender.data(), std::min(sender.size(), size_t(20)));

    // Create VM and execute
    evmc_vm* vm = EVMCHost::CreateVM();
    if (!vm) {
        Object result;
        result.push_back(Pair("success", false));
        result.push_back(Pair("error", "Failed to create evmone VM instance"));
        return result;
    }

    EVMResult evm_result = EVMCHost::Execute(
        vm,
        &ctx,
        EVMC_CANCUN,  // Use latest stable revision
        &msg,
        bytecode.data(),
        bytecode.size()
    );

    EVMCHost::DestroyVM(vm);

    // Build response
    Object result;
    result.push_back(Pair("success", evm_result.IsSuccess()));

    const char* status_str = "unknown";
    switch (evm_result.status) {
        case EVMResult::SUCCESS: status_str = "success"; break;
        case EVMResult::REVERT: status_str = "revert"; break;
        case EVMResult::OUT_OF_GAS: status_str = "out_of_gas"; break;
        case EVMResult::INVALID_INSTRUCTION: status_str = "invalid_instruction"; break;
        case EVMResult::UNDEFINED_INSTRUCTION: status_str = "undefined_instruction"; break;
        case EVMResult::STACK_OVERFLOW: status_str = "stack_overflow"; break;
        case EVMResult::STACK_UNDERFLOW: status_str = "stack_underflow"; break;
        case EVMResult::BAD_JUMP_DESTINATION: status_str = "bad_jump_destination"; break;
        case EVMResult::INVALID_MEMORY_ACCESS: status_str = "invalid_memory_access"; break;
        case EVMResult::CALL_DEPTH_EXCEEDED: status_str = "call_depth_exceeded"; break;
        case EVMResult::STATIC_MODE_VIOLATION: status_str = "static_mode_violation"; break;
        case EVMResult::PRECOMPILE_FAILURE: status_str = "precompile_failure"; break;
        case EVMResult::CONTRACT_VALIDATION_FAILURE: status_str = "contract_validation_failure"; break;
        case EVMResult::ARGUMENT_OUT_OF_RANGE: status_str = "argument_out_of_range"; break;
        case EVMResult::INSUFFICIENT_BALANCE: status_str = "insufficient_balance"; break;
        case EVMResult::INTERNAL_ERROR: status_str = "internal_error"; break;
    }
    result.push_back(Pair("status", status_str));
    result.push_back(Pair("gas_used", static_cast<int64_t>(evm_result.gas_used)));

    if (!evm_result.output.empty()) {
        result.push_back(Pair("output", BytesToHex(evm_result.output)));
    }

    if (!evm_result.error_message.empty()) {
        result.push_back(Pair("error", evm_result.error_message));
    }

    return result;
}

//-----------------------------------------------------------------------------
// RPC Registration
//-----------------------------------------------------------------------------

// RPC command table entry structure (matches bitcoinrpc.h)
static const CRPCCommand dions2Commands[] = {
    // DIONS 2.0 Anchor RPCs
    { "getdionsanchor",         &getdionsanchor,         false },
    { "getdionsproof",          &getdionsproof,          false },
    { "verifydionsproof",       &verifydionsproof,       false },
    { "getdionsquota",          &getdionsquota,          false },
    { "getdionstier",           &getdionstier,           false },
    { "getpayloadmode",         &getpayloadmode,         false },
    { "getdionsstats",          &getdionsstats,          false },
    { "getdionsgcstats",        &getdionsgcstats,        false },
    { "forcedionsgc",           &forcedionsgc,           false },
    { "gethybridsigschemes",    &gethybridsigschemes,    false },
    { "getrecommendedsigscheme",&getrecommendedsigscheme,false },
    // EVM Zone RPCs
    { "getevmstats",            &getevmstats,            false },
    { "createevmaccount",       &createevmaccount,       false },
    { "getevmbalance",          &getevmbalance,          false },
    // SVM Zone RPCs
    { "getsvmstats",            &getsvmstats,            false },
    { "createsvmaccount",       &createsvmaccount,       false },
    { "getsvmbalance",          &getsvmbalance,          false },
    { "getsvmrentexemption",    &getsvmrentexemption,    false },
    // EVM Execution RPC
    { "executeevm",             &executeevm,             false },
};

void RegisterDions2RPCs()
{
    // Note: In a full implementation, this would add to the global RPC table
    // For Phase 0, the commands are declared but need to be wired into
    // the existing RPC infrastructure in bitcoinrpc.cpp
}
