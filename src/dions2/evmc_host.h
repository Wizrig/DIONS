// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 EVMC Host Interface - Bridges EVMExecutor to evmone
//
// This implements the EVMC host interface allowing evmone to interact
// with the IOC blockchain state through EVMExecutor.

#ifndef DIONS2_EVMC_HOST_H
#define DIONS2_EVMC_HOST_H

#include "evm.h"
#include <evmc/evmc.h>
#include <map>
#include <set>
#include <vector>

namespace dions2 {

/**
 * EVMC Host Context - Holds execution state for a single EVM call
 */
struct EVMCHostContext {
    EVMExecutor* executor;                // Reference to our EVM state
    evmc_tx_context tx_context;           // Transaction context

    // Access tracking (EIP-2929)
    std::set<std::vector<uint8_t>> warm_accounts;
    std::map<std::vector<uint8_t>, std::set<std::vector<uint8_t>>> warm_storage;

    // Transient storage (EIP-1153)
    std::map<std::vector<uint8_t>, std::map<std::vector<uint8_t>, std::vector<uint8_t>>> transient_storage;

    // Selfdestructed accounts
    std::set<std::vector<uint8_t>> selfdestructed;

    // Log entries
    struct LogEntry {
        std::vector<uint8_t> address;
        std::vector<uint8_t> data;
        std::vector<std::vector<uint8_t>> topics;
    };
    std::vector<LogEntry> logs;

    EVMCHostContext(EVMExecutor* exec);
};

/**
 * EVMC Host Implementation
 *
 * Provides the evmc_host_interface implementation that bridges
 * evmone execution to our EVMExecutor state management.
 */
class EVMCHost {
public:
    // Get the singleton host interface
    static const evmc_host_interface* GetInterface();

    // Create evmone VM instance
    static evmc_vm* CreateVM();

    // Destroy evmone VM instance
    static void DestroyVM(evmc_vm* vm);

    // Execute EVM bytecode using evmone
    static EVMResult Execute(
        evmc_vm* vm,
        EVMCHostContext* context,
        evmc_revision rev,
        const evmc_message* msg,
        const uint8_t* code,
        size_t code_size
    );

private:
    // Host interface callbacks
    static bool account_exists(evmc_host_context* context, const evmc_address* address);
    static evmc_bytes32 get_storage(evmc_host_context* context, const evmc_address* address, const evmc_bytes32* key);
    static evmc_storage_status set_storage(evmc_host_context* context, const evmc_address* address, const evmc_bytes32* key, const evmc_bytes32* value);
    static evmc_uint256be get_balance(evmc_host_context* context, const evmc_address* address);
    static size_t get_code_size(evmc_host_context* context, const evmc_address* address);
    static evmc_bytes32 get_code_hash(evmc_host_context* context, const evmc_address* address);
    static size_t copy_code(evmc_host_context* context, const evmc_address* address, size_t code_offset, uint8_t* buffer_data, size_t buffer_size);
    static bool selfdestruct(evmc_host_context* context, const evmc_address* address, const evmc_address* beneficiary);
    static evmc_result call(evmc_host_context* context, const evmc_message* msg);
    static evmc_tx_context get_tx_context(evmc_host_context* context);
    static evmc_bytes32 get_block_hash(evmc_host_context* context, int64_t number);
    static void emit_log(evmc_host_context* context, const evmc_address* address, const uint8_t* data, size_t data_size, const evmc_bytes32 topics[], size_t topics_count);
    static evmc_access_status access_account(evmc_host_context* context, const evmc_address* address);
    static evmc_access_status access_storage(evmc_host_context* context, const evmc_address* address, const evmc_bytes32* key);
    static evmc_bytes32 get_transient_storage(evmc_host_context* context, const evmc_address* address, const evmc_bytes32* key);
    static void set_transient_storage(evmc_host_context* context, const evmc_address* address, const evmc_bytes32* key, const evmc_bytes32* value);

    // Helper functions
    static std::vector<uint8_t> AddressToVector(const evmc_address* addr);
    static std::vector<uint8_t> Bytes32ToVector(const evmc_bytes32* bytes);
    static evmc_address VectorToAddress(const std::vector<uint8_t>& vec);
    static evmc_bytes32 VectorToBytes32(const std::vector<uint8_t>& vec);
    static EVMCHostContext* GetContext(evmc_host_context* ctx);
};

// External declaration of evmone creation function
extern "C" {
    struct evmc_vm* evmc_create_evmone(void);
}

// Check if evmone is available (linked)
bool IsEvmoneAvailable();

} // namespace dions2

#endif // DIONS2_EVMC_HOST_H
