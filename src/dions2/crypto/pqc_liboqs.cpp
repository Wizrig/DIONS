// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 - liboqs Integration Implementation
//
// Production-grade PQC using NIST-approved algorithms from liboqs.

#include "pqc_liboqs.h"
#include <stdexcept>
#include <cstring>

namespace dions2 {
namespace pqc {

#ifdef HAVE_LIBOQS

//=============================================================================
// liboqs Availability Check
//=============================================================================

bool IsLiboqsAvailable() {
    return true;
}

std::string GetLiboqsVersion() {
    return OQS_VERSION_TEXT;
}

//=============================================================================
// ML-DSA Provider (Standardized Dilithium)
//=============================================================================

MLDSAProvider::MLDSAProvider(Algorithm algo) : algo_(algo), sig_(nullptr) {
    const char* alg_name = GetOQSAlgName();
    if (!alg_name) {
        throw std::invalid_argument("Invalid ML-DSA variant");
    }

    sig_ = OQS_SIG_new(alg_name);
    if (!sig_) {
        throw std::runtime_error("Failed to initialize ML-DSA algorithm");
    }
}

MLDSAProvider::~MLDSAProvider() {
    if (sig_) {
        OQS_SIG_free(sig_);
    }
}

const char* MLDSAProvider::GetOQSAlgName() const {
    switch (algo_) {
        case Algorithm::DILITHIUM2:
            return OQS_SIG_alg_ml_dsa_44;  // ML-DSA-44 replaces Dilithium2
        case Algorithm::DILITHIUM3:
            return OQS_SIG_alg_ml_dsa_65;  // ML-DSA-65 replaces Dilithium3
        case Algorithm::DILITHIUM5:
            return OQS_SIG_alg_ml_dsa_87;  // ML-DSA-87 replaces Dilithium5
        default:
            return nullptr;
    }
}

KeyPair MLDSAProvider::GenerateKeyPair() {
    KeyPair kp;
    kp.algorithm = algo_;
    kp.public_key.resize(sig_->length_public_key);
    kp.secret_key.resize(sig_->length_secret_key);

    OQS_STATUS rc = OQS_SIG_keypair(sig_,
                                    kp.public_key.data(),
                                    kp.secret_key.data());

    if (rc != OQS_SUCCESS) {
        throw std::runtime_error("ML-DSA key generation failed");
    }

    return kp;
}

Signature MLDSAProvider::Sign(const std::vector<uint8_t>& message,
                              const std::vector<uint8_t>& secret_key) {
    Signature sig;
    sig.algorithm = algo_;
    sig.data.resize(sig_->length_signature);
    size_t sig_len = sig.data.size();

    OQS_STATUS rc = OQS_SIG_sign(sig_,
                                 sig.data.data(), &sig_len,
                                 message.data(), message.size(),
                                 secret_key.data());

    if (rc != OQS_SUCCESS) {
        throw std::runtime_error("ML-DSA signing failed");
    }

    sig.data.resize(sig_len);
    return sig;
}

bool MLDSAProvider::Verify(const std::vector<uint8_t>& message,
                           const Signature& signature,
                           const std::vector<uint8_t>& public_key) {
    if (signature.algorithm != algo_) {
        return false;
    }

    OQS_STATUS rc = OQS_SIG_verify(sig_,
                                   message.data(), message.size(),
                                   signature.data.data(), signature.data.size(),
                                   public_key.data());

    return rc == OQS_SUCCESS;
}

//=============================================================================
// Falcon Provider (liboqs)
//=============================================================================

FalconLiboqsProvider::FalconLiboqsProvider(Algorithm algo) : algo_(algo), sig_(nullptr) {
    const char* alg_name = nullptr;

    switch (algo) {
        case Algorithm::FALCON512:
            alg_name = OQS_SIG_alg_falcon_512;
            break;
        case Algorithm::FALCON1024:
            alg_name = OQS_SIG_alg_falcon_1024;
            break;
        default:
            throw std::invalid_argument("Invalid Falcon variant");
    }

    sig_ = OQS_SIG_new(alg_name);
    if (!sig_) {
        throw std::runtime_error("Failed to initialize Falcon algorithm");
    }
}

FalconLiboqsProvider::~FalconLiboqsProvider() {
    if (sig_) {
        OQS_SIG_free(sig_);
    }
}

KeyPair FalconLiboqsProvider::GenerateKeyPair() {
    KeyPair kp;
    kp.algorithm = algo_;
    kp.public_key.resize(sig_->length_public_key);
    kp.secret_key.resize(sig_->length_secret_key);

    OQS_STATUS rc = OQS_SIG_keypair(sig_,
                                    kp.public_key.data(),
                                    kp.secret_key.data());

    if (rc != OQS_SUCCESS) {
        throw std::runtime_error("Falcon key generation failed");
    }

    return kp;
}

Signature FalconLiboqsProvider::Sign(const std::vector<uint8_t>& message,
                                     const std::vector<uint8_t>& secret_key) {
    Signature sig;
    sig.algorithm = algo_;
    sig.data.resize(sig_->length_signature);
    size_t sig_len = sig.data.size();

    OQS_STATUS rc = OQS_SIG_sign(sig_,
                                 sig.data.data(), &sig_len,
                                 message.data(), message.size(),
                                 secret_key.data());

    if (rc != OQS_SUCCESS) {
        throw std::runtime_error("Falcon signing failed");
    }

    sig.data.resize(sig_len);
    return sig;
}

bool FalconLiboqsProvider::Verify(const std::vector<uint8_t>& message,
                                  const Signature& signature,
                                  const std::vector<uint8_t>& public_key) {
    if (signature.algorithm != algo_) {
        return false;
    }

    OQS_STATUS rc = OQS_SIG_verify(sig_,
                                   message.data(), message.size(),
                                   signature.data.data(), signature.data.size(),
                                   public_key.data());

    return rc == OQS_SUCCESS;
}

//=============================================================================
// ML-KEM Provider (Standardized Kyber)
//=============================================================================

MLKEMProvider::MLKEMProvider(Algorithm algo) : algo_(algo), kem_(nullptr) {
    const char* alg_name = GetOQSAlgName();
    if (!alg_name) {
        throw std::invalid_argument("Invalid ML-KEM variant");
    }

    kem_ = OQS_KEM_new(alg_name);
    if (!kem_) {
        throw std::runtime_error("Failed to initialize ML-KEM algorithm");
    }
}

MLKEMProvider::~MLKEMProvider() {
    if (kem_) {
        OQS_KEM_free(kem_);
    }
}

const char* MLKEMProvider::GetOQSAlgName() const {
    switch (algo_) {
        case Algorithm::KYBER512:
            return OQS_KEM_alg_ml_kem_512;
        case Algorithm::KYBER768:
            return OQS_KEM_alg_ml_kem_768;
        case Algorithm::KYBER1024:
            return OQS_KEM_alg_ml_kem_1024;
        default:
            return nullptr;
    }
}

KeyPair MLKEMProvider::GenerateKeyPair() {
    KeyPair kp;
    kp.algorithm = algo_;
    kp.public_key.resize(kem_->length_public_key);
    kp.secret_key.resize(kem_->length_secret_key);

    OQS_STATUS rc = OQS_KEM_keypair(kem_,
                                    kp.public_key.data(),
                                    kp.secret_key.data());

    if (rc != OQS_SUCCESS) {
        throw std::runtime_error("ML-KEM key generation failed");
    }

    return kp;
}

KEMResult MLKEMProvider::Encapsulate(const std::vector<uint8_t>& public_key) {
    KEMResult result;
    result.algorithm = algo_;
    result.ciphertext.resize(kem_->length_ciphertext);
    result.shared_secret.resize(kem_->length_shared_secret);

    OQS_STATUS rc = OQS_KEM_encaps(kem_,
                                   result.ciphertext.data(),
                                   result.shared_secret.data(),
                                   public_key.data());

    if (rc != OQS_SUCCESS) {
        throw std::runtime_error("ML-KEM encapsulation failed");
    }

    return result;
}

std::vector<uint8_t> MLKEMProvider::Decapsulate(const std::vector<uint8_t>& ciphertext,
                                                const std::vector<uint8_t>& secret_key) {
    std::vector<uint8_t> shared_secret(kem_->length_shared_secret);

    OQS_STATUS rc = OQS_KEM_decaps(kem_,
                                   shared_secret.data(),
                                   ciphertext.data(),
                                   secret_key.data());

    if (rc != OQS_SUCCESS) {
        throw std::runtime_error("ML-KEM decapsulation failed");
    }

    return shared_secret;
}

//=============================================================================
// Legacy Kyber Provider (for backwards compatibility)
//=============================================================================

KyberLiboqsProvider::KyberLiboqsProvider(Algorithm algo) : algo_(algo), kem_(nullptr) {
    const char* alg_name = nullptr;

    switch (algo) {
        case Algorithm::KYBER512:
            alg_name = OQS_KEM_alg_kyber_512;
            break;
        case Algorithm::KYBER768:
            alg_name = OQS_KEM_alg_kyber_768;
            break;
        case Algorithm::KYBER1024:
            alg_name = OQS_KEM_alg_kyber_1024;
            break;
        default:
            throw std::invalid_argument("Invalid Kyber variant");
    }

    kem_ = OQS_KEM_new(alg_name);
    if (!kem_) {
        // Kyber might be disabled in newer liboqs, try ML-KEM instead
        throw std::runtime_error("Failed to initialize Kyber algorithm. "
                               "Consider using ML-KEM (FIPS 203) instead.");
    }
}

KyberLiboqsProvider::~KyberLiboqsProvider() {
    if (kem_) {
        OQS_KEM_free(kem_);
    }
}

KeyPair KyberLiboqsProvider::GenerateKeyPair() {
    KeyPair kp;
    kp.algorithm = algo_;
    kp.public_key.resize(kem_->length_public_key);
    kp.secret_key.resize(kem_->length_secret_key);

    OQS_STATUS rc = OQS_KEM_keypair(kem_,
                                    kp.public_key.data(),
                                    kp.secret_key.data());

    if (rc != OQS_SUCCESS) {
        throw std::runtime_error("Kyber key generation failed");
    }

    return kp;
}

KEMResult KyberLiboqsProvider::Encapsulate(const std::vector<uint8_t>& public_key) {
    KEMResult result;
    result.algorithm = algo_;
    result.ciphertext.resize(kem_->length_ciphertext);
    result.shared_secret.resize(kem_->length_shared_secret);

    OQS_STATUS rc = OQS_KEM_encaps(kem_,
                                   result.ciphertext.data(),
                                   result.shared_secret.data(),
                                   public_key.data());

    if (rc != OQS_SUCCESS) {
        throw std::runtime_error("Kyber encapsulation failed");
    }

    return result;
}

std::vector<uint8_t> KyberLiboqsProvider::Decapsulate(const std::vector<uint8_t>& ciphertext,
                                                       const std::vector<uint8_t>& secret_key) {
    std::vector<uint8_t> shared_secret(kem_->length_shared_secret);

    OQS_STATUS rc = OQS_KEM_decaps(kem_,
                                   shared_secret.data(),
                                   ciphertext.data(),
                                   secret_key.data());

    if (rc != OQS_SUCCESS) {
        throw std::runtime_error("Kyber decapsulation failed");
    }

    return shared_secret;
}

//=============================================================================
// Factory Functions
//=============================================================================

std::unique_ptr<ISignatureProvider> CreateLiboqsSignatureProvider(Algorithm algo) {
    switch (algo) {
        case Algorithm::DILITHIUM2:
        case Algorithm::DILITHIUM3:
        case Algorithm::DILITHIUM5:
            return std::make_unique<MLDSAProvider>(algo);
        case Algorithm::FALCON512:
        case Algorithm::FALCON1024:
            return std::make_unique<FalconLiboqsProvider>(algo);
        default:
            throw std::invalid_argument("Not a supported liboqs signature algorithm");
    }
}

std::unique_ptr<IKEMProvider> CreateLiboqsKEMProvider(Algorithm algo) {
    // Prefer ML-KEM (FIPS 203) over legacy Kyber
    try {
        return std::make_unique<MLKEMProvider>(algo);
    } catch (...) {
        // Fallback to legacy Kyber if ML-KEM not available
        return std::make_unique<KyberLiboqsProvider>(algo);
    }
}

std::unique_ptr<ISignatureProvider> CreateBestSignatureProvider(Algorithm algo) {
    try {
        return CreateLiboqsSignatureProvider(algo);
    } catch (...) {
        // Fallback to reference implementation
        return CreateSignatureProvider(algo);
    }
}

std::unique_ptr<IKEMProvider> CreateBestKEMProvider(Algorithm algo) {
    try {
        return CreateLiboqsKEMProvider(algo);
    } catch (...) {
        // Fallback to reference implementation
        return CreateKEMProvider(algo);
    }
}

#else // !HAVE_LIBOQS

// When liboqs is not available, these are handled by the inline stubs in the header

#endif // HAVE_LIBOQS

} // namespace pqc
} // namespace dions2
