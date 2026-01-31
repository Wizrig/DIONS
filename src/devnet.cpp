// DIONS 2.0 Devnet Mode Implementation
// Copyright (c) 2026 DIONS Development

#include "devnet.h"
#include "util.h"
#include "main.h"
#include "wallet.h"
#include "init.h"

bool fDevNet = false;

namespace DevNet {

void Initialize()
{
    if (!fDevNet)
        return;

    printf("===================================\n");
    printf("DEVNET MODE ACTIVE - TESTING ONLY\n");
    printf("===================================\n");
    printf("Devnet settings:\n");
    printf("  Coinbase maturity: %d blocks\n", DEVNET_COINBASE_MATURITY);
    printf("  Stake min age: %d seconds\n", DEVNET_STAKE_MIN_AGE);
    printf("  Stake confirmations: %d\n", DEVNET_STAKE_MIN_CONFIRMATIONS);
    printf("===================================\n");

    // Override maturity settings for devnet
    nCoinbaseMaturity = DEVNET_COINBASE_MATURITY;
    nStakeMinAge = DEVNET_STAKE_MIN_AGE;
    nStakeMinConfirmations = DEVNET_STAKE_MIN_CONFIRMATIONS;

    // In devnet, wallet will be auto-funded via genesis block
    // or via dev faucet RPC commands
    if (pwalletMain != NULL)
    {
        printf("Devnet: Wallet loaded, use 'devfaucet' RPC to fund addresses\n");
    }
}

bool IsActive()
{
    return fDevNet;
}

} // namespace DevNet
