// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 EVM Zone - Ethereum Virtual Machine executor for smart contracts
// Phase 0: State management infrastructure (evmone integration pending)

#ifndef DIONS2_EVM_H
#define DIONS2_EVM_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace dions2 {

/**
 * EVM Account - Represents an account in the EVM state
 */
struct EVMAccount {
    uint64_t nonce;
    std::vector<uint8_t> balance;  // 256-bit integer as bytes (big-endian)
    std::vector<uint8_t> code;     // Contract bytecode
    std::map<std::vector<uint8_t>, std::vector<uint8_t>> storage;  // Key-value storage

    EVMAccount() : nonce(0) {}
};

/**
 * EVM Transaction - Represents a transaction to execute
 */
struct EVMTransaction {
    std::vector<uint8_t> from;      // 20-byte sender address
    std::vector<uint8_t> to;        // 20-byte recipient (empty for contract creation)
    std::vector<uint8_t> value;     // 256-bit transfer amount
    std::vector<uint8_t> data;      // Transaction data or contract bytecode
    uint64_t gas_limit;
    std::vector<uint8_t> gas_price; // 256-bit gas price
    uint64_t nonce;

    EVMTransaction() : gas_limit(0), nonce(0) {}
};

/**
 * EVM Execution Result
 */
struct EVMResult {
    enum Status {
        SUCCESS = 0,
        REVERT = 1,
        OUT_OF_GAS = 2,
        INVALID_INSTRUCTION = 3,
        UNDEFINED_INSTRUCTION = 4,
        STACK_OVERFLOW = 5,
        STACK_UNDERFLOW = 6,
        BAD_JUMP_DESTINATION = 7,
        INVALID_MEMORY_ACCESS = 8,
        CALL_DEPTH_EXCEEDED = 9,
        STATIC_MODE_VIOLATION = 10,
        PRECOMPILE_FAILURE = 11,
        CONTRACT_VALIDATION_FAILURE = 12,
        ARGUMENT_OUT_OF_RANGE = 13,
        INSUFFICIENT_BALANCE = 16,
        INTERNAL_ERROR = -1
    };

    Status status;
    uint64_t gas_used;
    std::vector<uint8_t> output;
    std::string error_message;

    EVMResult() : status(INTERNAL_ERROR), gas_used(0) {}

    bool IsSuccess() const { return status == SUCCESS; }
};

/**
 * EVM Executor - Main execution engine for EVM contracts
 *
 * Note: This is the state management layer. Actual bytecode execution
 * requires evmone integration (TODO: integrate evmone library).
 */
class EVMExecutor {
private:
    std::map<std::vector<uint8_t>, EVMAccount> accounts_;
    void* evm_instance_;  // Opaque pointer to evmone instance (future)

    // Checkpoint for state rollback
    struct Checkpoint {
        std::map<std::vector<uint8_t>, EVMAccount> accounts_snapshot;
    };
    std::vector<Checkpoint> checkpoints_;

public:
    EVMExecutor();
    ~EVMExecutor();

    // Account management
    bool CreateAccount(const std::vector<uint8_t>& address,
                      const std::vector<uint8_t>& balance = {});
    bool AccountExists(const std::vector<uint8_t>& address) const;
    EVMAccount* GetAccount(const std::vector<uint8_t>& address);
    const EVMAccount* GetAccount(const std::vector<uint8_t>& address) const;
    bool DeleteAccount(const std::vector<uint8_t>& address);

    // Balance operations
    bool SetBalance(const std::vector<uint8_t>& address, const std::vector<uint8_t>& balance);
    std::vector<uint8_t> GetBalance(const std::vector<uint8_t>& address) const;
    bool Transfer(const std::vector<uint8_t>& from, const std::vector<uint8_t>& to,
                 const std::vector<uint8_t>& amount);

    // Code operations
    bool SetCode(const std::vector<uint8_t>& address, const std::vector<uint8_t>& code);
    std::vector<uint8_t> GetCode(const std::vector<uint8_t>& address) const;
    std::vector<uint8_t> GetCodeHash(const std::vector<uint8_t>& address) const;

    // Storage operations
    bool SetStorage(const std::vector<uint8_t>& address,
                   const std::vector<uint8_t>& key,
                   const std::vector<uint8_t>& value);
    std::vector<uint8_t> GetStorage(const std::vector<uint8_t>& address,
                                   const std::vector<uint8_t>& key) const;

    // Transaction execution (stub - requires evmone)
    EVMResult ExecuteTransaction(const EVMTransaction& tx);
    EVMResult Call(const std::vector<uint8_t>& from,
                  const std::vector<uint8_t>& to,
                  const std::vector<uint8_t>& data,
                  uint64_t gas_limit,
                  const std::vector<uint8_t>& value = {});

    // Contract deployment
    EVMResult DeployContract(const std::vector<uint8_t>& from,
                            const std::vector<uint8_t>& bytecode,
                            uint64_t gas_limit,
                            const std::vector<uint8_t>& value = {},
                            std::vector<uint8_t>* contract_address = nullptr);

    // Contract address generation
    std::vector<uint8_t> GenerateContractAddress(const std::vector<uint8_t>& deployer,
                                                 uint64_t nonce) const;
    std::vector<uint8_t> GenerateCreate2Address(const std::vector<uint8_t>& deployer,
                                                const std::vector<uint8_t>& salt,
                                                const std::vector<uint8_t>& init_code_hash) const;

    // Validation
    bool ValidateAddress(const std::vector<uint8_t>& address) const;
    bool ValidateTransaction(const EVMTransaction& tx) const;

    // State management
    void CreateCheckpoint();
    void RevertToCheckpoint();
    void CommitCheckpoint();
    void ClearState();

    // Statistics
    size_t GetAccountCount() const { return accounts_.size(); }

private:
    bool InitializeEVM();
    void CleanupEVM();

    // Big integer arithmetic helpers
    std::vector<uint8_t> AddBigInts(const std::vector<uint8_t>& a,
                                    const std::vector<uint8_t>& b) const;
    std::vector<uint8_t> SubBigInts(const std::vector<uint8_t>& a,
                                    const std::vector<uint8_t>& b) const;
    int CompareBigInts(const std::vector<uint8_t>& a,
                      const std::vector<uint8_t>& b) const;
};

// Utility functions
std::vector<uint8_t> HexToBytes(const std::string& hex);
std::string BytesToHex(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> StringToEVMAddress(const std::string& address_str);
std::string EVMAddressToString(const std::vector<uint8_t>& address);

// Convert between IOC addresses and EVM addresses
std::vector<uint8_t> IOCAddressToEVM(const std::string& ioc_address);
std::string EVMToIOCAddress(const std::vector<uint8_t>& evm_address);

} // namespace dions2

#endif // DIONS2_EVM_H
