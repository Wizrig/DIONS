// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 - liboqs Integration for Production PQC
//
// This header provides the bridge between our PQC interface and liboqs.
// liboqs provides NIST-approved implementations of post-quantum algorithms.

#ifndef DIONS2_PQC_LIBOQS_H
#define DIONS2_PQC_LIBOQS_H

#include "pqc.h"

// Check if liboqs is available
#ifdef HAVE_LIBOQS
#include <oqs/oqs.h>
#endif

namespace dions2 {
namespace pqc {

// Check if liboqs is available at runtime
bool IsLiboqsAvailable();

// Get liboqs version string
std::string GetLiboqsVersion();

#ifdef HAVE_LIBOQS

//=============================================================================
// liboqs-backed Signature Providers
//=============================================================================

// ML-DSA Provider (FIPS 204 standardized Dilithium replacement)
// Note: liboqs 0.15.0 removed Dilithium, use ML-DSA instead
class MLDSAProvider : public ISignatureProvider {
public:
    // Maps to ML-DSA-44 (L2), ML-DSA-65 (L3), ML-DSA-87 (L5)
    explicit MLDSAProvider(Algorithm algo = Algorithm::DILITHIUM3);
    ~MLDSAProvider();

    Algorithm GetAlgorithm() const override { return algo_; }
    KeyPair GenerateKeyPair() override;
    Signature Sign(const std::vector<uint8_t>& message,
                  const std::vector<uint8_t>& secret_key) override;
    bool Verify(const std::vector<uint8_t>& message,
               const Signature& signature,
               const std::vector<uint8_t>& public_key) override;

private:
    Algorithm algo_;
    OQS_SIG* sig_;
    const char* GetOQSAlgName() const;
};

// Falcon Provider using liboqs
class FalconLiboqsProvider : public ISignatureProvider {
public:
    explicit FalconLiboqsProvider(Algorithm algo = Algorithm::FALCON512);
    ~FalconLiboqsProvider();

    Algorithm GetAlgorithm() const override { return algo_; }
    KeyPair GenerateKeyPair() override;
    Signature Sign(const std::vector<uint8_t>& message,
                  const std::vector<uint8_t>& secret_key) override;
    bool Verify(const std::vector<uint8_t>& message,
               const Signature& signature,
               const std::vector<uint8_t>& public_key) override;

private:
    Algorithm algo_;
    OQS_SIG* sig_;
};

//=============================================================================
// liboqs-backed KEM Providers
//=============================================================================

// ML-KEM Provider (FIPS 203 standardized Kyber replacement)
class MLKEMProvider : public IKEMProvider {
public:
    // Maps to ML-KEM-512, ML-KEM-768, ML-KEM-1024
    explicit MLKEMProvider(Algorithm algo = Algorithm::KYBER768);
    ~MLKEMProvider();

    Algorithm GetAlgorithm() const override { return algo_; }
    KeyPair GenerateKeyPair() override;
    KEMResult Encapsulate(const std::vector<uint8_t>& public_key) override;
    std::vector<uint8_t> Decapsulate(const std::vector<uint8_t>& ciphertext,
                                     const std::vector<uint8_t>& secret_key) override;

private:
    Algorithm algo_;
    OQS_KEM* kem_;
    const char* GetOQSAlgName() const;
};

// Legacy Kyber Provider (for backwards compatibility)
// Note: Kyber is being phased out, ML-KEM is the standardized version
class KyberLiboqsProvider : public IKEMProvider {
public:
    explicit KyberLiboqsProvider(Algorithm algo = Algorithm::KYBER768);
    ~KyberLiboqsProvider();

    Algorithm GetAlgorithm() const override { return algo_; }
    KeyPair GenerateKeyPair() override;
    KEMResult Encapsulate(const std::vector<uint8_t>& public_key) override;
    std::vector<uint8_t> Decapsulate(const std::vector<uint8_t>& ciphertext,
                                     const std::vector<uint8_t>& secret_key) override;

private:
    Algorithm algo_;
    OQS_KEM* kem_;
};

//=============================================================================
// Factory Functions for liboqs-backed providers
//=============================================================================

// Create liboqs-backed signature provider
std::unique_ptr<ISignatureProvider> CreateLiboqsSignatureProvider(Algorithm algo);

// Create liboqs-backed KEM provider
std::unique_ptr<IKEMProvider> CreateLiboqsKEMProvider(Algorithm algo);

// Prefer liboqs if available, fallback to reference implementation
std::unique_ptr<ISignatureProvider> CreateBestSignatureProvider(Algorithm algo);
std::unique_ptr<IKEMProvider> CreateBestKEMProvider(Algorithm algo);

#else // !HAVE_LIBOQS

// Stubs when liboqs is not available
inline bool IsLiboqsAvailable() { return false; }
inline std::string GetLiboqsVersion() { return "not available"; }

// Use reference implementations
inline std::unique_ptr<ISignatureProvider> CreateBestSignatureProvider(Algorithm algo) {
    return CreateSignatureProvider(algo);
}

inline std::unique_ptr<IKEMProvider> CreateBestKEMProvider(Algorithm algo) {
    return CreateKEMProvider(algo);
}

#endif // HAVE_LIBOQS

} // namespace pqc
} // namespace dions2

#endif // DIONS2_PQC_LIBOQS_H
