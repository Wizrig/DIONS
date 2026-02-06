// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 - Database Extensions for Anchors and Quotas

#ifndef DIONS2_DIONSDB_H
#define DIONS2_DIONSDB_H

#include "anchor.h"
#include <string>
#include <vector>

// Forward declarations
namespace leveldb {
    class DB;
}

namespace dions2 {

//-----------------------------------------------------------------------------
// Database key prefixes for DIONS 2.0 data
//-----------------------------------------------------------------------------
namespace db_prefix {
    constexpr char ANCHOR = 'A';           // A + anchor_id -> CDionsAnchor
    constexpr char ANCHOR_BY_CHANNEL = 'C'; // C + channel_id + timestamp -> anchor_id
    constexpr char PAYLOAD_COMMIT = 'P';   // P + payload_hash -> CPayloadCommitment
    constexpr char QUOTA = 'Q';            // Q + address + month -> DionsQuota
    constexpr char EXPIRY_INDEX = 'E';     // E + expires_at + payload_hash -> empty
}

//-----------------------------------------------------------------------------
// Anchor commitment stored in DB (references on-chain tx)
//-----------------------------------------------------------------------------
struct CAnchorCommitment {
    std::array<uint8_t, 32> anchor_id;
    std::array<uint8_t, 32> tx_hash;      // On-chain transaction hash
    uint32_t block_height;
    int64_t block_time;
    CDionsAnchor anchor_data;

    CAnchorCommitment() : block_height(0), block_time(0) {
        anchor_id.fill(0);
        tx_hash.fill(0);
    }

    template<typename Stream>
    void Serialize(Stream& s) const {
        s.write(reinterpret_cast<const char*>(anchor_id.data()), 32);
        s.write(reinterpret_cast<const char*>(tx_hash.data()), 32);
        s << block_height << block_time;
        anchor_data.Serialize(s);
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        s.read(reinterpret_cast<char*>(anchor_id.data()), 32);
        s.read(reinterpret_cast<char*>(tx_hash.data()), 32);
        s >> block_height >> block_time;
        anchor_data.Unserialize(s);
    }
};

//-----------------------------------------------------------------------------
// Payload commitment - tracks payload storage status
//-----------------------------------------------------------------------------
struct CPayloadCommitment {
    std::array<uint8_t, 32> payload_hash;
    std::array<uint8_t, 32> anchor_id;
    int64_t created_at;
    int64_t expires_at;
    bool is_stored;             // True if we have the payload locally
    bool is_expired;            // True if past expiry but anchor retained

    CPayloadCommitment() : created_at(0), expires_at(0), is_stored(false), is_expired(false) {
        payload_hash.fill(0);
        anchor_id.fill(0);
    }

    template<typename Stream>
    void Serialize(Stream& s) const {
        s.write(reinterpret_cast<const char*>(payload_hash.data()), 32);
        s.write(reinterpret_cast<const char*>(anchor_id.data()), 32);
        s << created_at << expires_at;
        uint8_t flags = (is_stored ? 1 : 0) | (is_expired ? 2 : 0);
        s << flags;
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        s.read(reinterpret_cast<char*>(payload_hash.data()), 32);
        s.read(reinterpret_cast<char*>(anchor_id.data()), 32);
        s >> created_at >> expires_at;
        uint8_t flags;
        s >> flags;
        is_stored = (flags & 1) != 0;
        is_expired = (flags & 2) != 0;
    }
};

//-----------------------------------------------------------------------------
// Database operations
//-----------------------------------------------------------------------------

/**
 * Write an anchor commitment to the database
 */
bool WriteAnchor(leveldb::DB* db, const CAnchorCommitment& anchor);

/**
 * Read an anchor commitment by ID
 */
bool ReadAnchor(leveldb::DB* db, const std::array<uint8_t, 32>& anchor_id, CAnchorCommitment& out);

/**
 * Get anchors for a channel, ordered by timestamp
 */
bool GetAnchorsForChannel(leveldb::DB* db, const std::array<uint8_t, 32>& channel_id,
                          int64_t start_time, int64_t end_time,
                          std::vector<CAnchorCommitment>& out);

/**
 * Write a payload commitment
 */
bool WritePayloadCommitment(leveldb::DB* db, const CPayloadCommitment& commitment);

/**
 * Read a payload commitment by hash
 */
bool ReadPayloadCommitment(leveldb::DB* db, const std::array<uint8_t, 32>& payload_hash,
                           CPayloadCommitment& out);

/**
 * Mark a payload as expired (keeps commitment, removes from expiry index)
 */
bool MarkPayloadExpired(leveldb::DB* db, const std::array<uint8_t, 32>& payload_hash);

/**
 * Write quota usage for an address/month
 */
bool WriteQuota(leveldb::DB* db, const std::string& address, const std::string& month,
                const DionsQuota& quota);

/**
 * Read quota usage for an address/month
 */
bool ReadQuota(leveldb::DB* db, const std::string& address, const std::string& month,
               DionsQuota& out);

/**
 * Increment quota usage atomically
 */
bool IncrementQuota(leveldb::DB* db, const std::string& address, const std::string& month,
                    uint32_t messages, uint64_t bytes);

/**
 * Get payloads expiring before a given time
 */
std::vector<std::array<uint8_t, 32>> GetExpiringPayloads(leveldb::DB* db, int64_t before_time,
                                                          uint32_t limit = 1000);

/**
 * Prune expired payload commitments from the database
 * Marks them as expired but retains the commitment record
 */
uint32_t PruneExpiredPayloadCommitments(leveldb::DB* db, int64_t current_time);

/**
 * Get database statistics for DIONS 2.0 data
 */
struct DionsDbStats {
    uint64_t total_anchors;
    uint64_t total_payload_commitments;
    uint64_t expired_payload_commitments;
    uint64_t quota_entries;
};

DionsDbStats GetDionsDbStats(leveldb::DB* db);

} // namespace dions2

#endif // DIONS2_DIONSDB_H
