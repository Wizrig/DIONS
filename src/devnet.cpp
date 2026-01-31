// DIONS 2.0 Devnet Mode Implementation
// Copyright (c) 2026 DIONS Development

#include "devnet.h"
#include "util.h"
#include "main.h"
#include "wallet.h"
#include "init.h"
#include "base58.h"

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

    // Devnet faucet address (funded via genesis)
    if (pwalletMain != NULL)
    {
        printf("Devnet: Wallet loaded\n");
        printf("Devnet: Faucet privkey: cNn958MydGaReKQxS9p17Zn1qjYbPWx5XrPov8i6syALKEtVS4yH\n");
        printf("Devnet: Faucet pubkey: 02d8019ae39403a4c0b49e98a0be4ed9ad0b1ba20f324fd6268c7455841deddd0d (1M IOC from genesis)\n");
        printf("Devnet: Import faucet key: iocoind -devnet importprivkey cNn958MydGaReKQxS9p17Zn1qjYbPWx5XrPov8i6syALKEtVS4yH\n");
        printf("Devnet: Then use 'devfaucet' and 'devstake' for testing\n");
    }
}

bool IsActive()
{
    return fDevNet;
}

} // namespace DevNet
