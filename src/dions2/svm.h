// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 SVM Zone - Solana Virtual Machine executor for smart contracts
// Phase 0: State management infrastructure (BPF runtime pending)

#ifndef DIONS2_SVM_H
#define DIONS2_SVM_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <array>

namespace dions2 {

// Solana public key (32 bytes, like on actual Solana)
using SolanaPublicKey = std::array<uint8_t, 32>;

/**
 * Solana Account - Represents an account in the SVM state
 */
struct SolanaAccount {
    uint64_t lamports;             // Balance in lamports (1 SOL = 10^9 lamports)
    std::vector<uint8_t> data;     // Account data
    SolanaPublicKey owner;         // Program that owns this account
    bool executable;               // Whether this contains executable code
    uint64_t rent_epoch;           // Epoch at which rent was last paid

    SolanaAccount() : lamports(0), executable(false), rent_epoch(0) {
        owner.fill(0);
    }

    SolanaAccount(uint64_t lamps, const SolanaPublicKey& own, bool exec = false)
        : lamports(lamps), owner(own), executable(exec), rent_epoch(0) {}
};

/**
 * Solana Instruction - A single instruction to execute
 */
struct SolanaInstruction {
    SolanaPublicKey program_id;              // Program to execute
    std::vector<SolanaPublicKey> accounts;   // Accounts involved
    std::vector<uint8_t> data;               // Instruction data

    SolanaInstruction() = default;
    SolanaInstruction(const SolanaPublicKey& pid,
                     const std::vector<SolanaPublicKey>& accts,
                     const std::vector<uint8_t>& d)
        : program_id(pid), accounts(accts), data(d) {}
};

/**
 * Solana Transaction - A complete transaction
 */
struct SolanaTransaction {
    std::vector<SolanaPublicKey> signatures;     // Transaction signatures
    SolanaPublicKey fee_payer;                   // Account paying fees
    SolanaPublicKey recent_blockhash;            // Recent blockhash for validity
    std::vector<SolanaInstruction> instructions; // Instructions to execute

    SolanaTransaction() = default;
};

/**
 * Account Metadata - Permissions for transaction accounts
 */
struct AccountMeta {
    SolanaPublicKey pubkey;
    bool is_signer;
    bool is_writable;

    AccountMeta(const SolanaPublicKey& pk, bool signer, bool writable)
        : pubkey(pk), is_signer(signer), is_writable(writable) {}
};

/**
 * SVM Execution Result
 */
struct SVMResult {
    enum Status {
        SUCCESS = 0,
        ACCOUNT_NOT_FOUND = 1,
        INSUFFICIENT_FUNDS = 2,
        INVALID_INSTRUCTION = 3,
        PROGRAM_FAILURE = 4,
        ACCOUNT_DATA_TOO_SMALL = 5,
        INVALID_ACCOUNT_OWNER = 6,
        MISSING_REQUIRED_SIGNATURE = 7,
        READONLY_ACCOUNT_MODIFIED = 8,
        EXECUTABLE_MODIFIED = 9,
        RENT_NOT_EXEMPT = 10,
        UNSUPPORTED_PROGRAM = 11,
        CPI_FAILED = 12,
        INVALID_ACCOUNT_DATA = 13,
        ACCOUNT_ALREADY_INITIALIZED = 14,
        UNINITIALIZED_ACCOUNT = 15,
        INTERNAL_ERROR = -1
    };

    Status status;
    uint64_t compute_units_consumed;
    std::vector<uint8_t> return_data;
    std::string error_message;
    std::vector<std::string> logs;

    SVMResult() : status(INTERNAL_ERROR), compute_units_consumed(0) {}

    bool IsSuccess() const { return status == SUCCESS; }
};

/**
 * SVM Executor - Main execution engine for Solana-style contracts
 *
 * Note: This is the state management layer. Actual BPF bytecode execution
 * requires rbpf or similar library integration (TODO).
 */
class SVMExecutor {
public:
    // Built-in program addresses (Solana-compatible)
    static const SolanaPublicKey SYSTEM_PROGRAM_ID;
    static const SolanaPublicKey TOKEN_PROGRAM_ID;
    static const SolanaPublicKey ASSOCIATED_TOKEN_PROGRAM_ID;
    static const SolanaPublicKey RENT_PROGRAM_ID;

private:
    std::map<SolanaPublicKey, SolanaAccount> accounts_;
    uint64_t slot_;                     // Current slot number
    uint64_t compute_budget_;           // Compute units limit
    uint64_t compute_units_remaining_;

    // Checkpoint for state rollback
    struct Checkpoint {
        std::map<SolanaPublicKey, SolanaAccount> accounts_snapshot;
        uint64_t compute_units_remaining;
    };
    std::vector<Checkpoint> checkpoints_;

public:
    SVMExecutor();
    ~SVMExecutor();

    // Account management
    bool CreateAccount(const SolanaPublicKey& address, uint64_t lamports = 0,
                      const SolanaPublicKey& owner = SolanaPublicKey{});
    bool AccountExists(const SolanaPublicKey& address) const;
    SolanaAccount* GetAccount(const SolanaPublicKey& address);
    const SolanaAccount* GetAccount(const SolanaPublicKey& address) const;
    bool DeleteAccount(const SolanaPublicKey& address);

    // Balance operations
    bool SetBalance(const SolanaPublicKey& address, uint64_t lamports);
    uint64_t GetBalance(const SolanaPublicKey& address) const;
    bool Transfer(const SolanaPublicKey& from, const SolanaPublicKey& to, uint64_t lamports);

    // Account data operations
    bool SetAccountData(const SolanaPublicKey& address, const std::vector<uint8_t>& data);
    std::vector<uint8_t> GetAccountData(const SolanaPublicKey& address) const;
    bool ResizeAccountData(const SolanaPublicKey& address, size_t new_size);

    // Account ownership
    bool SetAccountOwner(const SolanaPublicKey& address, const SolanaPublicKey& owner);
    SolanaPublicKey GetAccountOwner(const SolanaPublicKey& address) const;

    // Executable accounts (programs)
    bool SetExecutable(const SolanaPublicKey& address, bool executable);
    bool IsExecutable(const SolanaPublicKey& address) const;
    bool DeployProgram(const SolanaPublicKey& program_address, const std::vector<uint8_t>& bytecode);

    // Transaction execution
    SVMResult ExecuteTransaction(const SolanaTransaction& tx);
    SVMResult ExecuteInstruction(const SolanaInstruction& instruction,
                                const std::vector<AccountMeta>& account_metas,
                                const SolanaPublicKey& fee_payer);

    // Built-in program handlers
    SVMResult ExecuteSystemProgram(const SolanaInstruction& instruction,
                                  const std::vector<AccountMeta>& account_metas);
    SVMResult ExecuteTokenProgram(const SolanaInstruction& instruction,
                                 const std::vector<AccountMeta>& account_metas);

    // Rent calculations
    uint64_t CalculateRentExemption(size_t data_size) const;
    uint64_t CalculateTransactionFee(const SolanaTransaction& tx) const;
    bool IsRentExempt(const SolanaPublicKey& address) const;

    // Program Derived Addresses (PDAs)
    std::pair<SolanaPublicKey, uint8_t> FindProgramAddress(
        const std::vector<std::vector<uint8_t>>& seeds,
        const SolanaPublicKey& program_id) const;
    bool TryFindProgramAddress(const std::vector<std::vector<uint8_t>>& seeds,
                              const SolanaPublicKey& program_id,
                              SolanaPublicKey* address, uint8_t* bump_seed) const;

    // Slot management
    void SetSlot(uint64_t slot);
    uint64_t GetSlot() const;

    // Compute budget
    void SetComputeBudget(uint64_t compute_units);
    uint64_t GetComputeBudget() const;
    uint64_t GetRemainingComputeUnits() const;

    // State management
    void CreateCheckpoint();
    void RevertToCheckpoint();
    void CommitCheckpoint();
    void ClearState();

    // Validation
    bool ValidateTransaction(const SolanaTransaction& tx) const;
    bool ValidateInstruction(const SolanaInstruction& instruction) const;

    // Statistics
    size_t GetAccountCount() const { return accounts_.size(); }

private:
    bool InitializeSVM();
    void InitializeBuiltinPrograms();
    bool ConsumeComputeUnits(uint64_t units);

    // System program instruction handlers
    SVMResult HandleCreateAccount(const std::vector<uint8_t>& data,
                                 const std::vector<AccountMeta>& account_metas);
    SVMResult HandleAssign(const std::vector<uint8_t>& data,
                          const std::vector<AccountMeta>& account_metas);
    SVMResult HandleTransfer(const std::vector<uint8_t>& data,
                            const std::vector<AccountMeta>& account_metas);
};

// Utility functions
SolanaPublicKey StringToPublicKey(const std::string& pubkey_str);
std::string PublicKeyToString(const SolanaPublicKey& pubkey);
std::vector<uint8_t> PublicKeyToBytes(const SolanaPublicKey& pubkey);
SolanaPublicKey BytesToPublicKey(const std::vector<uint8_t>& bytes);

// Hash function for SolanaPublicKey in std::map
struct SolanaPublicKeyHash {
    std::size_t operator()(const SolanaPublicKey& key) const;
};

// Convert between IOC addresses and Solana addresses
SolanaPublicKey IOCAddressToSolana(const std::string& ioc_address);
std::string SolanaToIOCAddress(const SolanaPublicKey& solana_address);

} // namespace dions2

#endif // DIONS2_SVM_H
