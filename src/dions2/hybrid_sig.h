// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 - Hybrid Signature Module
//
// Provides hybrid classical + PQC signatures for DIONS messages.
// During the transition period, messages can be signed with both
// Ed25519/ECDSA (classical) and Falcon/Dilithium (PQC).

#ifndef DIONS2_HYBRID_SIG_H
#define DIONS2_HYBRID_SIG_H

#include "crypto/pqc.h"
#include "../serialize.h"
#include <vector>
#include <string>
#include <array>

namespace dions2 {

// Signature modes
enum class SignatureMode {
    CLASSICAL_ONLY,   // Ed25519/ECDSA only (backwards compatible)
    PQC_ONLY,         // Falcon/Dilithium only (post-quantum secure)
    HYBRID            // Both classical + PQC (recommended for transition)
};

// Signature scheme identifiers (stored in messages)
enum class SignatureScheme : uint8_t {
    ECDSA_SECP256K1 = 0x00,     // Legacy Bitcoin/IOC ECDSA
    ED25519         = 0x01,     // EdDSA (compact)
    FALCON512       = 0x10,     // PQC: Falcon-512 (IoT optimized)
    FALCON1024      = 0x11,     // PQC: Falcon-1024 (high security)
    DILITHIUM2      = 0x20,     // PQC: Dilithium2
    DILITHIUM3      = 0x21,     // PQC: Dilithium3 (recommended)
    DILITHIUM5      = 0x22,     // PQC: Dilithium5 (paranoid)
    HYBRID_ED25519_FALCON512  = 0x80,  // Ed25519 + Falcon512
    HYBRID_ED25519_DILITHIUM3 = 0x81,  // Ed25519 + Dilithium3
    HYBRID_ECDSA_FALCON512    = 0x82,  // ECDSA + Falcon512
    HYBRID_ECDSA_DILITHIUM3   = 0x83   // ECDSA + Dilithium3
};

// DIONS 2.0 message signature structure
struct DionsSignature {
    SignatureScheme scheme;
    std::vector<uint8_t> classical_sig;  // Ed25519 or ECDSA signature
    std::vector<uint8_t> pqc_sig;        // Falcon or Dilithium signature

    DionsSignature() : scheme(SignatureScheme::ECDSA_SECP256K1) {}

    bool IsHybrid() const {
        return static_cast<uint8_t>(scheme) >= 0x80;
    }

    bool IsPQC() const {
        uint8_t s = static_cast<uint8_t>(scheme);
        return (s >= 0x10 && s < 0x80);
    }

    bool IsClassical() const {
        return static_cast<uint8_t>(scheme) < 0x10;
    }

    size_t Size() const {
        return 1 + classical_sig.size() + pqc_sig.size();
    }

    // Serialization
    template<typename Stream>
    void Serialize(Stream& s) const {
        s << static_cast<uint8_t>(scheme);
        uint32_t classical_len = classical_sig.size();
        uint32_t pqc_len = pqc_sig.size();
        s << classical_len << pqc_len;
        if (classical_len > 0) {
            s.write(reinterpret_cast<const char*>(classical_sig.data()), classical_len);
        }
        if (pqc_len > 0) {
            s.write(reinterpret_cast<const char*>(pqc_sig.data()), pqc_len);
        }
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        uint8_t scheme_byte;
        s >> scheme_byte;
        scheme = static_cast<SignatureScheme>(scheme_byte);

        uint32_t classical_len, pqc_len;
        s >> classical_len >> pqc_len;

        classical_sig.resize(classical_len);
        pqc_sig.resize(pqc_len);

        if (classical_len > 0) {
            s.read(reinterpret_cast<char*>(classical_sig.data()), classical_len);
        }
        if (pqc_len > 0) {
            s.read(reinterpret_cast<char*>(pqc_sig.data()), pqc_len);
        }
    }
};

// DIONS 2.0 public key structure (for hybrid keys)
struct DionsPublicKey {
    SignatureScheme scheme;
    std::vector<uint8_t> classical_pk;   // Ed25519 or ECDSA public key
    std::vector<uint8_t> pqc_pk;         // Falcon or Dilithium public key

    DionsPublicKey() : scheme(SignatureScheme::ECDSA_SECP256K1) {}

    bool IsValid() const {
        if (IsClassical()) return !classical_pk.empty();
        if (IsPQC()) return !pqc_pk.empty();
        if (IsHybrid()) return !classical_pk.empty() && !pqc_pk.empty();
        return false;
    }

    bool IsHybrid() const {
        return static_cast<uint8_t>(scheme) >= 0x80;
    }

    bool IsPQC() const {
        uint8_t s = static_cast<uint8_t>(scheme);
        return (s >= 0x10 && s < 0x80);
    }

    bool IsClassical() const {
        return static_cast<uint8_t>(scheme) < 0x10;
    }

    // Get fingerprint (SHA256 of public key(s))
    std::array<uint8_t, 32> GetFingerprint() const;

    // Serialization
    template<typename Stream>
    void Serialize(Stream& s) const {
        s << static_cast<uint8_t>(scheme);
        uint32_t classical_len = classical_pk.size();
        uint32_t pqc_len = pqc_pk.size();
        s << classical_len << pqc_len;
        if (classical_len > 0) {
            s.write(reinterpret_cast<const char*>(classical_pk.data()), classical_len);
        }
        if (pqc_len > 0) {
            s.write(reinterpret_cast<const char*>(pqc_pk.data()), pqc_len);
        }
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        uint8_t scheme_byte;
        s >> scheme_byte;
        scheme = static_cast<SignatureScheme>(scheme_byte);

        uint32_t classical_len, pqc_len;
        s >> classical_len >> pqc_len;

        classical_pk.resize(classical_len);
        pqc_pk.resize(pqc_len);

        if (classical_len > 0) {
            s.read(reinterpret_cast<char*>(classical_pk.data()), classical_len);
        }
        if (pqc_len > 0) {
            s.read(reinterpret_cast<char*>(pqc_pk.data()), pqc_len);
        }
    }
};

// Hybrid Signer class
class HybridSigner {
public:
    /**
     * Sign a DIONS message with the specified mode
     *
     * @param message The message bytes to sign
     * @param classical_privkey Classical private key (Ed25519 or ECDSA)
     * @param pqc_keypair PQC key pair (optional for non-hybrid modes)
     * @param mode Signature mode
     * @param pqc_algo PQC algorithm to use
     * @return DionsSignature containing the signature(s)
     */
    static DionsSignature Sign(
        const std::vector<uint8_t>& message,
        const std::vector<uint8_t>& classical_privkey,
        const pqc::KeyPair* pqc_keypair = nullptr,
        SignatureMode mode = SignatureMode::HYBRID,
        pqc::Algorithm pqc_algo = pqc::Algorithm::FALCON512
    );

    /**
     * Verify a DIONS signature
     *
     * @param message The message bytes
     * @param signature The signature to verify
     * @param pubkey The public key
     * @param require_both For hybrid, require both sigs valid (vs either-or)
     * @return true if signature is valid
     */
    static bool Verify(
        const std::vector<uint8_t>& message,
        const DionsSignature& signature,
        const DionsPublicKey& pubkey,
        bool require_both = true
    );

    /**
     * Generate a new hybrid key pair
     *
     * @param scheme The scheme to use
     * @param pqc_algo PQC algorithm for hybrid/PQC modes
     * @return Pair of (public_key, secret_key_bytes)
     */
    static std::pair<DionsPublicKey, std::vector<uint8_t>> GenerateKeyPair(
        SignatureScheme scheme,
        pqc::Algorithm pqc_algo = pqc::Algorithm::FALCON512
    );

    /**
     * Get recommended scheme for a device profile
     */
    static SignatureScheme GetRecommendedScheme(pqc::DeviceProfile profile);

    /**
     * Check if a scheme is quantum-resistant
     */
    static bool IsQuantumResistant(SignatureScheme scheme);

    /**
     * Get human-readable scheme name
     */
    static std::string SchemeName(SignatureScheme scheme);

private:
    // Sign with classical algorithm
    static std::vector<uint8_t> SignClassical(
        const std::vector<uint8_t>& message,
        const std::vector<uint8_t>& privkey,
        SignatureScheme scheme
    );

    // Verify classical signature
    static bool VerifyClassical(
        const std::vector<uint8_t>& message,
        const std::vector<uint8_t>& signature,
        const std::vector<uint8_t>& pubkey,
        SignatureScheme scheme
    );

    // Sign with PQC algorithm
    static std::vector<uint8_t> SignPQC(
        const std::vector<uint8_t>& message,
        const std::vector<uint8_t>& privkey,
        pqc::Algorithm algo
    );

    // Verify PQC signature
    static bool VerifyPQC(
        const std::vector<uint8_t>& message,
        const std::vector<uint8_t>& signature,
        const std::vector<uint8_t>& pubkey,
        pqc::Algorithm algo
    );
};

// Get PQC algorithm from signature scheme
pqc::Algorithm GetPQCAlgorithmFromScheme(SignatureScheme scheme);

// Convert scheme to string
std::string SignatureSchemeToString(SignatureScheme scheme);

} // namespace dions2

#endif // DIONS2_HYBRID_SIG_H
