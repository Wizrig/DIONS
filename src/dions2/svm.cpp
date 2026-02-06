// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 SVM Zone - Solana Virtual Machine executor implementation
// Phase 0: State management infrastructure

#include "svm.h"
#include "../util.h"  // For Hash()

#include <algorithm>
#include <cassert>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace dions2 {

//
// Built-in Program IDs (Solana-compatible addresses)
//

const SolanaPublicKey SVMExecutor::SYSTEM_PROGRAM_ID = {{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
}};

const SolanaPublicKey SVMExecutor::TOKEN_PROGRAM_ID = {{
    0x06, 0xdd, 0xf6, 0xe1, 0xd7, 0x65, 0xa1, 0x93,
    0xd9, 0xcb, 0xe1, 0x46, 0xce, 0xeb, 0x79, 0xac,
    0x1c, 0xb4, 0x85, 0xed, 0x5f, 0x5b, 0x37, 0x91,
    0x3a, 0x8c, 0xf5, 0x85, 0x7e, 0xff, 0x00, 0xa9
}};

const SolanaPublicKey SVMExecutor::ASSOCIATED_TOKEN_PROGRAM_ID = {{
    0x8c, 0x97, 0x25, 0x8f, 0x4e, 0x24, 0x89, 0xf1,
    0xbb, 0x3d, 0x10, 0x29, 0x14, 0x8e, 0x0d, 0x83,
    0x0b, 0x5a, 0x13, 0x99, 0xda, 0xfa, 0x7c, 0x65,
    0x14, 0xdd, 0x71, 0x4d, 0x78, 0x13, 0x2d, 0x8c
}};

const SolanaPublicKey SVMExecutor::RENT_PROGRAM_ID = {{
    0x06, 0xa7, 0xd5, 0x17, 0x18, 0x72, 0x57, 0xd2,
    0x04, 0xa7, 0x5b, 0x29, 0x01, 0x2a, 0x6d, 0xd5,
    0xc7, 0x38, 0xc8, 0x24, 0x46, 0x33, 0x6f, 0x9a,
    0x82, 0x67, 0xbf, 0xed, 0x69, 0x03, 0x2e, 0xa1
}};

//
// Utility Functions
//

SolanaPublicKey StringToPublicKey(const std::string& pubkey_str) {
    SolanaPublicKey pubkey;
    // TODO: Implement base58 decoding for real Solana addresses
    // For now, treat as hex if starts with 0x, otherwise zero-fill
    if (pubkey_str.length() >= 2 && pubkey_str.substr(0, 2) == "0x") {
        std::string hex = pubkey_str.substr(2);
        for (size_t i = 0; i < 32 && i * 2 < hex.length(); ++i) {
            pubkey[i] = static_cast<uint8_t>(std::stoul(hex.substr(i * 2, 2), nullptr, 16));
        }
    } else {
        pubkey.fill(0);
    }
    return pubkey;
}

std::string PublicKeyToString(const SolanaPublicKey& pubkey) {
    // Return hex representation
    // TODO: Implement base58 encoding for Solana compatibility
    std::stringstream ss;
    ss << "0x" << std::hex << std::setfill('0');
    for (uint8_t byte : pubkey) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

std::vector<uint8_t> PublicKeyToBytes(const SolanaPublicKey& pubkey) {
    return std::vector<uint8_t>(pubkey.begin(), pubkey.end());
}

SolanaPublicKey BytesToPublicKey(const std::vector<uint8_t>& bytes) {
    SolanaPublicKey pubkey;
    if (bytes.size() == 32) {
        std::copy(bytes.begin(), bytes.end(), pubkey.begin());
    } else {
        pubkey.fill(0);
    }
    return pubkey;
}

SolanaPublicKey IOCAddressToSolana(const std::string& ioc_address) {
    // Hash IOC address to get 32-byte Solana-style address
    std::vector<uint8_t> data(ioc_address.begin(), ioc_address.end());
    uint256 hash = Hash(data.begin(), data.end());

    SolanaPublicKey solana_addr;
    memcpy(solana_addr.data(), hash.begin(), 32);
    return solana_addr;
}

std::string SolanaToIOCAddress(const SolanaPublicKey& solana_address) {
    // Return hex (proper mapping needs lookup table)
    return PublicKeyToString(solana_address);
}

std::size_t SolanaPublicKeyHash::operator()(const SolanaPublicKey& key) const {
    std::size_t hash = 0;
    for (size_t i = 0; i < key.size(); i += sizeof(std::size_t)) {
        std::size_t chunk = 0;
        std::memcpy(&chunk, &key[i], std::min(sizeof(std::size_t), key.size() - i));
        hash ^= std::hash<std::size_t>{}(chunk) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    return hash;
}

//
// SVMExecutor Implementation
//

SVMExecutor::SVMExecutor()
    : slot_(0), compute_budget_(200000), compute_units_remaining_(200000) {
    InitializeSVM();
}

SVMExecutor::~SVMExecutor() {
    // Cleanup
}

bool SVMExecutor::InitializeSVM() {
    InitializeBuiltinPrograms();
    return true;
}

void SVMExecutor::InitializeBuiltinPrograms() {
    // System Program
    CreateAccount(SYSTEM_PROGRAM_ID, 1, SYSTEM_PROGRAM_ID);
    SetExecutable(SYSTEM_PROGRAM_ID, true);

    // Token Program
    CreateAccount(TOKEN_PROGRAM_ID, 1, TOKEN_PROGRAM_ID);
    SetExecutable(TOKEN_PROGRAM_ID, true);

    // Associated Token Program
    CreateAccount(ASSOCIATED_TOKEN_PROGRAM_ID, 1, ASSOCIATED_TOKEN_PROGRAM_ID);
    SetExecutable(ASSOCIATED_TOKEN_PROGRAM_ID, true);

    // Rent Program
    CreateAccount(RENT_PROGRAM_ID, 1, RENT_PROGRAM_ID);
    SetExecutable(RENT_PROGRAM_ID, true);
}

//
// Account Management
//

bool SVMExecutor::CreateAccount(const SolanaPublicKey& address, uint64_t lamports,
                               const SolanaPublicKey& owner) {
    if (AccountExists(address)) {
        return false; // Already exists
    }

    SolanaAccount account(lamports, owner);
    accounts_[address] = account;
    return true;
}

bool SVMExecutor::AccountExists(const SolanaPublicKey& address) const {
    return accounts_.find(address) != accounts_.end();
}

SolanaAccount* SVMExecutor::GetAccount(const SolanaPublicKey& address) {
    auto it = accounts_.find(address);
    return (it != accounts_.end()) ? &it->second : nullptr;
}

const SolanaAccount* SVMExecutor::GetAccount(const SolanaPublicKey& address) const {
    auto it = accounts_.find(address);
    return (it != accounts_.end()) ? &it->second : nullptr;
}

bool SVMExecutor::DeleteAccount(const SolanaPublicKey& address) {
    return accounts_.erase(address) > 0;
}

//
// Balance Operations
//

bool SVMExecutor::SetBalance(const SolanaPublicKey& address, uint64_t lamports) {
    SolanaAccount* account = GetAccount(address);
    if (!account) {
        return false;
    }
    account->lamports = lamports;
    return true;
}

uint64_t SVMExecutor::GetBalance(const SolanaPublicKey& address) const {
    const SolanaAccount* account = GetAccount(address);
    return account ? account->lamports : 0;
}

bool SVMExecutor::Transfer(const SolanaPublicKey& from, const SolanaPublicKey& to, uint64_t lamports) {
    SolanaAccount* from_account = GetAccount(from);
    SolanaAccount* to_account = GetAccount(to);

    if (!from_account || !to_account) {
        return false;
    }

    if (from_account->lamports < lamports) {
        return false; // Insufficient funds
    }

    from_account->lamports -= lamports;
    to_account->lamports += lamports;
    return true;
}

//
// Account Data Operations
//

bool SVMExecutor::SetAccountData(const SolanaPublicKey& address, const std::vector<uint8_t>& data) {
    SolanaAccount* account = GetAccount(address);
    if (!account) {
        return false;
    }
    account->data = data;
    return true;
}

std::vector<uint8_t> SVMExecutor::GetAccountData(const SolanaPublicKey& address) const {
    const SolanaAccount* account = GetAccount(address);
    return account ? account->data : std::vector<uint8_t>{};
}

bool SVMExecutor::ResizeAccountData(const SolanaPublicKey& address, size_t new_size) {
    SolanaAccount* account = GetAccount(address);
    if (!account) {
        return false;
    }
    account->data.resize(new_size);
    return true;
}

//
// Account Ownership
//

bool SVMExecutor::SetAccountOwner(const SolanaPublicKey& address, const SolanaPublicKey& owner) {
    SolanaAccount* account = GetAccount(address);
    if (!account) {
        return false;
    }
    account->owner = owner;
    return true;
}

SolanaPublicKey SVMExecutor::GetAccountOwner(const SolanaPublicKey& address) const {
    const SolanaAccount* account = GetAccount(address);
    return account ? account->owner : SolanaPublicKey{};
}

//
// Executable Accounts
//

bool SVMExecutor::SetExecutable(const SolanaPublicKey& address, bool executable) {
    SolanaAccount* account = GetAccount(address);
    if (!account) {
        return false;
    }
    account->executable = executable;
    return true;
}

bool SVMExecutor::IsExecutable(const SolanaPublicKey& address) const {
    const SolanaAccount* account = GetAccount(address);
    return account ? account->executable : false;
}

bool SVMExecutor::DeployProgram(const SolanaPublicKey& program_address,
                               const std::vector<uint8_t>& bytecode) {
    if (!CreateAccount(program_address, CalculateRentExemption(bytecode.size()), SYSTEM_PROGRAM_ID)) {
        return false;
    }

    SetAccountData(program_address, bytecode);
    SetExecutable(program_address, true);
    return true;
}

//
// Transaction Execution
//

SVMResult SVMExecutor::ExecuteTransaction(const SolanaTransaction& tx) {
    SVMResult result;
    result.status = SVMResult::SUCCESS;
    result.compute_units_consumed = 0;

    if (!ValidateTransaction(tx)) {
        result.status = SVMResult::INVALID_INSTRUCTION;
        result.error_message = "Invalid transaction";
        return result;
    }

    // Reset compute units
    compute_units_remaining_ = compute_budget_;

    // Create checkpoint
    CreateCheckpoint();

    try {
        // Deduct fee
        uint64_t fee = CalculateTransactionFee(tx);
        if (GetBalance(tx.fee_payer) < fee) {
            result.status = SVMResult::INSUFFICIENT_FUNDS;
            result.error_message = "Insufficient funds for fee";
            RevertToCheckpoint();
            return result;
        }

        SolanaAccount* fee_payer_account = GetAccount(tx.fee_payer);
        fee_payer_account->lamports -= fee;

        // Execute each instruction
        for (const auto& instruction : tx.instructions) {
            std::vector<AccountMeta> account_metas;
            for (const auto& account : instruction.accounts) {
                account_metas.emplace_back(account, false, true);
            }

            SVMResult instr_result = ExecuteInstruction(instruction, account_metas, tx.fee_payer);

            if (!instr_result.IsSuccess()) {
                result = instr_result;
                RevertToCheckpoint();
                return result;
            }

            result.compute_units_consumed += instr_result.compute_units_consumed;
            result.logs.insert(result.logs.end(),
                              instr_result.logs.begin(),
                              instr_result.logs.end());
        }

        CommitCheckpoint();

    } catch (const std::exception& e) {
        result.status = SVMResult::INTERNAL_ERROR;
        result.error_message = e.what();
        RevertToCheckpoint();
        return result;
    }

    result.compute_units_consumed = compute_budget_ - compute_units_remaining_;
    return result;
}

SVMResult SVMExecutor::ExecuteInstruction(const SolanaInstruction& instruction,
                                         const std::vector<AccountMeta>& account_metas,
                                         const SolanaPublicKey& fee_payer) {
    SVMResult result;
    result.status = SVMResult::SUCCESS;

    if (!ConsumeComputeUnits(1000)) { // Base instruction cost
        result.status = SVMResult::INTERNAL_ERROR;
        result.error_message = "Insufficient compute units";
        return result;
    }

    // Check built-in programs
    if (instruction.program_id == SYSTEM_PROGRAM_ID) {
        return ExecuteSystemProgram(instruction, account_metas);
    } else if (instruction.program_id == TOKEN_PROGRAM_ID) {
        return ExecuteTokenProgram(instruction, account_metas);
    } else {
        // Custom program execution (requires BPF runtime)
        result.status = SVMResult::UNSUPPORTED_PROGRAM;
        result.error_message = "Custom program execution not yet implemented";
        return result;
    }
}

//
// Built-in Program Handlers
//

SVMResult SVMExecutor::ExecuteSystemProgram(const SolanaInstruction& instruction,
                                           const std::vector<AccountMeta>& account_metas) {
    SVMResult result;
    result.status = SVMResult::SUCCESS;

    if (instruction.data.empty()) {
        result.status = SVMResult::INVALID_INSTRUCTION;
        result.error_message = "Empty system program instruction";
        return result;
    }

    uint32_t instruction_type = *reinterpret_cast<const uint32_t*>(instruction.data.data());

    switch (instruction_type) {
        case 0: // CreateAccount
            return HandleCreateAccount(instruction.data, account_metas);
        case 1: // Assign
            return HandleAssign(instruction.data, account_metas);
        case 2: // Transfer
            return HandleTransfer(instruction.data, account_metas);
        default:
            result.status = SVMResult::INVALID_INSTRUCTION;
            result.error_message = "Unknown system program instruction";
            break;
    }

    return result;
}

SVMResult SVMExecutor::ExecuteTokenProgram(const SolanaInstruction& instruction,
                                          const std::vector<AccountMeta>& account_metas) {
    SVMResult result;
    result.status = SVMResult::UNSUPPORTED_PROGRAM;
    result.error_message = "Token program not yet implemented";
    return result;
}

//
// System Program Instruction Handlers
//

SVMResult SVMExecutor::HandleCreateAccount(const std::vector<uint8_t>& data,
                                          const std::vector<AccountMeta>& account_metas) {
    SVMResult result;
    result.status = SVMResult::SUCCESS;

    if (account_metas.size() < 2 || data.size() < 52) {
        result.status = SVMResult::INVALID_INSTRUCTION;
        result.error_message = "Invalid CreateAccount instruction";
        return result;
    }

    const SolanaPublicKey& from = account_metas[0].pubkey;
    const SolanaPublicKey& to = account_metas[1].pubkey;

    // Parse instruction data
    uint64_t lamports = *reinterpret_cast<const uint64_t*>(&data[4]);
    uint64_t space = *reinterpret_cast<const uint64_t*>(&data[12]);
    SolanaPublicKey owner;
    std::memcpy(owner.data(), &data[20], 32);

    // Check source has sufficient balance
    if (GetBalance(from) < lamports) {
        result.status = SVMResult::INSUFFICIENT_FUNDS;
        result.error_message = "Insufficient funds for account creation";
        return result;
    }

    // Create the account
    if (!CreateAccount(to, 0, owner)) {
        result.status = SVMResult::ACCOUNT_ALREADY_INITIALIZED;
        result.error_message = "Account already exists";
        return result;
    }

    // Transfer lamports
    Transfer(from, to, lamports);

    // Resize data
    ResizeAccountData(to, static_cast<size_t>(space));

    return result;
}

SVMResult SVMExecutor::HandleAssign(const std::vector<uint8_t>& data,
                                   const std::vector<AccountMeta>& account_metas) {
    SVMResult result;
    result.status = SVMResult::SUCCESS;

    if (account_metas.empty() || data.size() < 36) {
        result.status = SVMResult::INVALID_INSTRUCTION;
        result.error_message = "Invalid Assign instruction";
        return result;
    }

    const SolanaPublicKey& account = account_metas[0].pubkey;
    SolanaPublicKey new_owner;
    std::memcpy(new_owner.data(), &data[4], 32);

    if (!SetAccountOwner(account, new_owner)) {
        result.status = SVMResult::ACCOUNT_NOT_FOUND;
        result.error_message = "Account not found for assignment";
    }

    return result;
}

SVMResult SVMExecutor::HandleTransfer(const std::vector<uint8_t>& data,
                                     const std::vector<AccountMeta>& account_metas) {
    SVMResult result;
    result.status = SVMResult::SUCCESS;

    if (account_metas.size() < 2 || data.size() < 12) {
        result.status = SVMResult::INVALID_INSTRUCTION;
        result.error_message = "Invalid Transfer instruction";
        return result;
    }

    const SolanaPublicKey& from = account_metas[0].pubkey;
    const SolanaPublicKey& to = account_metas[1].pubkey;
    uint64_t lamports = *reinterpret_cast<const uint64_t*>(&data[4]);

    if (!Transfer(from, to, lamports)) {
        result.status = SVMResult::INSUFFICIENT_FUNDS;
        result.error_message = "Transfer failed - insufficient funds";
    }

    return result;
}

//
// Rent Calculations
//

uint64_t SVMExecutor::CalculateRentExemption(size_t data_size) const {
    // Simplified rent calculation based on Solana's formula
    const uint64_t LAMPORTS_PER_BYTE_YEAR = 3480;  // ~3480 lamports/byte/year
    return (data_size + 128) * LAMPORTS_PER_BYTE_YEAR * 2; // 2 years for exemption
}

uint64_t SVMExecutor::CalculateTransactionFee(const SolanaTransaction& tx) const {
    // 5000 lamports per signature (like Solana)
    const uint64_t BASE_FEE_PER_SIGNATURE = 5000;
    return tx.signatures.size() * BASE_FEE_PER_SIGNATURE;
}

bool SVMExecutor::IsRentExempt(const SolanaPublicKey& address) const {
    const SolanaAccount* account = GetAccount(address);
    if (!account) {
        return false;
    }
    return account->lamports >= CalculateRentExemption(account->data.size());
}

//
// Program Derived Addresses
//

std::pair<SolanaPublicKey, uint8_t> SVMExecutor::FindProgramAddress(
    const std::vector<std::vector<uint8_t>>& seeds,
    const SolanaPublicKey& program_id) const {

    SolanaPublicKey address;
    uint8_t bump_seed;

    if (TryFindProgramAddress(seeds, program_id, &address, &bump_seed)) {
        return {address, bump_seed};
    }

    return {SolanaPublicKey{}, 0};
}

bool SVMExecutor::TryFindProgramAddress(const std::vector<std::vector<uint8_t>>& seeds,
                                       const SolanaPublicKey& program_id,
                                       SolanaPublicKey* address,
                                       uint8_t* bump_seed) const {
    // Try bump seeds from 255 down to 1
    for (int bump = 255; bump >= 1; --bump) {
        std::vector<uint8_t> hash_input;

        // Concatenate all seeds
        for (const auto& seed : seeds) {
            hash_input.insert(hash_input.end(), seed.begin(), seed.end());
        }

        // Add bump seed
        hash_input.push_back(static_cast<uint8_t>(bump));

        // Add program ID
        hash_input.insert(hash_input.end(), program_id.begin(), program_id.end());

        // Add PDA marker
        const char* marker = "ProgramDerivedAddress";
        hash_input.insert(hash_input.end(), marker, marker + strlen(marker));

        // Hash to get candidate address
        uint256 hash = Hash(hash_input.begin(), hash_input.end());

        SolanaPublicKey candidate;
        memcpy(candidate.data(), hash.begin(), 32);

        // TODO: Check if this is a valid PDA (not on curve)
        // For now, assume valid
        *address = candidate;
        *bump_seed = static_cast<uint8_t>(bump);
        return true;
    }

    return false;
}

//
// Compute Budget Management
//

bool SVMExecutor::ConsumeComputeUnits(uint64_t units) {
    if (compute_units_remaining_ < units) {
        return false;
    }
    compute_units_remaining_ -= units;
    return true;
}

void SVMExecutor::SetComputeBudget(uint64_t compute_units) {
    compute_budget_ = compute_units;
    compute_units_remaining_ = compute_units;
}

uint64_t SVMExecutor::GetComputeBudget() const {
    return compute_budget_;
}

uint64_t SVMExecutor::GetRemainingComputeUnits() const {
    return compute_units_remaining_;
}

//
// Slot Management
//

void SVMExecutor::SetSlot(uint64_t slot) {
    slot_ = slot;
}

uint64_t SVMExecutor::GetSlot() const {
    return slot_;
}

//
// State Management
//

void SVMExecutor::CreateCheckpoint() {
    Checkpoint cp;
    cp.accounts_snapshot = accounts_;
    cp.compute_units_remaining = compute_units_remaining_;
    checkpoints_.push_back(cp);
}

void SVMExecutor::RevertToCheckpoint() {
    if (!checkpoints_.empty()) {
        const auto& cp = checkpoints_.back();
        accounts_ = cp.accounts_snapshot;
        compute_units_remaining_ = cp.compute_units_remaining;
        checkpoints_.pop_back();
    }
}

void SVMExecutor::CommitCheckpoint() {
    if (!checkpoints_.empty()) {
        checkpoints_.pop_back();
    }
}

void SVMExecutor::ClearState() {
    accounts_.clear();
    checkpoints_.clear();
    compute_units_remaining_ = compute_budget_;
    slot_ = 0;
    InitializeBuiltinPrograms();
}

//
// Validation
//

bool SVMExecutor::ValidateTransaction(const SolanaTransaction& tx) const {
    if (tx.instructions.empty()) {
        return false;
    }

    if (!AccountExists(tx.fee_payer)) {
        return false;
    }

    for (const auto& instruction : tx.instructions) {
        if (!ValidateInstruction(instruction)) {
            return false;
        }
    }

    return true;
}

bool SVMExecutor::ValidateInstruction(const SolanaInstruction& instruction) const {
    // Check if program account exists and is executable
    if (!AccountExists(instruction.program_id)) {
        return false;
    }

    if (!IsExecutable(instruction.program_id)) {
        return false;
    }

    return true;
}

} // namespace dions2
