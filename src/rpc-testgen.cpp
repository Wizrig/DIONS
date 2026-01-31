// DIONS 2.0 Test Block Generation
// TESTING ONLY - Minimal PoW for isolated testnet coin generation
// Copyright (c) 2026 DIONS Development

#include "main.h"
#include "wallet.h"
#include "bitcoinrpc.h"
#include "init.h"
#include "miner.h"

using namespace json_spirit;
using namespace std;

extern CBigNum bnProofOfWorkLimitTestNet;

// testgenerate <nblocks> - Generate blocks on testnet for DIONS testing
// ONLY works on testnet, uses minimal PoW (unsafe for production)
Value testgenerate(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "testgenerate <nblocks>\n"
            "Generate blocks instantly on testnet for DIONS testing.\n"
            "TESTNET ONLY - Will fail on mainnet.\n"
            "Returns array of block hashes generated.");

    if (!fTestNet)
        throw runtime_error("testgenerate only works on testnet");

    int nBlocks = params[0].get_int();

    if (nBlocks < 1 || nBlocks > 1000)
        throw runtime_error("nBlocks must be between 1 and 1000");

    if (pwalletMain == NULL)
        throw runtime_error("Wallet not loaded");

    Array blockHashes;

    CBigNum bnTarget;
    bnTarget.SetCompact(bnProofOfWorkLimitTestNet.GetCompact());

    for (int i = 0; i < nBlocks; i++)
    {
        // Get new address for coinbase
        CPubKey newKey;
        if (!pwalletMain->GetKeyFromPool(newKey, false))
            throw runtime_error("Failed to get key from pool");

        CScript scriptPubKey;
        scriptPubKey.SetDestination(newKey.GetID());

        // Create coinbase transaction (PoW block)
        CBlock *pblock = CreateNewBlock(pwalletMain, false);

        if (!pblock)
            throw runtime_error("Failed to create block");

        pblock->vtx[0].vout[0].scriptPubKey = scriptPubKey;
        pblock->hashMerkleRoot = pblock->BuildMerkleTree();

        // Minimal PoW solve (testnet only, very low difficulty)
        pblock->nNonce = 0;

        while (pblock->nNonce < 0x7fffffff)
        {
            uint256 hash = pblock->GetHash();

            if (hash <= bnTarget.getuint256())
            {
                // Found valid block
                if (!ProcessBlock(NULL, pblock))
                {
                    delete pblock;
                    throw runtime_error("ProcessBlock failed");
                }

                blockHashes.push_back(hash.GetHex());
                break;
            }

            pblock->nNonce++;

            if (pblock->nNonce == 0)
            {
                // Tried all nonces, increment time
                pblock->nTime++;
                pblock->nNonce = 0;
                pblock->hashMerkleRoot = pblock->BuildMerkleTree();
            }
        }

        delete pblock;
    }

    return blockHashes;
}
