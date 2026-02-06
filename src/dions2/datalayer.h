// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 - Data Layer Interface
// Abstraction for off-chain payload storage

#ifndef DIONS2_DATALAYER_H
#define DIONS2_DATALAYER_H

#include "anchor.h"
#include <memory>
#include <functional>

namespace dions2 {

/**
 * IDataLayer - Abstract interface for payload storage
 *
 * Implementations:
 * - NullDataLayer: No storage (anchor-only mode)
 * - LocalDiskDataLayer: Local filesystem storage (Phase 0)
 * - IPFSDataLayer: IPFS-based distributed storage (Phase 1)
 * - P2PDataLayer: Custom p2p network storage (Phase 2)
 */
class IDataLayer {
public:
    virtual ~IDataLayer() = default;

    /**
     * Store a payload
     * @param payload The payload to store
     * @return true if stored successfully
     */
    virtual bool PutPayload(const PayloadObject& payload) = 0;

    /**
     * Retrieve a payload by hash
     * @param payload_hash SHA256 hash of the payload
     * @param out Output: the retrieved payload
     * @return true if found
     */
    virtual bool GetPayload(const std::array<uint8_t, 32>& payload_hash, PayloadObject& out) = 0;

    /**
     * Check if a payload exists
     * @param payload_hash SHA256 hash of the payload
     * @return true if payload exists in storage
     */
    virtual bool HasPayload(const std::array<uint8_t, 32>& payload_hash) = 0;

    /**
     * Delete a payload
     * @param payload_hash SHA256 hash of the payload
     * @return true if deleted (or already absent)
     */
    virtual bool DeletePayload(const std::array<uint8_t, 32>& payload_hash) = 0;

    /**
     * Prune expired payloads
     * @param current_time Current unix timestamp
     * @return Number of payloads pruned
     */
    virtual uint32_t PruneExpired(int64_t current_time) = 0;

    /**
     * Get storage statistics
     * @param total_payloads Output: total payload count
     * @param total_bytes Output: total bytes stored
     * @param expired_count Output: number of expired payloads pending deletion
     */
    virtual void GetStats(uint64_t& total_payloads, uint64_t& total_bytes, uint64_t& expired_count) = 0;

    /**
     * Get the data layer type name
     */
    virtual std::string GetTypeName() const = 0;
};

/**
 * NullDataLayer - No-op implementation for anchor-only mode
 * Useful for validators that don't want to store payloads
 */
class NullDataLayer : public IDataLayer {
public:
    bool PutPayload(const PayloadObject& payload) override { return true; }
    bool GetPayload(const std::array<uint8_t, 32>& payload_hash, PayloadObject& out) override { return false; }
    bool HasPayload(const std::array<uint8_t, 32>& payload_hash) override { return false; }
    bool DeletePayload(const std::array<uint8_t, 32>& payload_hash) override { return true; }
    uint32_t PruneExpired(int64_t current_time) override { return 0; }
    void GetStats(uint64_t& total_payloads, uint64_t& total_bytes, uint64_t& expired_count) override {
        total_payloads = 0;
        total_bytes = 0;
        expired_count = 0;
    }
    std::string GetTypeName() const override { return "null"; }
};

/**
 * LocalDiskDataLayer - Filesystem-based storage
 * Stores payloads in a local directory, organized by hash prefix
 */
class LocalDiskDataLayer : public IDataLayer {
public:
    explicit LocalDiskDataLayer(const std::string& base_path);
    ~LocalDiskDataLayer() override;

    bool PutPayload(const PayloadObject& payload) override;
    bool GetPayload(const std::array<uint8_t, 32>& payload_hash, PayloadObject& out) override;
    bool HasPayload(const std::array<uint8_t, 32>& payload_hash) override;
    bool DeletePayload(const std::array<uint8_t, 32>& payload_hash) override;
    uint32_t PruneExpired(int64_t current_time) override;
    void GetStats(uint64_t& total_payloads, uint64_t& total_bytes, uint64_t& expired_count) override;
    std::string GetTypeName() const override { return "local_disk"; }

private:
    std::string m_base_path;

    // Get file path for a payload hash
    std::string GetPayloadPath(const std::array<uint8_t, 32>& hash) const;

    // Ensure directory exists
    bool EnsureDirectory(const std::string& path) const;

    // Hash to hex string
    static std::string HashToHex(const std::array<uint8_t, 32>& hash);
};

/**
 * DataLayerFactory - Create data layer instances
 */
class DataLayerFactory {
public:
    /**
     * Create a data layer based on configuration
     * @param type Type name: "null", "local", "ipfs", etc.
     * @param config Configuration string (path for local, endpoint for IPFS, etc.)
     * @return Unique pointer to created data layer
     */
    static std::unique_ptr<IDataLayer> Create(const std::string& type, const std::string& config);
};

/**
 * Global data layer instance
 * Set during initialization based on -dions_payload_mode flag
 */
extern std::unique_ptr<IDataLayer> g_data_layer;

/**
 * Initialize the global data layer
 * @param mode Payload mode from command line
 * @param data_dir Base data directory
 * @return true if initialized successfully
 */
bool InitializeDataLayer(PayloadMode mode, const std::string& data_dir);

/**
 * Shutdown the global data layer
 */
void ShutdownDataLayer();

} // namespace dions2

#endif // DIONS2_DATALAYER_H
