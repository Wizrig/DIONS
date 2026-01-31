// DIONS 2.0 Devnet Mode
// PoS-friendly automated testing with deterministic prefunded wallets
// Copyright (c) 2026 DIONS Development

#ifndef DIONS_DEVNET_H
#define DIONS_DEVNET_H

#include <string>

// Devnet flag - can be set via -devnet runtime flag
extern bool fDevNet;

// Devnet configuration
namespace DevNet {
    // Deterministic faucet privkey for devnet funding (TESTING ONLY)
    // Pubkey: 04... Address will be computed from this
    const std::string FAUCET_PRIVKEY = "5HwoXVkHoRM8sL2KmNRS217n1g8mPPBomrY7yehCuXC1115WWsh";

    // Devnet genesis reward (large premine to faucet address)
    const int64_t DEVNET_GENESIS_REWARD = 1000000 * 100000000LL; // 1M IOC

    // Devnet maturity settings
    const int DEVNET_COINBASE_MATURITY = 1; // 1 block instead of 100
    const unsigned int DEVNET_STAKE_MIN_AGE = 60; // 1 minute instead of 8 hours
    const int DEVNET_STAKE_MIN_CONFIRMATIONS = 1; // 1 instead of 500

    // Initialize devnet (called from init.cpp)
    void Initialize();

    // Check if devnet is active
    bool IsActive();
}

#endif // DIONS_DEVNET_H
