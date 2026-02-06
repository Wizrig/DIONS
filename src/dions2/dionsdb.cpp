// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 - Database Implementation

#include "dionsdb.h"
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace dions2 {

namespace {

// Helper: convert array to hex string
std::string ArrayToHex(const std::array<uint8_t, 32>& arr) {
    std::ostringstream oss;
    for (uint8_t byte : arr) {
        oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

// Helper: convert hex string to array
bool HexToArray(const std::string& hex, std::array<uint8_t, 32>& out) {
    if (hex.size() != 64) return false;
    for (size_t i = 0; i < 32; i++) {
        unsigned int byte;
        std::istringstream iss(hex.substr(i * 2, 2));
        iss >> std::hex >> byte;
        out[i] = static_cast<uint8_t>(byte);
    }
    return true;
}

// Build anchor key: A + anchor_id
std::string MakeAnchorKey(const std::array<uint8_t, 32>& anchor_id) {
    std::string key;
    key.push_back(db_prefix::ANCHOR);
    key.append(reinterpret_cast<const char*>(anchor_id.data()), 32);
    return key;
}

// Build channel index key: C + channel_id + timestamp (big-endian for ordering)
std::string MakeChannelIndexKey(const std::array<uint8_t, 32>& channel_id, int64_t timestamp) {
    std::string key;
    key.push_back(db_prefix::ANCHOR_BY_CHANNEL);
    key.append(reinterpret_cast<const char*>(channel_id.data()), 32);
    // Big-endian timestamp for lexicographic ordering
    for (int i = 7; i >= 0; i--) {
        key.push_back(static_cast<char>((timestamp >> (i * 8)) & 0xFF));
    }
    return key;
}

// Build payload commitment key: P + payload_hash
std::string MakePayloadKey(const std::array<uint8_t, 32>& payload_hash) {
    std::string key;
    key.push_back(db_prefix::PAYLOAD_COMMIT);
    key.append(reinterpret_cast<const char*>(payload_hash.data()), 32);
    return key;
}

// Build expiry index key: E + expires_at (big-endian) + payload_hash
std::string MakeExpiryKey(int64_t expires_at, const std::array<uint8_t, 32>& payload_hash) {
    std::string key;
    key.push_back(db_prefix::EXPIRY_INDEX);
    for (int i = 7; i >= 0; i--) {
        key.push_back(static_cast<char>((expires_at >> (i * 8)) & 0xFF));
    }
    key.append(reinterpret_cast<const char*>(payload_hash.data()), 32);
    return key;
}

// Build quota key: Q + address + month
std::string MakeQuotaKey(const std::string& address, const std::string& month) {
    std::string key;
    key.push_back(db_prefix::QUOTA);
    key.append(address);
    key.push_back(':');
    key.append(month);
    return key;
}

// Simple serialization to string
template<typename T>
std::string SerializeToString(const T& obj) {
    std::ostringstream oss;
    obj.Serialize(oss);
    return oss.str();
}

// Simple deserialization from string
template<typename T>
bool DeserializeFromString(const std::string& data, T& obj) {
    std::istringstream iss(data);
    try {
        obj.Unserialize(iss);
        return true;
    } catch (...) {
        return false;
    }
}

} // anonymous namespace

//-----------------------------------------------------------------------------
// Anchor operations
//-----------------------------------------------------------------------------

bool WriteAnchor(leveldb::DB* db, const CAnchorCommitment& anchor) {
    if (!db) return false;

    leveldb::WriteBatch batch;

    // Write anchor data
    std::string key = MakeAnchorKey(anchor.anchor_id);
    std::string value = SerializeToString(anchor);
    batch.Put(key, value);

    // Write channel index
    std::string index_key = MakeChannelIndexKey(anchor.anchor_data.channel_id,
                                                 anchor.anchor_data.timestamp);
    batch.Put(index_key, std::string(reinterpret_cast<const char*>(anchor.anchor_id.data()), 32));

    return db->Write(leveldb::WriteOptions(), &batch).ok();
}

bool ReadAnchor(leveldb::DB* db, const std::array<uint8_t, 32>& anchor_id, CAnchorCommitment& out) {
    if (!db) return false;

    std::string key = MakeAnchorKey(anchor_id);
    std::string value;

    if (!db->Get(leveldb::ReadOptions(), key, &value).ok()) {
        return false;
    }

    return DeserializeFromString(value, out);
}

bool GetAnchorsForChannel(leveldb::DB* db, const std::array<uint8_t, 32>& channel_id,
                          int64_t start_time, int64_t end_time,
                          std::vector<CAnchorCommitment>& out) {
    if (!db) return false;

    out.clear();

    std::string start_key = MakeChannelIndexKey(channel_id, start_time);
    std::string end_key = MakeChannelIndexKey(channel_id, end_time);

    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
    for (it->Seek(start_key); it->Valid(); it->Next()) {
        std::string key = it->key().ToString();
        if (key > end_key) break;

        // Extract anchor_id from value
        if (it->value().size() != 32) continue;

        std::array<uint8_t, 32> anchor_id;
        memcpy(anchor_id.data(), it->value().data(), 32);

        CAnchorCommitment anchor;
        if (ReadAnchor(db, anchor_id, anchor)) {
            out.push_back(anchor);
        }
    }
    delete it;

    return true;
}

//-----------------------------------------------------------------------------
// Payload commitment operations
//-----------------------------------------------------------------------------

bool WritePayloadCommitment(leveldb::DB* db, const CPayloadCommitment& commitment) {
    if (!db) return false;

    leveldb::WriteBatch batch;

    // Write payload commitment
    std::string key = MakePayloadKey(commitment.payload_hash);
    std::string value = SerializeToString(commitment);
    batch.Put(key, value);

    // Write expiry index
    if (commitment.expires_at > 0 && !commitment.is_expired) {
        std::string expiry_key = MakeExpiryKey(commitment.expires_at, commitment.payload_hash);
        batch.Put(expiry_key, "");
    }

    return db->Write(leveldb::WriteOptions(), &batch).ok();
}

bool ReadPayloadCommitment(leveldb::DB* db, const std::array<uint8_t, 32>& payload_hash,
                           CPayloadCommitment& out) {
    if (!db) return false;

    std::string key = MakePayloadKey(payload_hash);
    std::string value;

    if (!db->Get(leveldb::ReadOptions(), key, &value).ok()) {
        return false;
    }

    return DeserializeFromString(value, out);
}

bool MarkPayloadExpired(leveldb::DB* db, const std::array<uint8_t, 32>& payload_hash) {
    if (!db) return false;

    CPayloadCommitment commitment;
    if (!ReadPayloadCommitment(db, payload_hash, commitment)) {
        return false;
    }

    leveldb::WriteBatch batch;

    // Update commitment
    commitment.is_expired = true;
    commitment.is_stored = false;
    std::string key = MakePayloadKey(payload_hash);
    std::string value = SerializeToString(commitment);
    batch.Put(key, value);

    // Remove from expiry index
    std::string expiry_key = MakeExpiryKey(commitment.expires_at, payload_hash);
    batch.Delete(expiry_key);

    return db->Write(leveldb::WriteOptions(), &batch).ok();
}

//-----------------------------------------------------------------------------
// Quota operations
//-----------------------------------------------------------------------------

bool WriteQuota(leveldb::DB* db, const std::string& address, const std::string& month,
                const DionsQuota& quota) {
    if (!db) return false;

    std::string key = MakeQuotaKey(address, month);
    std::string value = SerializeToString(quota);

    return db->Put(leveldb::WriteOptions(), key, value).ok();
}

bool ReadQuota(leveldb::DB* db, const std::string& address, const std::string& month,
               DionsQuota& out) {
    if (!db) return false;

    std::string key = MakeQuotaKey(address, month);
    std::string value;

    if (!db->Get(leveldb::ReadOptions(), key, &value).ok()) {
        // Initialize empty quota
        out = DionsQuota();
        out.address = address;
        out.month = month;
        return true;
    }

    return DeserializeFromString(value, out);
}

bool IncrementQuota(leveldb::DB* db, const std::string& address, const std::string& month,
                    uint32_t messages, uint64_t bytes) {
    if (!db) return false;

    DionsQuota quota;
    ReadQuota(db, address, month, quota);

    quota.messages_used += messages;
    quota.bytes_used += bytes;
    quota.last_updated = time(nullptr);

    return WriteQuota(db, address, month, quota);
}

//-----------------------------------------------------------------------------
// Expiry operations
//-----------------------------------------------------------------------------

std::vector<std::array<uint8_t, 32>> GetExpiringPayloads(leveldb::DB* db, int64_t before_time,
                                                          uint32_t limit) {
    std::vector<std::array<uint8_t, 32>> result;
    if (!db) return result;

    std::string start_key;
    start_key.push_back(db_prefix::EXPIRY_INDEX);

    std::string end_key = MakeExpiryKey(before_time, std::array<uint8_t, 32>{});

    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
    for (it->Seek(start_key); it->Valid() && result.size() < limit; it->Next()) {
        std::string key = it->key().ToString();
        if (key.empty() || key[0] != db_prefix::EXPIRY_INDEX) break;
        if (key > end_key) break;

        // Extract payload_hash from key (last 32 bytes)
        if (key.size() >= 1 + 8 + 32) {
            std::array<uint8_t, 32> payload_hash;
            memcpy(payload_hash.data(), key.data() + 1 + 8, 32);
            result.push_back(payload_hash);
        }
    }
    delete it;

    return result;
}

uint32_t PruneExpiredPayloadCommitments(leveldb::DB* db, int64_t current_time) {
    if (!db) return 0;

    uint32_t pruned = 0;
    auto expiring = GetExpiringPayloads(db, current_time, 10000);

    for (const auto& payload_hash : expiring) {
        if (MarkPayloadExpired(db, payload_hash)) {
            pruned++;
        }
    }

    return pruned;
}

//-----------------------------------------------------------------------------
// Statistics
//-----------------------------------------------------------------------------

DionsDbStats GetDionsDbStats(leveldb::DB* db) {
    DionsDbStats stats = {0, 0, 0, 0};
    if (!db) return stats;

    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string key = it->key().ToString();
        if (key.empty()) continue;

        switch (key[0]) {
            case db_prefix::ANCHOR:
                stats.total_anchors++;
                break;
            case db_prefix::PAYLOAD_COMMIT:
                stats.total_payload_commitments++;
                {
                    CPayloadCommitment commit;
                    if (DeserializeFromString(it->value().ToString(), commit)) {
                        if (commit.is_expired) {
                            stats.expired_payload_commitments++;
                        }
                    }
                }
                break;
            case db_prefix::QUOTA:
                stats.quota_entries++;
                break;
        }
    }
    delete it;

    return stats;
}

} // namespace dions2
