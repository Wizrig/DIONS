// DIONS 2.0 Devnet RPC Commands
// Automated PoS testing support
// Copyright (c) 2026 DIONS Development

#include "main.h"
#include "wallet.h"
#include "walletdb.h"
#include "init.h"
#include "bitcoinrpc.h"
#include "devnet.h"
#include "miner.h"
#include "base58.h"

using namespace json_spirit;
using namespace std;

// devfaucet <address> <amount>
// Send coins from devnet faucet to address (devnet only)
Value devfaucet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "devfaucet <address> <amount>\n"
            "Send coins from devnet faucet to address.\n"
            "DEVNET ONLY - Will fail if not in devnet mode.");

    if (!DevNet::IsActive())
        throw runtime_error("devfaucet only works in devnet mode (use -devnet)");

    if (pwalletMain == NULL)
        throw runtime_error("Wallet not loaded");

    // Parse address
    cba address(params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid IOCoin address");

    // Parse amount
    int64_t nAmount = AmountFromValue(params[1]);
    if (nAmount < nMinimumInputValue)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Amount too small");

    // Send from faucet key
    __wx__Tx wtx;
    string strError = pwalletMain->SendMoneyToDestination(address.Get(), nAmount, wtx, false, "");

    if (strError != "")
        throw JSONRPCError(RPC_WALLET_ERROR, strError);

    return wtx.GetHash().GetHex();
}

// devstake <nblocks>
// Advance blockchain by staking N blocks (devnet only)
// Uses existing PoS staking mechanism with time advancement
Value devstake(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "devstake <nblocks>\n"
            "Advance blockchain by staking N blocks in devnet.\n"
            "DEVNET ONLY - Uses PoS staking with time advancement.\n"
            "Returns array of block hashes generated.");

    if (!DevNet::IsActive())
        throw runtime_error("devstake only works in devnet mode (use -devnet)");

    int nBlocks = params[0].get_int();

    if (nBlocks < 1 || nBlocks > 1000)
        throw runtime_error("nBlocks must be between 1 and 1000");

    if (pwalletMain == NULL)
        throw runtime_error("Wallet not loaded");

    Array blockHashes;

    // Reserve key for coinbase
    CReserveKey reservekey(pwalletMain);

    for (int i = 0; i < nBlocks; i++)
    {
        // Try to create stake block
        int64_t nFees = 0;
        unique_ptr<CBlock> pblock(CreateNewBlock(pwalletMain, true, &nFees));

        if (!pblock.get())
        {
            // If can't stake, advance time and try again
            SetMockTime(GetTime() + 60); // Advance 1 minute
            pblock.reset(CreateNewBlock(pwalletMain, true, &nFees));

            if (!pblock.get())
            {
                // Still can't stake, might need more mature coins
                throw runtime_error(strprintf(
                    "Failed to create stake block %d (may need more mature coins or stake weight)", i + 1));
            }
        }

        // Sign the stake block
        if (!pblock->SignBlock(*pwalletMain, nFees))
        {
            throw runtime_error(strprintf("Failed to sign block %d", i + 1));
        }

        // Process the block
        if (!ProcessBlock(NULL, pblock.get()))
        {
            throw runtime_error(strprintf("ProcessBlock failed for block %d", i + 1));
        }

        blockHashes.push_back(pblock->GetHash().GetHex());

        // Advance time for next block
        SetMockTime(GetTime() + 61); // Slightly more than min age
    }

    return blockHashes;
}

// devtime <seconds>
// Advance mock time by N seconds (devnet only)
Value devtime(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "devtime <seconds>\n"
            "Advance mock time by N seconds in devnet.\n"
            "DEVNET ONLY - Used to age stakes/coins.");

    if (!DevNet::IsActive())
        throw runtime_error("devtime only works in devnet mode (use -devnet)");

    int64_t nSeconds = params[0].get_int64();

    if (nSeconds < 0 || nSeconds > 86400 * 365)
        throw runtime_error("seconds must be between 0 and 31536000 (1 year)");

    SetMockTime(GetTime() + nSeconds);

    Object result;
    result.push_back(Pair("mocktime", GetTime()));
    result.push_back(Pair("advanced", nSeconds));

    return result;
}
