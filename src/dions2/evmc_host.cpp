// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 EVMC Host Interface Implementation
//
// Bridges EVMExecutor state management to evmone for bytecode execution.

#include "evmc_host.h"
#include "util.h"  // For IOC's Hash() function
#include <cstring>
#include <algorithm>

namespace dions2 {

//=============================================================================
// EVMCHostContext Implementation
//=============================================================================

EVMCHostContext::EVMCHostContext(EVMExecutor* exec) : executor(exec) {
    // Initialize transaction context with defaults
    std::memset(&tx_context, 0, sizeof(tx_context));

    // Set chain ID (IOC mainnet = 1, testnet = 2)
    tx_context.chain_id.bytes[31] = 1;

    // Default gas price
    tx_context.tx_gas_price.bytes[31] = 1;

    // Default block gas limit (30M like Ethereum)
    tx_context.block_gas_limit = 30000000;
}

//=============================================================================
// Helper Functions
//=============================================================================

std::vector<uint8_t> EVMCHost::AddressToVector(const evmc_address* addr) {
    return std::vector<uint8_t>(addr->bytes, addr->bytes + 20);
}

std::vector<uint8_t> EVMCHost::Bytes32ToVector(const evmc_bytes32* bytes) {
    return std::vector<uint8_t>(bytes->bytes, bytes->bytes + 32);
}

evmc_address EVMCHost::VectorToAddress(const std::vector<uint8_t>& vec) {
    evmc_address addr;
    std::memset(&addr, 0, sizeof(addr));
    size_t copy_size = std::min(vec.size(), size_t(20));
    // Right-align for addresses shorter than 20 bytes
    std::memcpy(addr.bytes + (20 - copy_size), vec.data(), copy_size);
    return addr;
}

evmc_bytes32 EVMCHost::VectorToBytes32(const std::vector<uint8_t>& vec) {
    evmc_bytes32 bytes;
    std::memset(&bytes, 0, sizeof(bytes));
    size_t copy_size = std::min(vec.size(), size_t(32));
    // Right-align for values shorter than 32 bytes
    std::memcpy(bytes.bytes + (32 - copy_size), vec.data(), copy_size);
    return bytes;
}

EVMCHostContext* EVMCHost::GetContext(evmc_host_context* ctx) {
    return reinterpret_cast<EVMCHostContext*>(ctx);
}

//=============================================================================
// Host Interface Callbacks
//=============================================================================

bool EVMCHost::account_exists(evmc_host_context* context, const evmc_address* address) {
    auto* ctx = GetContext(context);
    auto addr_vec = AddressToVector(address);
    return ctx->executor->AccountExists(addr_vec);
}

evmc_bytes32 EVMCHost::get_storage(evmc_host_context* context, const evmc_address* address, const evmc_bytes32* key) {
    auto* ctx = GetContext(context);
    auto addr_vec = AddressToVector(address);
    auto key_vec = Bytes32ToVector(key);

    auto value = ctx->executor->GetStorage(addr_vec, key_vec);
    return VectorToBytes32(value);
}

evmc_storage_status EVMCHost::set_storage(evmc_host_context* context, const evmc_address* address, const evmc_bytes32* key, const evmc_bytes32* value) {
    auto* ctx = GetContext(context);
    auto addr_vec = AddressToVector(address);
    auto key_vec = Bytes32ToVector(key);
    auto value_vec = Bytes32ToVector(value);

    // Get current value to determine storage status
    auto current = ctx->executor->GetStorage(addr_vec, key_vec);

    // Check if value is zero (all bytes zero)
    auto is_zero = [](const std::vector<uint8_t>& v) {
        return std::all_of(v.begin(), v.end(), [](uint8_t b) { return b == 0; });
    };

    bool current_is_zero = current.empty() || is_zero(current);
    bool new_is_zero = is_zero(value_vec);

    // Set the new value
    ctx->executor->SetStorage(addr_vec, key_vec, value_vec);

    // Determine storage status for gas calculation
    if (current_is_zero && !new_is_zero) {
        return EVMC_STORAGE_ADDED;
    } else if (!current_is_zero && new_is_zero) {
        return EVMC_STORAGE_DELETED;
    } else if (!current_is_zero && !new_is_zero && current != value_vec) {
        return EVMC_STORAGE_MODIFIED;
    }
    return EVMC_STORAGE_ASSIGNED;
}

evmc_uint256be EVMCHost::get_balance(evmc_host_context* context, const evmc_address* address) {
    auto* ctx = GetContext(context);
    auto addr_vec = AddressToVector(address);
    auto balance = ctx->executor->GetBalance(addr_vec);
    return VectorToBytes32(balance);
}

size_t EVMCHost::get_code_size(evmc_host_context* context, const evmc_address* address) {
    auto* ctx = GetContext(context);
    auto addr_vec = AddressToVector(address);
    auto code = ctx->executor->GetCode(addr_vec);
    return code.size();
}

evmc_bytes32 EVMCHost::get_code_hash(evmc_host_context* context, const evmc_address* address) {
    auto* ctx = GetContext(context);
    auto addr_vec = AddressToVector(address);

    // Check if account exists
    if (!ctx->executor->AccountExists(addr_vec)) {
        evmc_bytes32 zero;
        std::memset(&zero, 0, sizeof(zero));
        return zero;
    }

    auto code_hash = ctx->executor->GetCodeHash(addr_vec);
    return VectorToBytes32(code_hash);
}

size_t EVMCHost::copy_code(evmc_host_context* context, const evmc_address* address, size_t code_offset, uint8_t* buffer_data, size_t buffer_size) {
    auto* ctx = GetContext(context);
    auto addr_vec = AddressToVector(address);
    auto code = ctx->executor->GetCode(addr_vec);

    if (code_offset >= code.size()) {
        return 0;
    }

    size_t copy_size = std::min(buffer_size, code.size() - code_offset);
    std::memcpy(buffer_data, code.data() + code_offset, copy_size);
    return copy_size;
}

bool EVMCHost::selfdestruct(evmc_host_context* context, const evmc_address* address, const evmc_address* beneficiary) {
    auto* ctx = GetContext(context);
    auto addr_vec = AddressToVector(address);
    auto beneficiary_vec = AddressToVector(beneficiary);

    // Check if already selfdestructed
    bool first_time = ctx->selfdestructed.find(addr_vec) == ctx->selfdestructed.end();

    if (first_time) {
        ctx->selfdestructed.insert(addr_vec);

        // Transfer balance to beneficiary
        auto balance = ctx->executor->GetBalance(addr_vec);
        if (!balance.empty()) {
            ctx->executor->Transfer(addr_vec, beneficiary_vec, balance);
        }

        // Mark for deletion (actual deletion happens at end of transaction)
        ctx->executor->DeleteAccount(addr_vec);
    }

    return first_time;
}

evmc_result EVMCHost::call(evmc_host_context* context, const evmc_message* msg) {
    auto* ctx = GetContext(context);

    // For now, return failure - full nested call support requires more infrastructure
    evmc_result result;
    std::memset(&result, 0, sizeof(result));
    result.status_code = EVMC_REVERT;
    result.gas_left = 0;
    return result;
}

evmc_tx_context EVMCHost::get_tx_context(evmc_host_context* context) {
    auto* ctx = GetContext(context);
    return ctx->tx_context;
}

evmc_bytes32 EVMCHost::get_block_hash(evmc_host_context* context, int64_t number) {
    // TODO: Hook into IOC blockchain for actual block hashes
    // For now, return a deterministic hash based on block number
    evmc_bytes32 hash;
    std::memset(&hash, 0, sizeof(hash));

    // Simple deterministic hash: keccak256(block_number)
    std::vector<uint8_t> data(8);
    for (int i = 0; i < 8; i++) {
        data[i] = (number >> (56 - i * 8)) & 0xFF;
    }

    uint256 h = Hash(data.begin(), data.end());
    std::memcpy(hash.bytes, h.begin(), 32);
    return hash;
}

void EVMCHost::emit_log(evmc_host_context* context, const evmc_address* address, const uint8_t* data, size_t data_size, const evmc_bytes32 topics[], size_t topics_count) {
    auto* ctx = GetContext(context);

    EVMCHostContext::LogEntry log;
    log.address = AddressToVector(address);
    log.data = std::vector<uint8_t>(data, data + data_size);

    for (size_t i = 0; i < topics_count; i++) {
        log.topics.push_back(Bytes32ToVector(&topics[i]));
    }

    ctx->logs.push_back(std::move(log));
}

evmc_access_status EVMCHost::access_account(evmc_host_context* context, const evmc_address* address) {
    auto* ctx = GetContext(context);
    auto addr_vec = AddressToVector(address);

    if (ctx->warm_accounts.find(addr_vec) != ctx->warm_accounts.end()) {
        return EVMC_ACCESS_WARM;
    }

    ctx->warm_accounts.insert(addr_vec);
    return EVMC_ACCESS_COLD;
}

evmc_access_status EVMCHost::access_storage(evmc_host_context* context, const evmc_address* address, const evmc_bytes32* key) {
    auto* ctx = GetContext(context);
    auto addr_vec = AddressToVector(address);
    auto key_vec = Bytes32ToVector(key);

    auto& storage_keys = ctx->warm_storage[addr_vec];
    if (storage_keys.find(key_vec) != storage_keys.end()) {
        return EVMC_ACCESS_WARM;
    }

    storage_keys.insert(key_vec);
    return EVMC_ACCESS_COLD;
}

evmc_bytes32 EVMCHost::get_transient_storage(evmc_host_context* context, const evmc_address* address, const evmc_bytes32* key) {
    auto* ctx = GetContext(context);
    auto addr_vec = AddressToVector(address);
    auto key_vec = Bytes32ToVector(key);

    auto it = ctx->transient_storage.find(addr_vec);
    if (it != ctx->transient_storage.end()) {
        auto key_it = it->second.find(key_vec);
        if (key_it != it->second.end()) {
            return VectorToBytes32(key_it->second);
        }
    }

    evmc_bytes32 zero;
    std::memset(&zero, 0, sizeof(zero));
    return zero;
}

void EVMCHost::set_transient_storage(evmc_host_context* context, const evmc_address* address, const evmc_bytes32* key, const evmc_bytes32* value) {
    auto* ctx = GetContext(context);
    auto addr_vec = AddressToVector(address);
    auto key_vec = Bytes32ToVector(key);
    auto value_vec = Bytes32ToVector(value);

    ctx->transient_storage[addr_vec][key_vec] = value_vec;
}

//=============================================================================
// Host Interface Singleton
//=============================================================================

const evmc_host_interface* EVMCHost::GetInterface() {
    static const evmc_host_interface host_interface = {
        account_exists,
        get_storage,
        set_storage,
        get_balance,
        get_code_size,
        get_code_hash,
        copy_code,
        selfdestruct,
        call,
        get_tx_context,
        get_block_hash,
        emit_log,
        access_account,
        access_storage,
        get_transient_storage,
        set_transient_storage
    };
    return &host_interface;
}

//=============================================================================
// VM Management
//=============================================================================

#ifdef HAVE_EVMONE

evmc_vm* EVMCHost::CreateVM() {
    return evmc_create_evmone();
}

void EVMCHost::DestroyVM(evmc_vm* vm) {
    if (vm && vm->destroy) {
        vm->destroy(vm);
    }
}

EVMResult EVMCHost::Execute(
    evmc_vm* vm,
    EVMCHostContext* context,
    evmc_revision rev,
    const evmc_message* msg,
    const uint8_t* code,
    size_t code_size
) {
    EVMResult result;

    if (!vm || !context || !msg) {
        result.status = EVMResult::INTERNAL_ERROR;
        result.error_message = "Invalid parameters";
        return result;
    }

    // Execute via evmone
    evmc_result evmc_res = vm->execute(
        vm,
        GetInterface(),
        reinterpret_cast<evmc_host_context*>(context),
        rev,
        msg,
        code,
        code_size
    );

    // Convert result
    result.gas_used = msg->gas - evmc_res.gas_left;

    switch (evmc_res.status_code) {
        case EVMC_SUCCESS:
            result.status = EVMResult::SUCCESS;
            break;
        case EVMC_REVERT:
            result.status = EVMResult::REVERT;
            break;
        case EVMC_OUT_OF_GAS:
            result.status = EVMResult::OUT_OF_GAS;
            break;
        case EVMC_INVALID_INSTRUCTION:
            result.status = EVMResult::INVALID_INSTRUCTION;
            break;
        case EVMC_UNDEFINED_INSTRUCTION:
            result.status = EVMResult::UNDEFINED_INSTRUCTION;
            break;
        case EVMC_STACK_OVERFLOW:
            result.status = EVMResult::STACK_OVERFLOW;
            break;
        case EVMC_STACK_UNDERFLOW:
            result.status = EVMResult::STACK_UNDERFLOW;
            break;
        case EVMC_BAD_JUMP_DESTINATION:
            result.status = EVMResult::BAD_JUMP_DESTINATION;
            break;
        case EVMC_INVALID_MEMORY_ACCESS:
            result.status = EVMResult::INVALID_MEMORY_ACCESS;
            break;
        case EVMC_CALL_DEPTH_EXCEEDED:
            result.status = EVMResult::CALL_DEPTH_EXCEEDED;
            break;
        case EVMC_STATIC_MODE_VIOLATION:
            result.status = EVMResult::STATIC_MODE_VIOLATION;
            break;
        case EVMC_PRECOMPILE_FAILURE:
            result.status = EVMResult::PRECOMPILE_FAILURE;
            break;
        case EVMC_CONTRACT_VALIDATION_FAILURE:
            result.status = EVMResult::CONTRACT_VALIDATION_FAILURE;
            break;
        case EVMC_ARGUMENT_OUT_OF_RANGE:
            result.status = EVMResult::ARGUMENT_OUT_OF_RANGE;
            break;
        case EVMC_INSUFFICIENT_BALANCE:
            result.status = EVMResult::INSUFFICIENT_BALANCE;
            break;
        default:
            result.status = EVMResult::INTERNAL_ERROR;
            break;
    }

    // Copy output
    if (evmc_res.output_data && evmc_res.output_size > 0) {
        result.output.assign(evmc_res.output_data, evmc_res.output_data + evmc_res.output_size);
    }

    // Release evmone result resources
    if (evmc_res.release) {
        evmc_res.release(&evmc_res);
    }

    return result;
}

bool IsEvmoneAvailable() {
    return true;
}

#else // !HAVE_EVMONE

evmc_vm* EVMCHost::CreateVM() {
    return nullptr;
}

void EVMCHost::DestroyVM(evmc_vm* vm) {
    (void)vm;
}

EVMResult EVMCHost::Execute(
    evmc_vm* vm,
    EVMCHostContext* context,
    evmc_revision rev,
    const evmc_message* msg,
    const uint8_t* code,
    size_t code_size
) {
    (void)vm; (void)context; (void)rev; (void)msg; (void)code; (void)code_size;

    EVMResult result;
    result.status = EVMResult::INTERNAL_ERROR;
    result.error_message = "evmone not available - compile with HAVE_EVMONE";
    return result;
}

bool IsEvmoneAvailable() {
    return false;
}

#endif // HAVE_EVMONE

} // namespace dions2
