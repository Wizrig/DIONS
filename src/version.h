// Copyright (c) 2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_VERSION_H
#define BITCOIN_VERSION_H

#include "clientversion.h"
#include <string>

//
// client versioning
//

static const int CLIENT_VERSION =
                           3000000 * CLIENT_VERSION_MAJOR
                         +   10000 * CLIENT_VERSION_MINOR
                         +     100 * CLIENT_VERSION_REVISION
                         +       0 * CLIENT_VERSION_BUILD;

extern const std::string CLIENT_NAME;
extern const std::string CLIENT_BUILD;
extern const std::string CLIENT_DATE;

//
// database format versioning
//
static const int DATABASE_VERSION = 70509;

//
// network protocol versioning
//

static const int PROTOCOL_VERSION    = 60023;
static const int X4_PROTOCOL_VERSION = 60021;
static const int X3_PROTOCOL_VERSION = 60020;
static const int X2_PROTOCOL_VERSION = 60019;
static const int X1_PROTOCOL_VERSION = 60017;

// intial proto version, to be increased after version/verack negotiation
static const int INIT_PROTO_VERSION = 209;

// disconnect from peers older than this proto version
// NOTE: Keeping backward compatibility - old peers still accepted
static const int MIN_PEER_PROTO_VERSION = 60022;

// nTime field added to CAddress, starting with this version;
// if possible, avoid requesting addresses nodes older than this
static const int CADDR_TIME_VERSION = 31402;

// only request blocks from nodes outside this range of versions
static const int NOBLKS_VERSION_START = 60002;
static const int NOBLKS_VERSION_END = 60006;

// BIP 0031, pong message, is enabled for all versions AFTER this one
static const int BIP0031_VERSION = 60000;

// "mempool" command, enhanced "getdata" behavior starts with this version:
static const int MEMPOOL_GD_VERSION = 60002;

//
// DIONS 2.0 activation scaffolding
//
// NOTE: These constants are placeholders for future fork activation.
// Currently set to 0 (disabled) - no behavior changes enforced yet.
//

static const int DIONS2_PROTOCOL_VERSION = 60023;

// Activation heights (currently disabled - set to 0)
// Will be updated to actual block heights when DIONS 2.0 features are ready
static const int DIONS2_ACTIVATION_HEIGHT_MAINNET = 0;
static const int DIONS2_ACTIVATION_HEIGHT_TESTNET = 0;
static const int DIONS2_ACTIVATION_HEIGHT_REGTEST = 0;

// Helper function to check if DIONS 2.0 rules are active
// Currently always returns false (no activation)
inline bool IsDions2Active(int nHeight, bool fTestNet = false, bool fRegTest = false)
{
    // Return false until activation heights are set to non-zero values
    if (fRegTest && DIONS2_ACTIVATION_HEIGHT_REGTEST > 0)
        return nHeight >= DIONS2_ACTIVATION_HEIGHT_REGTEST;
    if (fTestNet && DIONS2_ACTIVATION_HEIGHT_TESTNET > 0)
        return nHeight >= DIONS2_ACTIVATION_HEIGHT_TESTNET;
    if (DIONS2_ACTIVATION_HEIGHT_MAINNET > 0)
        return nHeight >= DIONS2_ACTIVATION_HEIGHT_MAINNET;

    return false;  // Not active on any network yet
}

#endif
