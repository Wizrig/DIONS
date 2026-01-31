// DIONS 2.0 Test Mining Mode
// TESTING ONLY - Provides instant block generation for automated testing
// This file must NEVER be included in production builds

#ifndef DIONS_TEST_MINING_H
#define DIONS_TEST_MINING_H

#ifdef DIONS_TEST_MODE

#include "main.h"
#include "wallet.h"

// Test mode: Allow mining with minimal difficulty
inline bool TestMode_AllowInstantMining() {
    return true;
}

// Test mode: Generate block immediately
inline bool TestMode_GenerateBlock(CWallet* pwallet) {
    if (!pwallet)
        return false;

    CReserveKey reservekey(pwallet);

    // Create coinbase transaction
    CTransaction txNew;
    txNew.vin.resize(1);
    txNew.vin[0].prevout.SetNull();
    txNew.vout.resize(1);

    CPubKey pubkey;
    if (!reservekey.GetReservedKey(pubkey))
        return false;

    txNew.vout[0].scriptPubKey.SetDestination(pubkey.GetID());
    txNew.vout[0].nValue = GetProofOfWorkReward(pindexBest->nHeight + 1, 0);

    // Create block
    CBlock block;
    block.vtx.push_back(txNew);
    block.hashPrevBlock = pindexBest->GetBlockHash();
    block.nTime = max(pindexBest->GetMedianTimePast() + 1, GetAdjustedTime());
    block.nBits = GetNextTargetRequired(pindexBest, false);
    block.nNonce = 0;

    // Minimal PoW for test mode
    while (!CheckProofOfWork(block.GetHash(), block.nBits)) {
        block.nNonce++;
        if (block.nNonce > 1000) {
            // Reduce difficulty for test mode
            block.nBits = 0x207fffff;
        }
    }

    // Process block
    CValidationState state;
    if (!ProcessBlock(state, NULL, &block))
        return false;

    reservekey.KeepKey();
    return true;
}

#endif // DIONS_TEST_MODE
#endif // DIONS_TEST_MINING_H
