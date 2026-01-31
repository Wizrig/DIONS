// Copyright (c) 2026 DIONS Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef IOCOIN_OPENSSL_COMPAT_H
#define IOCOIN_OPENSSL_COMPAT_H

/**
 * OpenSSL Compatibility Layer
 *
 * Purpose: Provide compatibility shims for OpenSSL 1.0.1 → 1.1.1+ and 3.0+
 *
 * Strategy:
 *   - OpenSSL 1.1.1+: Use new API names directly
 *   - OpenSSL 1.0.1 (legacy): Provide compatibility macros
 *   - OpenSSL 3.0+: Test with default provider first, legacy only if needed
 *
 * CRITICAL: This layer provides API compatibility ONLY.
 *           Cryptographic behavior, key formats, and message formats
 *           MUST remain identical to preserve consensus and wallet compatibility.
 *
 * Testing: All crypto operations must be verified for behavior equivalence
 *          between OpenSSL versions (key generation, signing, ECDH, encryption).
 */

#include <openssl/opensslv.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/ec.h>
#include <openssl/ecdh.h>
#include <openssl/rand.h>
#include <openssl/bn.h>

// OpenSSL version detection
#if OPENSSL_VERSION_NUMBER < 0x10100000L
  #define OPENSSL_1_0_X
#elif OPENSSL_VERSION_NUMBER < 0x30000000L
  #define OPENSSL_1_1_X
#else
  #define OPENSSL_3_X
#endif

//
// EVP_MD_CTX compatibility (message digest context)
//
#ifdef OPENSSL_1_0_X
  // OpenSSL 1.0.x uses create/destroy naming
  #define EVP_MD_CTX_new EVP_MD_CTX_create
  #define EVP_MD_CTX_free EVP_MD_CTX_destroy
#endif
// OpenSSL 1.1.x and 3.x use new/free natively


//
// RSA structure accessors (OpenSSL 1.1.0+ made RSA opaque)
//
#if defined(OPENSSL_1_0_X)
  // OpenSSL 1.0.x: Direct field access allowed, but provide getters for consistency
  inline void RSA_get0_key(const RSA *r, const BIGNUM **n, const BIGNUM **e, const BIGNUM **d)
  {
    if (n != NULL) *n = r->n;
    if (e != NULL) *e = r->e;
    if (d != NULL) *d = r->d;
  }

  inline void RSA_get0_factors(const RSA *r, const BIGNUM **p, const BIGNUM **q)
  {
    if (p != NULL) *p = r->p;
    if (q != NULL) *q = r->q;
  }

  inline void RSA_get0_crt_params(const RSA *r, const BIGNUM **dmp1, const BIGNUM **dmq1, const BIGNUM **iqmp)
  {
    if (dmp1 != NULL) *dmp1 = r->dmp1;
    if (dmq1 != NULL) *dmq1 = r->dmq1;
    if (iqmp != NULL) *iqmp = r->iqmp;
  }

  inline int RSA_set0_key(RSA *r, BIGNUM *n, BIGNUM *e, BIGNUM *d)
  {
    if ((r->n == NULL && n == NULL) || (r->e == NULL && e == NULL))
      return 0;
    if (n != NULL) { BN_free(r->n); r->n = n; }
    if (e != NULL) { BN_free(r->e); r->e = e; }
    if (d != NULL) { BN_free(r->d); r->d = d; }
    return 1;
  }

  inline int RSA_set0_factors(RSA *r, BIGNUM *p, BIGNUM *q)
  {
    if ((r->p == NULL && p == NULL) || (r->q == NULL && q == NULL))
      return 0;
    if (p != NULL) { BN_free(r->p); r->p = p; }
    if (q != NULL) { BN_free(r->q); r->q = q; }
    return 1;
  }
#endif
// OpenSSL 1.1.x and 3.x provide these functions natively


//
// EC_KEY structure accessors (OpenSSL 1.1.0+ made EC_KEY opaque)
//
#if defined(OPENSSL_1_0_X)
  // OpenSSL 1.0.x: Direct field access allowed, provide getters for forward compatibility
  inline const EC_GROUP* EC_KEY_get0_group(const EC_KEY *key)
  {
    return key->group;
  }

  inline const BIGNUM* EC_KEY_get0_private_key(const EC_KEY *key)
  {
    return key->priv_key;
  }

  inline const EC_POINT* EC_KEY_get0_public_key(const EC_KEY *key)
  {
    return key->pub_key;
  }

  inline int EC_KEY_set_private_key(EC_KEY *key, const BIGNUM *priv_key)
  {
    if (key->priv_key) BN_free(key->priv_key);
    key->priv_key = BN_dup(priv_key);
    return (key->priv_key != NULL);
  }

  inline int EC_KEY_set_public_key(EC_KEY *key, const EC_POINT *pub_key)
  {
    if (key->pub_key) EC_POINT_free(key->pub_key);
    key->pub_key = EC_POINT_dup(pub_key, key->group);
    return (key->pub_key != NULL);
  }
#endif
// OpenSSL 1.1.x and 3.x provide these functions natively


//
// ECDH compatibility (OpenSSL 1.1.0 deprecated ECDH_compute_key)
//
#if defined(OPENSSL_1_1_X) || defined(OPENSSL_3_X)
  // For OpenSSL 1.1.x and 3.x, ECDH_compute_key is deprecated but still available
  // If it's removed in future, we'll need EVP_PKEY_derive replacement
  // Current strategy: Use deprecated function with -Wno-deprecated-declarations
  // TODO: Migrate to EVP_PKEY_derive in future phase
#endif


//
// OpenSSL 3.0+ specific compatibility
//
#if defined(OPENSSL_3_X)
  // OpenSSL 3.0 moved many algorithms to "legacy" provider
  // Strategy: Test with default provider first
  // If RSA_generate_key_ex, ECDH, or other functions fail, load legacy provider

  // Note: Legacy provider loading would be added here if empirical testing shows it's needed
  // Example (not enabled by default):
  /*
  #include <openssl/provider.h>

  inline void load_openssl3_legacy_provider()
  {
    static bool loaded = false;
    if (!loaded) {
      OSSL_PROVIDER_load(NULL, "legacy");
      OSSL_PROVIDER_load(NULL, "default");
      loaded = true;
    }
  }
  */

  // For now, assume default provider is sufficient
  // If testing reveals failures, uncomment above and call in init functions
#endif


//
// Utility: OpenSSL version string for logging
//
inline const char* OpenSSL_version_str()
{
#if defined(OPENSSL_1_0_X)
  return "OpenSSL 1.0.x (compatibility mode)";
#elif defined(OPENSSL_1_1_X)
  return "OpenSSL 1.1.x";
#elif defined(OPENSSL_3_X)
  return "OpenSSL 3.x";
#else
  return "OpenSSL (unknown version)";
#endif
}


//
// Compile-time checks
//
#if OPENSSL_VERSION_NUMBER < 0x10001000L
  // Require at least OpenSSL 1.0.1 (for ECDH support)
  #error "OpenSSL 1.0.1 or later required"
#endif

#if defined(OPENSSL_1_0_X)
  // Warn about EOL version (but still support for migration)
  #warning "OpenSSL 1.0.x is end-of-life. Upgrade to 1.1.1+ or 3.0+ recommended."
#endif


#endif // IOCOIN_OPENSSL_COMPAT_H
