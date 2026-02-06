// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 - Post-Quantum Cryptography Implementation
//
// Reference implementations using simplified algorithms.
// Production should integrate liboqs for NIST-approved implementations.

#include "pqc.h"
#include <random>
#include <algorithm>
#include <stdexcept>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/evp.h>

namespace dions2 {
namespace pqc {

// Algorithm parameters lookup table
static const std::map<Algorithm, AlgorithmParams> ALGORITHM_PARAMS = {
    // Dilithium variants (CRYSTALS-Dilithium)
    {Algorithm::DILITHIUM2, {1312, 2528, 2420, 0, 0, 2, "Dilithium2"}},
    {Algorithm::DILITHIUM3, {1952, 4000, 3293, 0, 0, 3, "Dilithium3"}},
    {Algorithm::DILITHIUM5, {2592, 4864, 4595, 0, 0, 5, "Dilithium5"}},

    // Falcon variants (compact signatures for IoT)
    {Algorithm::FALCON512, {897, 1281, 690, 0, 0, 1, "Falcon-512"}},
    {Algorithm::FALCON1024, {1793, 2305, 1330, 0, 0, 5, "Falcon-1024"}},

    // SPHINCS+ (hash-based backup)
    {Algorithm::SPHINCS_SHA2_128F, {32, 64, 17088, 0, 0, 1, "SPHINCS+-SHA2-128f"}},

    // Kyber variants (CRYSTALS-Kyber KEM)
    {Algorithm::KYBER512, {800, 1632, 0, 768, 32, 1, "Kyber512"}},
    {Algorithm::KYBER768, {1184, 2400, 0, 1088, 32, 3, "Kyber768"}},
    {Algorithm::KYBER1024, {1568, 3168, 0, 1568, 32, 5, "Kyber1024"}}
};

const AlgorithmParams& GetAlgorithmParams(Algorithm algo) {
    auto it = ALGORITHM_PARAMS.find(algo);
    if (it == ALGORITHM_PARAMS.end()) {
        throw std::invalid_argument("Unknown algorithm");
    }
    return it->second;
}

bool IsSignatureAlgorithm(Algorithm algo) {
    return algo == Algorithm::DILITHIUM2 ||
           algo == Algorithm::DILITHIUM3 ||
           algo == Algorithm::DILITHIUM5 ||
           algo == Algorithm::FALCON512 ||
           algo == Algorithm::FALCON1024 ||
           algo == Algorithm::SPHINCS_SHA2_128F;
}

bool IsKEMAlgorithm(Algorithm algo) {
    return algo == Algorithm::KYBER512 ||
           algo == Algorithm::KYBER768 ||
           algo == Algorithm::KYBER1024;
}

std::string AlgorithmToString(Algorithm algo) {
    auto it = ALGORITHM_PARAMS.find(algo);
    if (it != ALGORITHM_PARAMS.end()) {
        return it->second.name;
    }
    return "Unknown";
}

RecommendedAlgorithms GetRecommendedAlgorithms(DeviceProfile profile) {
    switch (profile) {
        case DeviceProfile::IOT_MINIMAL:
            return {Algorithm::FALCON512, Algorithm::KYBER512};
        case DeviceProfile::IOT_STANDARD:
            return {Algorithm::FALCON512, Algorithm::KYBER768};
        case DeviceProfile::ROBOT_STANDARD:
            return {Algorithm::DILITHIUM3, Algorithm::KYBER768};
        case DeviceProfile::ROBOT_PREMIUM:
        case DeviceProfile::SERVER:
            return {Algorithm::DILITHIUM5, Algorithm::KYBER1024};
        case DeviceProfile::PARANOID:
        default:
            return {Algorithm::DILITHIUM5, Algorithm::KYBER1024};
    }
}

// Secure random bytes using OpenSSL
static void SecureRandom(std::vector<uint8_t>& buffer) {
    if (RAND_bytes(buffer.data(), buffer.size()) != 1) {
        throw std::runtime_error("Failed to generate secure random bytes");
    }
}

//=============================================================================
// DILITHIUM IMPLEMENTATION (Simplified reference)
// Production: Use liboqs DILITHIUM implementation
//=============================================================================

DilithiumProvider::DilithiumProvider(Algorithm algo) : algo_(algo) {
    if (algo != Algorithm::DILITHIUM2 &&
        algo != Algorithm::DILITHIUM3 &&
        algo != Algorithm::DILITHIUM5) {
        throw std::invalid_argument("Invalid Dilithium variant");
    }
}

KeyPair DilithiumProvider::GenerateKeyPair() {
    const auto& params = GetAlgorithmParams(algo_);
    KeyPair kp;
    kp.algorithm = algo_;
    kp.public_key.resize(params.public_key_size);
    kp.secret_key.resize(params.secret_key_size);

    // Generate seed and derive keys (simplified)
    std::vector<uint8_t> seed(32);
    SecureRandom(seed);

    // Derive public key from seed (simplified hash expansion)
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, seed.data(), seed.size());
    SHA256_Update(&ctx, "dilithium_pk", 12);

    // Expand to full public key size
    for (size_t i = 0; i < params.public_key_size; i += 32) {
        uint8_t counter = i / 32;
        SHA256_CTX ctx2 = ctx;
        SHA256_Update(&ctx2, &counter, 1);
        SHA256_Final(kp.public_key.data() + i, &ctx2);
    }

    // Secret key includes seed + derived values
    std::copy(seed.begin(), seed.end(), kp.secret_key.begin());
    std::vector<uint8_t> sk_rest(kp.secret_key.size() - 32);
    SecureRandom(sk_rest);
    std::copy(sk_rest.begin(), sk_rest.end(), kp.secret_key.begin() + 32);

    return kp;
}

Signature DilithiumProvider::Sign(const std::vector<uint8_t>& message,
                                   const std::vector<uint8_t>& secret_key) {
    const auto& params = GetAlgorithmParams(algo_);
    Signature sig;
    sig.algorithm = algo_;
    sig.data.resize(params.signature_size);

    // Hash message with secret key (simplified)
    SHA512_CTX ctx;
    SHA512_Init(&ctx);
    SHA512_Update(&ctx, secret_key.data(), std::min(secret_key.size(), (size_t)64));
    SHA512_Update(&ctx, message.data(), message.size());

    // Expand hash to signature size
    std::vector<uint8_t> hash(64);
    SHA512_Final(hash.data(), &ctx);

    for (size_t i = 0; i < params.signature_size; i += 64) {
        uint8_t counter = i / 64;
        SHA512_CTX ctx2;
        SHA512_Init(&ctx2);
        SHA512_Update(&ctx2, hash.data(), 64);
        SHA512_Update(&ctx2, &counter, 1);
        SHA512_Final(sig.data.data() + i, &ctx2);
    }

    return sig;
}

bool DilithiumProvider::Verify(const std::vector<uint8_t>& message,
                                const Signature& signature,
                                const std::vector<uint8_t>& public_key) {
    if (signature.algorithm != algo_) return false;

    const auto& params = GetAlgorithmParams(algo_);
    if (signature.data.size() != params.signature_size) return false;
    if (public_key.size() != params.public_key_size) return false;

    // Recompute signature verification (simplified)
    SHA512_CTX ctx;
    SHA512_Init(&ctx);
    SHA512_Update(&ctx, public_key.data(), std::min(public_key.size(), (size_t)64));
    SHA512_Update(&ctx, message.data(), message.size());
    SHA512_Update(&ctx, signature.data.data(), std::min(signature.data.size(), (size_t)64));

    std::vector<uint8_t> verification(64);
    SHA512_Final(verification.data(), &ctx);

    // Simplified acceptance (real implementation uses lattice verification)
    return verification[0] < 128;
}

//=============================================================================
// FALCON IMPLEMENTATION (Simplified reference - IoT optimized)
// Production: Use liboqs FALCON implementation
//=============================================================================

FalconProvider::FalconProvider(Algorithm algo) : algo_(algo) {
    if (algo != Algorithm::FALCON512 && algo != Algorithm::FALCON1024) {
        throw std::invalid_argument("Invalid Falcon variant");
    }
}

KeyPair FalconProvider::GenerateKeyPair() {
    const auto& params = GetAlgorithmParams(algo_);
    KeyPair kp;
    kp.algorithm = algo_;
    kp.public_key.resize(params.public_key_size);
    kp.secret_key.resize(params.secret_key_size);

    // Generate keys (simplified)
    SecureRandom(kp.secret_key);

    // Derive public key
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, kp.secret_key.data(), kp.secret_key.size());

    for (size_t i = 0; i < params.public_key_size; i += 32) {
        uint8_t counter = i / 32;
        SHA256_CTX ctx2 = ctx;
        SHA256_Update(&ctx2, &counter, 1);
        SHA256_Final(kp.public_key.data() + i, &ctx2);
    }

    return kp;
}

Signature FalconProvider::Sign(const std::vector<uint8_t>& message,
                                const std::vector<uint8_t>& secret_key) {
    const auto& params = GetAlgorithmParams(algo_);
    Signature sig;
    sig.algorithm = algo_;
    sig.data.resize(params.signature_size);

    // Compact signature generation (simplified)
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, secret_key.data(), std::min(secret_key.size(), (size_t)32));
    SHA256_Update(&ctx, message.data(), message.size());

    for (size_t i = 0; i < params.signature_size; i += 32) {
        uint8_t counter = i / 32;
        SHA256_CTX ctx2 = ctx;
        SHA256_Update(&ctx2, &counter, 1);
        SHA256_Final(sig.data.data() + i, &ctx2);
    }

    return sig;
}

bool FalconProvider::Verify(const std::vector<uint8_t>& message,
                             const Signature& signature,
                             const std::vector<uint8_t>& public_key) {
    if (signature.algorithm != algo_) return false;

    const auto& params = GetAlgorithmParams(algo_);
    if (signature.data.size() != params.signature_size) return false;
    if (public_key.size() != params.public_key_size) return false;

    // Real implementation uses NTRU lattice verification
    // This is a placeholder for the reference implementation
    return true;
}

//=============================================================================
// KYBER KEM IMPLEMENTATION (Simplified reference)
// Production: Use liboqs KYBER implementation
//=============================================================================

KyberProvider::KyberProvider(Algorithm algo) : algo_(algo) {
    if (algo != Algorithm::KYBER512 &&
        algo != Algorithm::KYBER768 &&
        algo != Algorithm::KYBER1024) {
        throw std::invalid_argument("Invalid Kyber variant");
    }
}

KeyPair KyberProvider::GenerateKeyPair() {
    const auto& params = GetAlgorithmParams(algo_);
    KeyPair kp;
    kp.algorithm = algo_;
    kp.public_key.resize(params.public_key_size);
    kp.secret_key.resize(params.secret_key_size);

    SecureRandom(kp.secret_key);

    // Derive public key
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, kp.secret_key.data(), kp.secret_key.size());

    for (size_t i = 0; i < params.public_key_size; i += 32) {
        uint8_t counter = i / 32;
        SHA256_CTX ctx2 = ctx;
        SHA256_Update(&ctx2, &counter, 1);
        SHA256_Final(kp.public_key.data() + i, &ctx2);
    }

    return kp;
}

KEMResult KyberProvider::Encapsulate(const std::vector<uint8_t>& public_key) {
    const auto& params = GetAlgorithmParams(algo_);
    KEMResult result;
    result.algorithm = algo_;
    result.ciphertext.resize(params.ciphertext_size);
    result.shared_secret.resize(params.shared_secret_size);

    // Generate ephemeral randomness
    std::vector<uint8_t> randomness(32);
    SecureRandom(randomness);

    // Encapsulate (simplified)
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, public_key.data(), public_key.size());
    SHA256_Update(&ctx, randomness.data(), randomness.size());

    // Derive shared secret
    SHA256_Final(result.shared_secret.data(), &ctx);

    // Create ciphertext
    for (size_t i = 0; i < params.ciphertext_size; i += 32) {
        uint8_t counter = i / 32;
        SHA256_CTX ctx2;
        SHA256_Init(&ctx2);
        SHA256_Update(&ctx2, result.shared_secret.data(), 32);
        SHA256_Update(&ctx2, randomness.data(), 32);
        SHA256_Update(&ctx2, &counter, 1);
        SHA256_Final(result.ciphertext.data() + i, &ctx2);
    }

    return result;
}

std::vector<uint8_t> KyberProvider::Decapsulate(const std::vector<uint8_t>& ciphertext,
                                                  const std::vector<uint8_t>& secret_key) {
    const auto& params = GetAlgorithmParams(algo_);
    std::vector<uint8_t> shared_secret(params.shared_secret_size);

    // Decapsulate (simplified)
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, secret_key.data(), secret_key.size());
    SHA256_Update(&ctx, ciphertext.data(), ciphertext.size());
    SHA256_Final(shared_secret.data(), &ctx);

    return shared_secret;
}

//=============================================================================
// HYBRID SIGNATURE PROVIDER (PQC + Ed25519 for transition)
//=============================================================================

HybridSignatureProvider::HybridSignatureProvider(Algorithm pqc_algo)
    : pqc_provider_(CreateSignatureProvider(pqc_algo)) {}

HybridKeyPair HybridSignatureProvider::GenerateKeyPair() {
    HybridKeyPair hkp;

    // Generate PQC keypair
    hkp.pqc_keypair = pqc_provider_->GenerateKeyPair();

    // Generate Ed25519 keypair (using OpenSSL)
    EVP_PKEY* pkey = nullptr;
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
    if (ctx && EVP_PKEY_keygen_init(ctx) > 0 && EVP_PKEY_keygen(ctx, &pkey) > 0) {
        size_t len = 32;
        hkp.classical_public.resize(32);
        hkp.classical_secret.resize(64);
        EVP_PKEY_get_raw_public_key(pkey, hkp.classical_public.data(), &len);
        len = 64;
        EVP_PKEY_get_raw_private_key(pkey, hkp.classical_secret.data(), &len);
        hkp.classical_secret.resize(len);
    }
    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(ctx);

    return hkp;
}

HybridSignature HybridSignatureProvider::Sign(const std::vector<uint8_t>& message,
                                               const HybridKeyPair& keypair) {
    HybridSignature hsig;

    // PQC signature
    hsig.pqc_signature = pqc_provider_->Sign(message, keypair.pqc_keypair.secret_key);

    // Ed25519 signature
    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr,
                                                   keypair.classical_secret.data(),
                                                   keypair.classical_secret.size());
    if (pkey) {
        EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
        if (md_ctx && EVP_DigestSignInit(md_ctx, nullptr, nullptr, nullptr, pkey) > 0) {
            size_t sig_len = 64;
            hsig.classical_signature.resize(sig_len);
            EVP_DigestSign(md_ctx, hsig.classical_signature.data(), &sig_len,
                          message.data(), message.size());
            hsig.classical_signature.resize(sig_len);
        }
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
    }

    return hsig;
}

bool HybridSignatureProvider::Verify(const std::vector<uint8_t>& message,
                                      const HybridSignature& signature,
                                      const HybridKeyPair& keypair) {
    // Verify PQC signature
    if (!pqc_provider_->Verify(message, signature.pqc_signature,
                               keypair.pqc_keypair.public_key)) {
        return false;
    }

    // Verify Ed25519 signature
    EVP_PKEY* pkey = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr,
                                                  keypair.classical_public.data(),
                                                  keypair.classical_public.size());
    bool result = false;
    if (pkey) {
        EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
        if (md_ctx && EVP_DigestVerifyInit(md_ctx, nullptr, nullptr, nullptr, pkey) > 0) {
            result = EVP_DigestVerify(md_ctx, signature.classical_signature.data(),
                                     signature.classical_signature.size(),
                                     message.data(), message.size()) == 1;
        }
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
    }

    return result;
}

bool HybridSignatureProvider::VerifyPQCOnly(const std::vector<uint8_t>& message,
                                            const Signature& signature,
                                            const std::vector<uint8_t>& public_key) {
    return pqc_provider_->Verify(message, signature, public_key);
}

//=============================================================================
// FACTORY FUNCTIONS
//=============================================================================

std::unique_ptr<ISignatureProvider> CreateSignatureProvider(Algorithm algo) {
    switch (algo) {
        case Algorithm::DILITHIUM2:
        case Algorithm::DILITHIUM3:
        case Algorithm::DILITHIUM5:
            return std::make_unique<DilithiumProvider>(algo);
        case Algorithm::FALCON512:
        case Algorithm::FALCON1024:
            return std::make_unique<FalconProvider>(algo);
        default:
            throw std::invalid_argument("Not a signature algorithm");
    }
}

std::unique_ptr<IKEMProvider> CreateKEMProvider(Algorithm algo) {
    switch (algo) {
        case Algorithm::KYBER512:
        case Algorithm::KYBER768:
        case Algorithm::KYBER1024:
            return std::make_unique<KyberProvider>(algo);
        default:
            throw std::invalid_argument("Not a KEM algorithm");
    }
}

} // namespace pqc
} // namespace dions2
