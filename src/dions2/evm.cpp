// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 EVM Zone - Ethereum Virtual Machine executor implementation
// Phase 0: State management infrastructure

#include "evm.h"
#include "../util.h"  // For Hash()

#include <algorithm>
#include <cassert>
#include <cstring>
#include <sstream>
#include <iomanip>

// TODO: Enable evmone integration once build system configured
// #include <evmone/evmone.h>

namespace dions2 {

//
// Utility Functions
//

std::vector<uint8_t> HexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    std::string clean_hex = hex;

    // Remove 0x prefix if present
    if (clean_hex.length() >= 2 && clean_hex.substr(0, 2) == "0x") {
        clean_hex = clean_hex.substr(2);
    }

    // Ensure even length
    if (clean_hex.length() % 2 != 0) {
        clean_hex = "0" + clean_hex;
    }

    for (size_t i = 0; i < clean_hex.length(); i += 2) {
        std::string byte_str = clean_hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
        bytes.push_back(byte);
    }

    return bytes;
}

std::string BytesToHex(const std::vector<uint8_t>& bytes) {
    std::stringstream ss;
    ss << "0x";
    for (uint8_t byte : bytes) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

std::vector<uint8_t> StringToEVMAddress(const std::string& address_str) {
    return HexToBytes(address_str);
}

std::string EVMAddressToString(const std::vector<uint8_t>& address) {
    return BytesToHex(address);
}

std::vector<uint8_t> IOCAddressToEVM(const std::string& ioc_address) {
    // Simple conversion: hash IOC address to 20 bytes
    // In production, use proper address derivation
    std::vector<uint8_t> data(ioc_address.begin(), ioc_address.end());
    uint256 hash = Hash(data.begin(), data.end());

    std::vector<uint8_t> evm_addr(20);
    memcpy(evm_addr.data(), hash.begin(), 20);
    return evm_addr;
}

std::string EVMToIOCAddress(const std::vector<uint8_t>& evm_address) {
    // Return hex representation (proper conversion needs lookup)
    return BytesToHex(evm_address);
}

//
// EVMExecutor Implementation
//

EVMExecutor::EVMExecutor() : evm_instance_(nullptr) {
    InitializeEVM();
}

EVMExecutor::~EVMExecutor() {
    CleanupEVM();
}

bool EVMExecutor::InitializeEVM() {
    // TODO: Initialize evmone instance
    // evm_instance_ = evmc_create_evmone();
    evm_instance_ = nullptr; // Placeholder
    return true;
}

void EVMExecutor::CleanupEVM() {
    if (evm_instance_) {
        // TODO: Cleanup evmone
        // evmc_vm* vm = static_cast<evmc_vm*>(evm_instance_);
        // vm->destroy(vm);
        evm_instance_ = nullptr;
    }
}

//
// Account Management
//

bool EVMExecutor::CreateAccount(const std::vector<uint8_t>& address,
                               const std::vector<uint8_t>& balance) {
    if (!ValidateAddress(address)) {
        return false;
    }

    if (AccountExists(address)) {
        return false; // Already exists
    }

    EVMAccount account;
    if (!balance.empty()) {
        account.balance = balance;
    } else {
        account.balance = {0}; // Default zero balance
    }

    accounts_[address] = account;
    return true;
}

bool EVMExecutor::AccountExists(const std::vector<uint8_t>& address) const {
    return accounts_.find(address) != accounts_.end();
}

EVMAccount* EVMExecutor::GetAccount(const std::vector<uint8_t>& address) {
    auto it = accounts_.find(address);
    return (it != accounts_.end()) ? &it->second : nullptr;
}

const EVMAccount* EVMExecutor::GetAccount(const std::vector<uint8_t>& address) const {
    auto it = accounts_.find(address);
    return (it != accounts_.end()) ? &it->second : nullptr;
}

bool EVMExecutor::DeleteAccount(const std::vector<uint8_t>& address) {
    return accounts_.erase(address) > 0;
}

//
// Balance Operations
//

bool EVMExecutor::SetBalance(const std::vector<uint8_t>& address,
                            const std::vector<uint8_t>& balance) {
    EVMAccount* account = GetAccount(address);
    if (!account) {
        // Auto-create account if it doesn't exist
        if (!CreateAccount(address, balance)) {
            return false;
        }
        return true;
    }

    account->balance = balance;
    return true;
}

std::vector<uint8_t> EVMExecutor::GetBalance(const std::vector<uint8_t>& address) const {
    const EVMAccount* account = GetAccount(address);
    if (account) {
        return account->balance;
    }
    return {0}; // Zero for non-existent accounts
}

bool EVMExecutor::Transfer(const std::vector<uint8_t>& from,
                          const std::vector<uint8_t>& to,
                          const std::vector<uint8_t>& amount) {
    EVMAccount* from_account = GetAccount(from);
    if (!from_account) {
        return false; // Sender doesn't exist
    }

    // Check sufficient balance
    if (CompareBigInts(from_account->balance, amount) < 0) {
        return false; // Insufficient balance
    }

    // Create receiver if needed
    if (!AccountExists(to)) {
        CreateAccount(to);
    }

    EVMAccount* to_account = GetAccount(to);
    if (!to_account) {
        return false;
    }

    // Perform transfer
    from_account->balance = SubBigInts(from_account->balance, amount);
    to_account->balance = AddBigInts(to_account->balance, amount);

    return true;
}

//
// Code Operations
//

bool EVMExecutor::SetCode(const std::vector<uint8_t>& address,
                         const std::vector<uint8_t>& code) {
    EVMAccount* account = GetAccount(address);
    if (!account) {
        if (!CreateAccount(address)) {
            return false;
        }
        account = GetAccount(address);
    }

    account->code = code;
    return true;
}

std::vector<uint8_t> EVMExecutor::GetCode(const std::vector<uint8_t>& address) const {
    const EVMAccount* account = GetAccount(address);
    if (account) {
        return account->code;
    }
    return {}; // Empty code
}

std::vector<uint8_t> EVMExecutor::GetCodeHash(const std::vector<uint8_t>& address) const {
    std::vector<uint8_t> code = GetCode(address);
    if (code.empty()) {
        // Return empty code hash (keccak256 of empty string)
        return HexToBytes("c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
    }

    // Hash the code
    uint256 hash = Hash(code.begin(), code.end());
    std::vector<uint8_t> hash_bytes(32);
    memcpy(hash_bytes.data(), hash.begin(), 32);
    return hash_bytes;
}

//
// Storage Operations
//

bool EVMExecutor::SetStorage(const std::vector<uint8_t>& address,
                            const std::vector<uint8_t>& key,
                            const std::vector<uint8_t>& value) {
    EVMAccount* account = GetAccount(address);
    if (!account) {
        return false;
    }

    account->storage[key] = value;
    return true;
}

std::vector<uint8_t> EVMExecutor::GetStorage(const std::vector<uint8_t>& address,
                                            const std::vector<uint8_t>& key) const {
    const EVMAccount* account = GetAccount(address);
    if (account) {
        auto it = account->storage.find(key);
        if (it != account->storage.end()) {
            return it->second;
        }
    }
    return {0}; // Zero for non-existent storage
}

//
// Transaction Execution
//

EVMResult EVMExecutor::ExecuteTransaction(const EVMTransaction& tx) {
    EVMResult result;

    if (!ValidateTransaction(tx)) {
        result.status = EVMResult::INVALID_INSTRUCTION;
        result.error_message = "Invalid transaction";
        return result;
    }

    if (tx.to.empty()) {
        // Contract deployment
        std::vector<uint8_t> contract_address;
        return DeployContract(tx.from, tx.data, tx.gas_limit, tx.value, &contract_address);
    } else {
        // Regular call
        return Call(tx.from, tx.to, tx.data, tx.gas_limit, tx.value);
    }
}

EVMResult EVMExecutor::Call(const std::vector<uint8_t>& from,
                           const std::vector<uint8_t>& to,
                           const std::vector<uint8_t>& data,
                           uint64_t gas_limit,
                           const std::vector<uint8_t>& value) {
    EVMResult result;

    // Basic validation
    if (!AccountExists(from)) {
        result.status = EVMResult::INSUFFICIENT_BALANCE;
        result.error_message = "Sender account does not exist";
        return result;
    }

    // TODO: Execute EVM bytecode using evmone
    // For now, return success (state management only)
    result.status = EVMResult::SUCCESS;
    result.gas_used = 21000; // Basic gas cost
    result.output = {};

    return result;
}

EVMResult EVMExecutor::DeployContract(const std::vector<uint8_t>& from,
                                     const std::vector<uint8_t>& bytecode,
                                     uint64_t gas_limit,
                                     const std::vector<uint8_t>& value,
                                     std::vector<uint8_t>* contract_address) {
    EVMResult result;

    // Validate deployer
    if (!AccountExists(from)) {
        result.status = EVMResult::INSUFFICIENT_BALANCE;
        result.error_message = "Deployer account does not exist";
        return result;
    }

    // Generate contract address
    EVMAccount* deployer = GetAccount(from);
    std::vector<uint8_t> addr = GenerateContractAddress(from, deployer->nonce);

    if (contract_address) {
        *contract_address = addr;
    }

    // Create contract account
    if (!CreateAccount(addr)) {
        result.status = EVMResult::INTERNAL_ERROR;
        result.error_message = "Failed to create contract account";
        return result;
    }

    // Set contract code
    if (!SetCode(addr, bytecode)) {
        result.status = EVMResult::INTERNAL_ERROR;
        result.error_message = "Failed to set contract code";
        return result;
    }

    // Transfer value if specified
    if (!value.empty() && CompareBigInts(value, {0}) > 0) {
        if (!Transfer(from, addr, value)) {
            result.status = EVMResult::INSUFFICIENT_BALANCE;
            result.error_message = "Insufficient balance for contract endowment";
            return result;
        }
    }

    // Increment deployer nonce
    deployer->nonce++;

    // TODO: Execute constructor using evmone
    result.status = EVMResult::SUCCESS;
    result.gas_used = 53000; // Approximate deployment cost
    result.output = addr;

    return result;
}

//
// Address Generation
//

std::vector<uint8_t> EVMExecutor::GenerateContractAddress(const std::vector<uint8_t>& deployer,
                                                         uint64_t nonce) const {
    // CREATE: keccak256(RLP([deployer, nonce]))[12:]
    // Simplified: hash(deployer || nonce)

    std::vector<uint8_t> data = deployer;

    // Append nonce as bytes (big-endian)
    for (int i = 7; i >= 0; --i) {
        data.push_back(static_cast<uint8_t>((nonce >> (i * 8)) & 0xFF));
    }

    uint256 hash = Hash(data.begin(), data.end());

    // Take last 20 bytes
    std::vector<uint8_t> address(20);
    memcpy(address.data(), hash.begin() + 12, 20);

    return address;
}

std::vector<uint8_t> EVMExecutor::GenerateCreate2Address(const std::vector<uint8_t>& deployer,
                                                        const std::vector<uint8_t>& salt,
                                                        const std::vector<uint8_t>& init_code_hash) const {
    // CREATE2: keccak256(0xff || deployer || salt || keccak256(init_code))[12:]

    std::vector<uint8_t> data;
    data.push_back(0xff);
    data.insert(data.end(), deployer.begin(), deployer.end());
    data.insert(data.end(), salt.begin(), salt.end());
    data.insert(data.end(), init_code_hash.begin(), init_code_hash.end());

    uint256 hash = Hash(data.begin(), data.end());

    std::vector<uint8_t> address(20);
    memcpy(address.data(), hash.begin() + 12, 20);

    return address;
}

//
// Validation
//

bool EVMExecutor::ValidateAddress(const std::vector<uint8_t>& address) const {
    return address.size() == 20; // Ethereum addresses are 20 bytes
}

bool EVMExecutor::ValidateTransaction(const EVMTransaction& tx) const {
    if (tx.from.size() != 20) return false;
    if (!tx.to.empty() && tx.to.size() != 20) return false;
    if (tx.gas_limit == 0) return false;

    return true;
}

//
// State Management
//

void EVMExecutor::CreateCheckpoint() {
    Checkpoint cp;
    cp.accounts_snapshot = accounts_;
    checkpoints_.push_back(cp);
}

void EVMExecutor::RevertToCheckpoint() {
    if (!checkpoints_.empty()) {
        accounts_ = checkpoints_.back().accounts_snapshot;
        checkpoints_.pop_back();
    }
}

void EVMExecutor::CommitCheckpoint() {
    if (!checkpoints_.empty()) {
        checkpoints_.pop_back();
    }
}

void EVMExecutor::ClearState() {
    accounts_.clear();
    checkpoints_.clear();
}

//
// Big Integer Arithmetic (simplified)
//

std::vector<uint8_t> EVMExecutor::AddBigInts(const std::vector<uint8_t>& a,
                                            const std::vector<uint8_t>& b) const {
    std::vector<uint8_t> result;
    size_t max_size = std::max(a.size(), b.size());
    result.resize(max_size + 1, 0);

    int carry = 0;
    for (size_t i = 0; i < max_size; ++i) {
        int val_a = (i < a.size()) ? a[a.size() - 1 - i] : 0;
        int val_b = (i < b.size()) ? b[b.size() - 1 - i] : 0;
        int sum = val_a + val_b + carry;

        result[result.size() - 1 - i] = sum & 0xFF;
        carry = sum >> 8;
    }

    if (carry) {
        result[0] = carry;
    } else {
        result.erase(result.begin());
    }

    return result;
}

std::vector<uint8_t> EVMExecutor::SubBigInts(const std::vector<uint8_t>& a,
                                            const std::vector<uint8_t>& b) const {
    std::vector<uint8_t> result = a;

    int borrow = 0;
    for (size_t i = 0; i < std::max(a.size(), b.size()); ++i) {
        int val_a = (i < result.size()) ? result[result.size() - 1 - i] : 0;
        int val_b = (i < b.size()) ? b[b.size() - 1 - i] : 0;

        int diff = val_a - val_b - borrow;
        if (diff < 0) {
            diff += 256;
            borrow = 1;
        } else {
            borrow = 0;
        }

        if (i < result.size()) {
            result[result.size() - 1 - i] = diff;
        }
    }

    return result;
}

int EVMExecutor::CompareBigInts(const std::vector<uint8_t>& a,
                               const std::vector<uint8_t>& b) const {
    if (a.size() != b.size()) {
        return (a.size() < b.size()) ? -1 : 1;
    }

    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) {
            return (a[i] < b[i]) ? -1 : 1;
        }
    }

    return 0; // Equal
}

} // namespace dions2
