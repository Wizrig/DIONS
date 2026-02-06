// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 - Post-Quantum Cryptography Module
//
// Provides quantum-resistant cryptographic primitives for IoT devices
// and humanoid robots using NIST-approved algorithms.

#ifndef DIONS2_PQC_H
#define DIONS2_PQC_H

#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <array>
#include <map>

namespace dions2 {
namespace pqc {

// NIST PQC Algorithm identifiers
enum class Algorithm {
    // Signature algorithms
    DILITHIUM2,      // NIST Level 2 (128-bit security)
    DILITHIUM3,      // NIST Level 3 (192-bit security)
    DILITHIUM5,      // NIST Level 5 (256-bit security)
    FALCON512,       // NIST Level 1 - compact signatures (IoT optimized)
    FALCON1024,      // NIST Level 5 - compact signatures
    SPHINCS_SHA2_128F, // Hash-based (stateless) backup

    // Key Encapsulation Mechanisms
    KYBER512,        // NIST Level 1 (IoT optimized)
    KYBER768,        // NIST Level 3 (recommended)
    KYBER1024        // NIST Level 5 (paranoid)
};

// Device profiles for automatic algorithm selection
enum class DeviceProfile {
    IOT_MINIMAL,     // Kyber512 + Falcon512 (smallest footprint)
    IOT_STANDARD,    // Kyber768 + Falcon512
    ROBOT_STANDARD,  // Kyber768 + Dilithium3
    ROBOT_PREMIUM,   // Kyber1024 + Dilithium5
    SERVER,          // Kyber1024 + Dilithium5
    PARANOID         // All algorithms at max strength
};

// Algorithm parameters
struct AlgorithmParams {
    size_t public_key_size;
    size_t secret_key_size;
    size_t signature_size;    // For signature algorithms
    size_t ciphertext_size;   // For KEMs
    size_t shared_secret_size;
    uint8_t nist_level;
    const char* name;
};

// Get parameters for an algorithm
const AlgorithmParams& GetAlgorithmParams(Algorithm algo);

// Key pair structure
struct KeyPair {
    std::vector<uint8_t> public_key;
    std::vector<uint8_t> secret_key;
    Algorithm algorithm;

    bool IsValid() const { return !public_key.empty() && !secret_key.empty(); }
    void Clear() {
        std::fill(secret_key.begin(), secret_key.end(), 0);
        public_key.clear();
        secret_key.clear();
    }
};

// Hybrid key pair (classical + PQC)
struct HybridKeyPair {
    KeyPair pqc_keypair;
    std::vector<uint8_t> classical_public;   // Ed25519 or ECDSA
    std::vector<uint8_t> classical_secret;

    void Clear() {
        pqc_keypair.Clear();
        std::fill(classical_secret.begin(), classical_secret.end(), 0);
        classical_public.clear();
        classical_secret.clear();
    }
};

// Signature structure
struct Signature {
    std::vector<uint8_t> data;
    Algorithm algorithm;

    bool IsValid() const { return !data.empty(); }
};

// Hybrid signature (both classical and PQC)
struct HybridSignature {
    Signature pqc_signature;
    std::vector<uint8_t> classical_signature;  // Ed25519 or ECDSA

    bool IsValid() const { return pqc_signature.IsValid() && !classical_signature.empty(); }
};

// KEM ciphertext and shared secret
struct KEMResult {
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> shared_secret;
    Algorithm algorithm;
};

// Abstract signature provider interface
class ISignatureProvider {
public:
    virtual ~ISignatureProvider() = default;
    virtual Algorithm GetAlgorithm() const = 0;
    virtual KeyPair GenerateKeyPair() = 0;
    virtual Signature Sign(const std::vector<uint8_t>& message,
                          const std::vector<uint8_t>& secret_key) = 0;
    virtual bool Verify(const std::vector<uint8_t>& message,
                       const Signature& signature,
                       const std::vector<uint8_t>& public_key) = 0;
};

// Abstract KEM provider interface
class IKEMProvider {
public:
    virtual ~IKEMProvider() = default;
    virtual Algorithm GetAlgorithm() const = 0;
    virtual KeyPair GenerateKeyPair() = 0;
    virtual KEMResult Encapsulate(const std::vector<uint8_t>& public_key) = 0;
    virtual std::vector<uint8_t> Decapsulate(const std::vector<uint8_t>& ciphertext,
                                             const std::vector<uint8_t>& secret_key) = 0;
};

// Dilithium signature provider
class DilithiumProvider : public ISignatureProvider {
public:
    explicit DilithiumProvider(Algorithm algo = Algorithm::DILITHIUM3);
    Algorithm GetAlgorithm() const override { return algo_; }
    KeyPair GenerateKeyPair() override;
    Signature Sign(const std::vector<uint8_t>& message,
                  const std::vector<uint8_t>& secret_key) override;
    bool Verify(const std::vector<uint8_t>& message,
               const Signature& signature,
               const std::vector<uint8_t>& public_key) override;
private:
    Algorithm algo_;
};

// Falcon signature provider (IoT optimized - smaller signatures)
class FalconProvider : public ISignatureProvider {
public:
    explicit FalconProvider(Algorithm algo = Algorithm::FALCON512);
    Algorithm GetAlgorithm() const override { return algo_; }
    KeyPair GenerateKeyPair() override;
    Signature Sign(const std::vector<uint8_t>& message,
                  const std::vector<uint8_t>& secret_key) override;
    bool Verify(const std::vector<uint8_t>& message,
               const Signature& signature,
               const std::vector<uint8_t>& public_key) override;
private:
    Algorithm algo_;
};

// Kyber KEM provider
class KyberProvider : public IKEMProvider {
public:
    explicit KyberProvider(Algorithm algo = Algorithm::KYBER768);
    Algorithm GetAlgorithm() const override { return algo_; }
    KeyPair GenerateKeyPair() override;
    KEMResult Encapsulate(const std::vector<uint8_t>& public_key) override;
    std::vector<uint8_t> Decapsulate(const std::vector<uint8_t>& ciphertext,
                                     const std::vector<uint8_t>& secret_key) override;
private:
    Algorithm algo_;
};

// Hybrid signature provider (PQC + classical for transition period)
class HybridSignatureProvider {
public:
    explicit HybridSignatureProvider(Algorithm pqc_algo = Algorithm::DILITHIUM3);
    HybridKeyPair GenerateKeyPair();
    HybridSignature Sign(const std::vector<uint8_t>& message,
                        const HybridKeyPair& keypair);
    bool Verify(const std::vector<uint8_t>& message,
               const HybridSignature& signature,
               const HybridKeyPair& keypair);
    // Verify just PQC (post-migration)
    bool VerifyPQCOnly(const std::vector<uint8_t>& message,
                      const Signature& signature,
                      const std::vector<uint8_t>& public_key);
private:
    std::unique_ptr<ISignatureProvider> pqc_provider_;
};

// Factory functions
std::unique_ptr<ISignatureProvider> CreateSignatureProvider(Algorithm algo);
std::unique_ptr<IKEMProvider> CreateKEMProvider(Algorithm algo);

// Utility: Get recommended algorithms for device profile
struct RecommendedAlgorithms {
    Algorithm signature;
    Algorithm kem;
};
RecommendedAlgorithms GetRecommendedAlgorithms(DeviceProfile profile);

// Utility: Check if algorithm is signature type
bool IsSignatureAlgorithm(Algorithm algo);
bool IsKEMAlgorithm(Algorithm algo);

// Algorithm name to string
std::string AlgorithmToString(Algorithm algo);

} // namespace pqc
} // namespace dions2

#endif // DIONS2_PQC_H
