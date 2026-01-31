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
        printf("Devnet: Faucet privkey: cU3HMLC5rFV83Kq3pCTzLgxTvP86qo2uo8b7HvTfmHDEy6qinGDp\n");
        printf("Devnet: Faucet address: mqKqfUYTxDvmfHB3Bd3JBt8NZjVJi1Loom (1M IOC from genesis)\n");
        printf("Devnet: Import faucet key: iocoind -devnet importprivkey cU3HMLC5rFV83Kq3pCTzLgxTvP86qo2uo8b7HvTfmHDEy6qinGDp\n");
        printf("Devnet: Then use 'devfaucet' and 'devstake' for testing\n");
    }
}

bool IsActive()
{
    return fDevNet;
}

} // namespace DevNet
