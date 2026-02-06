// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 - Hybrid Signature Implementation

#include "hybrid_sig.h"
#include "../key.h"
#include "../util.h"  // Contains Hash() function

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <stdexcept>

namespace dions2 {

//-----------------------------------------------------------------------------
// Utility functions
//-----------------------------------------------------------------------------

pqc::Algorithm GetPQCAlgorithmFromScheme(SignatureScheme scheme)
{
    switch (scheme) {
        case SignatureScheme::FALCON512:
        case SignatureScheme::HYBRID_ED25519_FALCON512:
        case SignatureScheme::HYBRID_ECDSA_FALCON512:
            return pqc::Algorithm::FALCON512;

        case SignatureScheme::FALCON1024:
            return pqc::Algorithm::FALCON1024;

        case SignatureScheme::DILITHIUM2:
            return pqc::Algorithm::DILITHIUM2;

        case SignatureScheme::DILITHIUM3:
        case SignatureScheme::HYBRID_ED25519_DILITHIUM3:
        case SignatureScheme::HYBRID_ECDSA_DILITHIUM3:
            return pqc::Algorithm::DILITHIUM3;

        case SignatureScheme::DILITHIUM5:
            return pqc::Algorithm::DILITHIUM5;

        default:
            return pqc::Algorithm::FALCON512;  // Default to compact IoT-friendly algo
    }
}

std::string SignatureSchemeToString(SignatureScheme scheme)
{
    switch (scheme) {
        case SignatureScheme::ECDSA_SECP256K1:    return "ecdsa_secp256k1";
        case SignatureScheme::ED25519:            return "ed25519";
        case SignatureScheme::FALCON512:          return "falcon512";
        case SignatureScheme::FALCON1024:         return "falcon1024";
        case SignatureScheme::DILITHIUM2:         return "dilithium2";
        case SignatureScheme::DILITHIUM3:         return "dilithium3";
        case SignatureScheme::DILITHIUM5:         return "dilithium5";
        case SignatureScheme::HYBRID_ED25519_FALCON512:  return "hybrid_ed25519_falcon512";
        case SignatureScheme::HYBRID_ED25519_DILITHIUM3: return "hybrid_ed25519_dilithium3";
        case SignatureScheme::HYBRID_ECDSA_FALCON512:    return "hybrid_ecdsa_falcon512";
        case SignatureScheme::HYBRID_ECDSA_DILITHIUM3:   return "hybrid_ecdsa_dilithium3";
        default: return "unknown";
    }
}

//-----------------------------------------------------------------------------
// DionsPublicKey implementation
//-----------------------------------------------------------------------------

std::array<uint8_t, 32> DionsPublicKey::GetFingerprint() const
{
    std::array<uint8_t, 32> fingerprint;

    // Concatenate all public key material and hash
    std::vector<uint8_t> data;
    data.push_back(static_cast<uint8_t>(scheme));
    data.insert(data.end(), classical_pk.begin(), classical_pk.end());
    data.insert(data.end(), pqc_pk.begin(), pqc_pk.end());

    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data.data(), data.size());
    SHA256_Final(fingerprint.data(), &ctx);

    return fingerprint;
}

//-----------------------------------------------------------------------------
// HybridSigner implementation
//-----------------------------------------------------------------------------

DionsSignature HybridSigner::Sign(
    const std::vector<uint8_t>& message,
    const std::vector<uint8_t>& classical_privkey,
    const pqc::KeyPair* pqc_keypair,
    SignatureMode mode,
    pqc::Algorithm pqc_algo)
{
    DionsSignature sig;

    switch (mode) {
        case SignatureMode::CLASSICAL_ONLY:
        {
            sig.scheme = SignatureScheme::ECDSA_SECP256K1;
            sig.classical_sig = SignClassical(message, classical_privkey, sig.scheme);
            break;
        }

        case SignatureMode::PQC_ONLY:
        {
            if (!pqc_keypair || !pqc_keypair->IsValid()) {
                throw std::runtime_error("PQC key pair required for PQC_ONLY mode");
            }

            // Set scheme based on PQC algorithm
            switch (pqc_algo) {
                case pqc::Algorithm::FALCON512:   sig.scheme = SignatureScheme::FALCON512; break;
                case pqc::Algorithm::FALCON1024:  sig.scheme = SignatureScheme::FALCON1024; break;
                case pqc::Algorithm::DILITHIUM2:  sig.scheme = SignatureScheme::DILITHIUM2; break;
                case pqc::Algorithm::DILITHIUM3:  sig.scheme = SignatureScheme::DILITHIUM3; break;
                case pqc::Algorithm::DILITHIUM5:  sig.scheme = SignatureScheme::DILITHIUM5; break;
                default: sig.scheme = SignatureScheme::FALCON512; break;
            }

            sig.pqc_sig = SignPQC(message, pqc_keypair->secret_key, pqc_algo);
            break;
        }

        case SignatureMode::HYBRID:
        {
            if (!pqc_keypair || !pqc_keypair->IsValid()) {
                throw std::runtime_error("PQC key pair required for HYBRID mode");
            }

            // Set hybrid scheme based on PQC algorithm
            if (pqc_algo == pqc::Algorithm::FALCON512) {
                sig.scheme = SignatureScheme::HYBRID_ECDSA_FALCON512;
            } else {
                sig.scheme = SignatureScheme::HYBRID_ECDSA_DILITHIUM3;
            }

            // Sign with both algorithms
            sig.classical_sig = SignClassical(message, classical_privkey, SignatureScheme::ECDSA_SECP256K1);
            sig.pqc_sig = SignPQC(message, pqc_keypair->secret_key, pqc_algo);
            break;
        }
    }

    return sig;
}

bool HybridSigner::Verify(
    const std::vector<uint8_t>& message,
    const DionsSignature& signature,
    const DionsPublicKey& pubkey,
    bool require_both)
{
    // Scheme must match
    if (signature.scheme != pubkey.scheme) {
        return false;
    }

    if (signature.IsClassical()) {
        // Classical-only verification
        return VerifyClassical(message, signature.classical_sig, pubkey.classical_pk, signature.scheme);
    }

    if (signature.IsPQC()) {
        // PQC-only verification
        pqc::Algorithm algo = GetPQCAlgorithmFromScheme(signature.scheme);
        return VerifyPQC(message, signature.pqc_sig, pubkey.pqc_pk, algo);
    }

    if (signature.IsHybrid()) {
        // Hybrid verification
        SignatureScheme classical_scheme = SignatureScheme::ECDSA_SECP256K1;
        if (signature.scheme == SignatureScheme::HYBRID_ED25519_FALCON512 ||
            signature.scheme == SignatureScheme::HYBRID_ED25519_DILITHIUM3) {
            classical_scheme = SignatureScheme::ED25519;
        }

        bool classical_valid = VerifyClassical(message, signature.classical_sig,
                                               pubkey.classical_pk, classical_scheme);

        pqc::Algorithm pqc_algo = GetPQCAlgorithmFromScheme(signature.scheme);
        bool pqc_valid = VerifyPQC(message, signature.pqc_sig, pubkey.pqc_pk, pqc_algo);

        if (require_both) {
            // Both must be valid (maximum security)
            return classical_valid && pqc_valid;
        } else {
            // Either valid (for transition period, accepts if quantum computer breaks classical)
            return classical_valid || pqc_valid;
        }
    }

    return false;
}

std::pair<DionsPublicKey, std::vector<uint8_t>> HybridSigner::GenerateKeyPair(
    SignatureScheme scheme,
    pqc::Algorithm pqc_algo)
{
    DionsPublicKey pubkey;
    std::vector<uint8_t> secret;
    pubkey.scheme = scheme;

    if (pubkey.IsClassical() || pubkey.IsHybrid()) {
        // Generate classical key pair (ECDSA/secp256k1)
        CKey key;
        key.MakeNewKey(true);  // Compressed

        CPubKey cpubkey = key.GetPubKey();
        std::vector<unsigned char> raw = cpubkey.Raw();
        pubkey.classical_pk.assign(raw.begin(), raw.end());

        // Store classical private key
        CPrivKey privkey = key.GetPrivKey();
        secret.insert(secret.end(), privkey.begin(), privkey.end());
    }

    if (pubkey.IsPQC() || pubkey.IsHybrid()) {
        // Generate PQC key pair
        auto pqc_provider = pqc::CreateSignatureProvider(pqc_algo);
        pqc::KeyPair pqc_kp = pqc_provider->GenerateKeyPair();

        pubkey.pqc_pk = pqc_kp.public_key;

        // Append PQC secret key (with length prefix)
        uint32_t pqc_sk_len = pqc_kp.secret_key.size();
        secret.push_back((pqc_sk_len >> 24) & 0xFF);
        secret.push_back((pqc_sk_len >> 16) & 0xFF);
        secret.push_back((pqc_sk_len >> 8) & 0xFF);
        secret.push_back(pqc_sk_len & 0xFF);
        secret.insert(secret.end(), pqc_kp.secret_key.begin(), pqc_kp.secret_key.end());

        // Clear PQC secret key from memory
        pqc_kp.Clear();
    }

    return {pubkey, secret};
}

SignatureScheme HybridSigner::GetRecommendedScheme(pqc::DeviceProfile profile)
{
    switch (profile) {
        case pqc::DeviceProfile::IOT_MINIMAL:
        case pqc::DeviceProfile::IOT_STANDARD:
            // IoT: Use Falcon512 for compact signatures
            return SignatureScheme::HYBRID_ECDSA_FALCON512;

        case pqc::DeviceProfile::ROBOT_STANDARD:
        case pqc::DeviceProfile::ROBOT_PREMIUM:
        case pqc::DeviceProfile::SERVER:
            // Robots/Servers: Use Dilithium3 for balance of security and size
            return SignatureScheme::HYBRID_ECDSA_DILITHIUM3;

        case pqc::DeviceProfile::PARANOID:
            // Maximum security: Dilithium5 (but larger signatures)
            return SignatureScheme::DILITHIUM5;

        default:
            return SignatureScheme::HYBRID_ECDSA_FALCON512;
    }
}

bool HybridSigner::IsQuantumResistant(SignatureScheme scheme)
{
    // Only classical-only schemes are not quantum resistant
    return scheme != SignatureScheme::ECDSA_SECP256K1 &&
           scheme != SignatureScheme::ED25519;
}

std::string HybridSigner::SchemeName(SignatureScheme scheme)
{
    return SignatureSchemeToString(scheme);
}

//-----------------------------------------------------------------------------
// Private helper implementations
//-----------------------------------------------------------------------------

std::vector<uint8_t> HybridSigner::SignClassical(
    const std::vector<uint8_t>& message,
    const std::vector<uint8_t>& privkey,
    SignatureScheme scheme)
{
    if (scheme == SignatureScheme::ED25519) {
        // Ed25519 signing (not implemented yet - placeholder)
        throw std::runtime_error("Ed25519 not yet implemented");
    }

    // ECDSA/secp256k1 (default)
    // Hash the message first
    uint256 hash = Hash(message.begin(), message.end());

    // Create key from private key bytes
    CKey key;
    if (!key.SetPrivKey(CPrivKey(privkey.begin(), privkey.end()))) {
        throw std::runtime_error("Invalid private key");
    }

    // Sign
    std::vector<unsigned char> vchSig;
    if (!key.Sign(hash, vchSig)) {
        throw std::runtime_error("Signing failed");
    }

    return vchSig;
}

bool HybridSigner::VerifyClassical(
    const std::vector<uint8_t>& message,
    const std::vector<uint8_t>& signature,
    const std::vector<uint8_t>& pubkey,
    SignatureScheme scheme)
{
    if (scheme == SignatureScheme::ED25519) {
        // Ed25519 verification (not implemented yet)
        return false;
    }

    // ECDSA/secp256k1
    uint256 hash = Hash(message.begin(), message.end());

    // Create CPubKey from raw bytes
    std::vector<unsigned char> vchPubKey(pubkey.begin(), pubkey.end());
    CPubKey cpubkey(vchPubKey);
    if (!cpubkey.IsValid()) {
        return false;
    }

    // Set public key in CKey for verification
    CKey key;
    if (!key.SetPubKey(cpubkey)) {
        return false;
    }

    return key.Verify(hash, signature);
}

std::vector<uint8_t> HybridSigner::SignPQC(
    const std::vector<uint8_t>& message,
    const std::vector<uint8_t>& privkey,
    pqc::Algorithm algo)
{
    auto provider = pqc::CreateSignatureProvider(algo);
    pqc::Signature sig = provider->Sign(message, privkey);
    return sig.data;
}

bool HybridSigner::VerifyPQC(
    const std::vector<uint8_t>& message,
    const std::vector<uint8_t>& signature,
    const std::vector<uint8_t>& pubkey,
    pqc::Algorithm algo)
{
    auto provider = pqc::CreateSignatureProvider(algo);

    pqc::Signature sig;
    sig.data = signature;
    sig.algorithm = algo;

    return provider->Verify(message, sig, pubkey);
}

} // namespace dions2
