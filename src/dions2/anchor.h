// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 - Anchor Transaction Types and Merkle Tree
// L1 anchors commitments; bulk payload + execution happen off-L1

#ifndef DIONS2_ANCHOR_H
#define DIONS2_ANCHOR_H

#include <cstdint>
#include <vector>
#include <string>
#include <array>

// Forward declarations for types that may be defined elsewhere
class uint256;

namespace dions2 {

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
constexpr int64_t DIONS_MIN_STAKE = 1000;           // 1,000 IOC minimum
constexpr int64_t DIONS_MIN_STAKE_AGE = 86400;      // 24 hours in seconds
constexpr int32_t DIONS_PAYLOAD_EXPIRY_DAYS = 30;   // Payloads auto-delete after 30 days
constexpr int64_t DIONS_PAYLOAD_EXPIRY_SECONDS = DIONS_PAYLOAD_EXPIRY_DAYS * 24 * 60 * 60;

//-----------------------------------------------------------------------------
// Stake Tiers - determines monthly quotas
//-----------------------------------------------------------------------------
enum class DionsTier : int {
    NONE = 0,
    BASIC = 1,        // 1,000 - 4,999 IOC
    STANDARD = 2,     // 5,000 - 9,999 IOC
    PREMIUM = 3,      // 10,000 - 49,999 IOC
    ENTERPRISE = 4,   // 50,000 - 99,999 IOC
    UNLIMITED = 5     // 100,000+ IOC
};

struct TierLimits {
    int64_t min_stake;
    uint32_t monthly_messages;
    uint64_t monthly_bytes;
    uint32_t max_batch_size;

    static TierLimits GetLimits(DionsTier tier);
};

//-----------------------------------------------------------------------------
// Anchor Types
//-----------------------------------------------------------------------------
enum class AnchorKind : uint8_t {
    MESSAGE = 0x01,     // Encrypted messaging
    REGISTER = 0x02,    // Name registration
    UPDATE = 0x03,      // Name update
    TRANSFER = 0x04,    // Name transfer
    DATA = 0x05,        // Generic data anchor
    BATCH = 0x10        // Batched operations
};

//-----------------------------------------------------------------------------
// Availability Certificate - proves payload is stored by k-of-n storage nodes
//-----------------------------------------------------------------------------
struct AvailabilityCert {
    uint32_t threshold_k;                    // Required signatures
    uint32_t total_n;                        // Total storage nodes
    std::vector<std::array<uint8_t, 33>> pubkeys;  // Compressed pubkeys
    std::vector<std::array<uint8_t, 64>> signatures; // Schnorr signatures

    bool IsValid() const { return signatures.size() >= threshold_k; }

    // Serialization - manual for std::array which lacks stream operators
    template<typename Stream>
    void Serialize(Stream& s) const {
        s << threshold_k << total_n;
        // Write pubkeys
        uint32_t pubkey_count = static_cast<uint32_t>(pubkeys.size());
        s << pubkey_count;
        for (const auto& pk : pubkeys) {
            s.write(reinterpret_cast<const char*>(pk.data()), 33);
        }
        // Write signatures
        uint32_t sig_count = static_cast<uint32_t>(signatures.size());
        s << sig_count;
        for (const auto& sig : signatures) {
            s.write(reinterpret_cast<const char*>(sig.data()), 64);
        }
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        s >> threshold_k >> total_n;
        // Read pubkeys
        uint32_t pubkey_count;
        s >> pubkey_count;
        pubkeys.resize(pubkey_count);
        for (auto& pk : pubkeys) {
            s.read(reinterpret_cast<char*>(pk.data()), 33);
        }
        // Read signatures
        uint32_t sig_count;
        s >> sig_count;
        signatures.resize(sig_count);
        for (auto& sig : signatures) {
            s.read(reinterpret_cast<char*>(sig.data()), 64);
        }
    }
};

//-----------------------------------------------------------------------------
// TX_DIONS_ANCHOR - The core on-chain commitment structure
// One anchor can represent thousands of off-chain messages
//-----------------------------------------------------------------------------
struct CDionsAnchor {
    uint8_t version;                 // Protocol version
    AnchorKind kind;                 // Operation type
    uint32_t epoch_id;               // Epoch identifier for batching
    std::array<uint8_t, 32> channel_id;   // Channel/name identifier
    std::array<uint8_t, 32> payload_root; // Merkle root of payload hashes
    uint32_t payload_count;          // Number of payloads in this anchor
    uint32_t meta_flags;             // Metadata flags
    int64_t timestamp;               // Unix timestamp
    AvailabilityCert availability_cert;   // Storage proof
    int64_t total_fees;              // Total fees paid

    CDionsAnchor() : version(1), kind(AnchorKind::MESSAGE), epoch_id(0),
                     payload_count(0), meta_flags(0), timestamp(0), total_fees(0) {
        channel_id.fill(0);
        payload_root.fill(0);
    }

    // Serialization
    template<typename Stream>
    void Serialize(Stream& s) const {
        s << version << static_cast<uint8_t>(kind) << epoch_id;
        s.write(reinterpret_cast<const char*>(channel_id.data()), 32);
        s.write(reinterpret_cast<const char*>(payload_root.data()), 32);
        s << payload_count << meta_flags << timestamp;
        availability_cert.Serialize(s);
        s << total_fees;
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        uint8_t kind_byte;
        s >> version >> kind_byte >> epoch_id;
        kind = static_cast<AnchorKind>(kind_byte);
        s.read(reinterpret_cast<char*>(channel_id.data()), 32);
        s.read(reinterpret_cast<char*>(payload_root.data()), 32);
        s >> payload_count >> meta_flags >> timestamp;
        availability_cert.Unserialize(s);
        s >> total_fees;
    }

    // Get unique anchor ID (hash of anchor data)
    std::array<uint8_t, 32> GetAnchorId() const;
};

//-----------------------------------------------------------------------------
// Merkle Tree for payload batching
//-----------------------------------------------------------------------------
struct MerkleProof {
    std::vector<std::array<uint8_t, 32>> siblings;
    std::vector<bool> path;  // true = right, false = left
    uint32_t leaf_index;

    template<typename Stream>
    void Serialize(Stream& s) const {
        // Write siblings manually (std::array lacks stream operators)
        uint32_t sibling_count = static_cast<uint32_t>(siblings.size());
        s << sibling_count;
        for (const auto& sib : siblings) {
            s.write(reinterpret_cast<const char*>(sib.data()), 32);
        }
        s << leaf_index;
        // Pack path bits into bytes
        std::vector<uint8_t> path_bytes((path.size() + 7) / 8);
        for (size_t i = 0; i < path.size(); i++) {
            if (path[i]) path_bytes[i / 8] |= (1 << (i % 8));
        }
        uint32_t path_bytes_size = static_cast<uint32_t>(path_bytes.size());
        s << path_bytes_size;
        if (!path_bytes.empty()) {
            s.write(reinterpret_cast<const char*>(path_bytes.data()), path_bytes.size());
        }
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        // Read siblings manually
        uint32_t sibling_count;
        s >> sibling_count;
        siblings.resize(sibling_count);
        for (auto& sib : siblings) {
            s.read(reinterpret_cast<char*>(sib.data()), 32);
        }
        s >> leaf_index;
        // Read path bytes
        uint32_t path_bytes_size;
        s >> path_bytes_size;
        std::vector<uint8_t> path_bytes(path_bytes_size);
        if (path_bytes_size > 0) {
            s.read(reinterpret_cast<char*>(path_bytes.data()), path_bytes_size);
        }
        path.resize(siblings.size());
        for (size_t i = 0; i < path.size(); i++) {
            path[i] = (path_bytes[i / 8] >> (i % 8)) & 1;
        }
    }
};

class MerkleTree {
public:
    // Build tree from leaf hashes
    static std::array<uint8_t, 32> Build(const std::vector<std::array<uint8_t, 32>>& leaves);

    // Build tree and return all layers (for proof generation)
    static std::vector<std::vector<std::array<uint8_t, 32>>> BuildLayers(
        const std::vector<std::array<uint8_t, 32>>& leaves);

    // Generate inclusion proof for a leaf
    static MerkleProof GetProof(
        const std::vector<std::vector<std::array<uint8_t, 32>>>& layers,
        uint32_t leaf_index);

    // Verify a Merkle proof
    static bool VerifyProof(
        const std::array<uint8_t, 32>& root,
        const std::array<uint8_t, 32>& leaf,
        const MerkleProof& proof);

private:
    static std::array<uint8_t, 32> HashNodes(
        const std::array<uint8_t, 32>& left,
        const std::array<uint8_t, 32>& right);
};

//-----------------------------------------------------------------------------
// Payload Object - lives off-chain, only hash anchored on L1
//-----------------------------------------------------------------------------
struct PayloadObject {
    std::array<uint8_t, 32> payload_hash;    // SHA256 of encrypted_data
    std::array<uint8_t, 32> anchor_id;       // Reference to on-chain anchor
    std::vector<uint8_t> encrypted_data;     // X25519 + AES-GCM encrypted payload
    int64_t created_at;                      // Unix timestamp
    int64_t expires_at;                      // Auto-delete time
    std::string sender_address;              // Staker address

    bool IsExpired(int64_t current_time) const {
        return current_time >= expires_at;
    }

    template<typename Stream>
    void Serialize(Stream& s) const {
        s.write(reinterpret_cast<const char*>(payload_hash.data()), 32);
        s.write(reinterpret_cast<const char*>(anchor_id.data()), 32);
        s << encrypted_data << created_at << expires_at << sender_address;
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        s.read(reinterpret_cast<char*>(payload_hash.data()), 32);
        s.read(reinterpret_cast<char*>(anchor_id.data()), 32);
        s >> encrypted_data >> created_at >> expires_at >> sender_address;
    }
};

//-----------------------------------------------------------------------------
// Quota tracking
//-----------------------------------------------------------------------------
struct DionsQuota {
    std::string address;
    std::string month;           // YYYY-MM format
    uint32_t messages_used;
    uint64_t bytes_used;
    DionsTier tier;
    int64_t last_updated;

    DionsQuota() : messages_used(0), bytes_used(0), tier(DionsTier::NONE), last_updated(0) {}

    template<typename Stream>
    void Serialize(Stream& s) const {
        s << address << month << messages_used << bytes_used;
        s << static_cast<int>(tier) << last_updated;
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        int tier_int;
        s >> address >> month >> messages_used >> bytes_used >> tier_int >> last_updated;
        tier = static_cast<DionsTier>(tier_int);
    }
};

//-----------------------------------------------------------------------------
// Stake check result
//-----------------------------------------------------------------------------
struct StakeCheckResult {
    bool has_minimum_stake;
    bool has_minimum_age;
    int64_t total_stake;
    int64_t oldest_coin_age;
    DionsTier tier;
    std::string error;
};

//-----------------------------------------------------------------------------
// Payload mode for transition period
//-----------------------------------------------------------------------------
enum class PayloadMode {
    LEGACY,     // Old DIONS behavior (on-chain payloads)
    ANCHOR,     // New anchor-only mode
    HYBRID      // Accept both during transition
};

// Get tier from stake amount
DionsTier GetTierFromStake(int64_t stake_amount);

// Get current month string
std::string GetCurrentMonth();

} // namespace dions2

#endif // DIONS2_ANCHOR_H
