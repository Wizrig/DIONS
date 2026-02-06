// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 - Data Layer Implementation

#include "datalayer.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <cstring>

namespace dions2 {

// Global data layer instance
std::unique_ptr<IDataLayer> g_data_layer;

//-----------------------------------------------------------------------------
// LocalDiskDataLayer implementation
//-----------------------------------------------------------------------------

LocalDiskDataLayer::LocalDiskDataLayer(const std::string& base_path)
    : m_base_path(base_path)
{
    EnsureDirectory(m_base_path);
}

LocalDiskDataLayer::~LocalDiskDataLayer() = default;

std::string LocalDiskDataLayer::HashToHex(const std::array<uint8_t, 32>& hash) {
    std::ostringstream oss;
    for (uint8_t byte : hash) {
        oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

std::string LocalDiskDataLayer::GetPayloadPath(const std::array<uint8_t, 32>& hash) const {
    std::string hex = HashToHex(hash);
    // Use first 2 bytes (4 hex chars) as directory prefix for sharding
    std::string subdir = m_base_path + "/" + hex.substr(0, 4);
    return subdir + "/" + hex + ".payload";
}

bool LocalDiskDataLayer::EnsureDirectory(const std::string& path) const {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return mkdir(path.c_str(), 0755) == 0;
}

bool LocalDiskDataLayer::PutPayload(const PayloadObject& payload) {
    std::string path = GetPayloadPath(payload.payload_hash);

    // Ensure subdirectory exists
    std::string hex = HashToHex(payload.payload_hash);
    std::string subdir = m_base_path + "/" + hex.substr(0, 4);
    if (!EnsureDirectory(subdir)) {
        return false;
    }

    // Write payload to file
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    // Simple binary format:
    // [32 bytes: payload_hash]
    // [32 bytes: anchor_id]
    // [8 bytes: created_at]
    // [8 bytes: expires_at]
    // [4 bytes: sender_address length]
    // [N bytes: sender_address]
    // [4 bytes: encrypted_data length]
    // [N bytes: encrypted_data]

    file.write(reinterpret_cast<const char*>(payload.payload_hash.data()), 32);
    file.write(reinterpret_cast<const char*>(payload.anchor_id.data()), 32);
    file.write(reinterpret_cast<const char*>(&payload.created_at), 8);
    file.write(reinterpret_cast<const char*>(&payload.expires_at), 8);

    uint32_t addr_len = payload.sender_address.size();
    file.write(reinterpret_cast<const char*>(&addr_len), 4);
    file.write(payload.sender_address.data(), addr_len);

    uint32_t data_len = payload.encrypted_data.size();
    file.write(reinterpret_cast<const char*>(&data_len), 4);
    file.write(reinterpret_cast<const char*>(payload.encrypted_data.data()), data_len);

    return file.good();
}

bool LocalDiskDataLayer::GetPayload(const std::array<uint8_t, 32>& payload_hash, PayloadObject& out) {
    std::string path = GetPayloadPath(payload_hash);

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    file.read(reinterpret_cast<char*>(out.payload_hash.data()), 32);
    file.read(reinterpret_cast<char*>(out.anchor_id.data()), 32);
    file.read(reinterpret_cast<char*>(&out.created_at), 8);
    file.read(reinterpret_cast<char*>(&out.expires_at), 8);

    uint32_t addr_len;
    file.read(reinterpret_cast<char*>(&addr_len), 4);
    out.sender_address.resize(addr_len);
    file.read(&out.sender_address[0], addr_len);

    uint32_t data_len;
    file.read(reinterpret_cast<char*>(&data_len), 4);
    out.encrypted_data.resize(data_len);
    file.read(reinterpret_cast<char*>(out.encrypted_data.data()), data_len);

    return file.good();
}

bool LocalDiskDataLayer::HasPayload(const std::array<uint8_t, 32>& payload_hash) {
    std::string path = GetPayloadPath(payload_hash);
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

bool LocalDiskDataLayer::DeletePayload(const std::array<uint8_t, 32>& payload_hash) {
    std::string path = GetPayloadPath(payload_hash);
    return unlink(path.c_str()) == 0 || errno == ENOENT;
}

uint32_t LocalDiskDataLayer::PruneExpired(int64_t current_time) {
    uint32_t pruned = 0;

    // Iterate through all subdirectories
    DIR* dir = opendir(m_base_path.c_str());
    if (!dir) return 0;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_DIR) continue;
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        std::string subdir = m_base_path + "/" + entry->d_name;
        DIR* subdir_ptr = opendir(subdir.c_str());
        if (!subdir_ptr) continue;

        struct dirent* payload_entry;
        while ((payload_entry = readdir(subdir_ptr)) != nullptr) {
            if (payload_entry->d_type != DT_REG) continue;

            std::string filename = payload_entry->d_name;
            if (filename.size() < 8 || filename.substr(filename.size() - 8) != ".payload") continue;

            std::string filepath = subdir + "/" + filename;

            // Read just the expiry time
            std::ifstream file(filepath, std::ios::binary);
            if (!file) continue;

            file.seekg(32 + 32 + 8);  // Skip hash, anchor_id, created_at
            int64_t expires_at;
            file.read(reinterpret_cast<char*>(&expires_at), 8);
            file.close();

            if (expires_at > 0 && current_time >= expires_at) {
                if (unlink(filepath.c_str()) == 0) {
                    pruned++;
                }
            }
        }
        closedir(subdir_ptr);
    }
    closedir(dir);

    return pruned;
}

void LocalDiskDataLayer::GetStats(uint64_t& total_payloads, uint64_t& total_bytes, uint64_t& expired_count) {
    total_payloads = 0;
    total_bytes = 0;
    expired_count = 0;

    int64_t current_time = time(nullptr);

    DIR* dir = opendir(m_base_path.c_str());
    if (!dir) return;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_DIR) continue;
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        std::string subdir = m_base_path + "/" + entry->d_name;
        DIR* subdir_ptr = opendir(subdir.c_str());
        if (!subdir_ptr) continue;

        struct dirent* payload_entry;
        while ((payload_entry = readdir(subdir_ptr)) != nullptr) {
            if (payload_entry->d_type != DT_REG) continue;

            std::string filename = payload_entry->d_name;
            if (filename.size() < 8 || filename.substr(filename.size() - 8) != ".payload") continue;

            std::string filepath = subdir + "/" + filename;

            struct stat st;
            if (stat(filepath.c_str(), &st) == 0) {
                total_payloads++;
                total_bytes += st.st_size;

                // Check if expired
                std::ifstream file(filepath, std::ios::binary);
                if (file) {
                    file.seekg(32 + 32 + 8);
                    int64_t expires_at;
                    file.read(reinterpret_cast<char*>(&expires_at), 8);
                    if (expires_at > 0 && current_time >= expires_at) {
                        expired_count++;
                    }
                }
            }
        }
        closedir(subdir_ptr);
    }
    closedir(dir);
}

//-----------------------------------------------------------------------------
// DataLayerFactory implementation
//-----------------------------------------------------------------------------

std::unique_ptr<IDataLayer> DataLayerFactory::Create(const std::string& type, const std::string& config) {
    if (type == "null" || type == "none") {
        return std::make_unique<NullDataLayer>();
    }
    if (type == "local" || type == "disk" || type == "local_disk") {
        return std::make_unique<LocalDiskDataLayer>(config);
    }
    // Future: IPFS, P2P, etc.
    return nullptr;
}

//-----------------------------------------------------------------------------
// Global initialization
//-----------------------------------------------------------------------------

bool InitializeDataLayer(PayloadMode mode, const std::string& data_dir) {
    switch (mode) {
        case PayloadMode::LEGACY:
            // No separate data layer needed - payloads stored on-chain
            g_data_layer = std::make_unique<NullDataLayer>();
            return true;

        case PayloadMode::ANCHOR:
        case PayloadMode::HYBRID:
            {
                std::string payload_dir = data_dir + "/dions2_payloads";
                g_data_layer = std::make_unique<LocalDiskDataLayer>(payload_dir);
                return g_data_layer != nullptr;
            }
    }
    return false;
}

void ShutdownDataLayer() {
    g_data_layer.reset();
}

} // namespace dions2
