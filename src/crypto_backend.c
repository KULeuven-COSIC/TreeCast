#include "crypto_backend.h"
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/logging/log.h>

#include <ocrypto_ecdh_p256.h>
#include <ocrypto_sha256.h>

/* AES (CTR) */
extern void soft_aes_ctr_encrypt(const uint8_t *key,
                                 const uint8_t *iv,
                                 const uint8_t *data,
                                 uint32_t len,
                                 uint8_t *out);

LOG_MODULE_REGISTER(crypto_backend, LOG_LEVEL_INF);

/* SHA-256 hash */
int tc_hash(const uint8_t *in, size_t in_len, uint8_t out[32])
{
    ocrypto_sha256_ctx ctx;

    ocrypto_sha256_init(&ctx);
    ocrypto_sha256_update(&ctx, in, in_len);
    ocrypto_sha256_final(&ctx, out);

    return 0;
}

/* EC keypair generation on secp256r1 (P-256) */
int tc_ec_keypair_gen(tc_ec_keypair_t *kp)
{
    /* Random private key from Zephyr CSPRNG */
    sys_csrand_get(kp->priv, TREECAST_PRIV_BYTES);

    /* Derive public key */
    int ret = ocrypto_ecdh_p256_public_key(kp->pub, kp->priv);
    return (ret == 0) ? 0 : -1;
}

/* Hybrid Diffie-Hellman: H(sk * pk_peer) */
int tc_ec_hdh(const uint8_t priv[TREECAST_PRIV_BYTES],
              const uint8_t peer_pub[TREECAST_PUB_BYTES],
              uint8_t out[TREECAST_KEY_BYTES])
{
    uint8_t shared_secret[32];

    int ret = ocrypto_ecdh_p256_common_secret(shared_secret,
                                              priv,
                                              peer_pub);
    if (ret != 0) {
        return -1;
    }

    return tc_hash(shared_secret, sizeof(shared_secret), out);
}

/* Symmetric encryption
 *
 * For benchmarking, we implement AES-CTR in software and fabricate an
 * authentication tag. This gives us realistic CPU load without depending
 * on a specific hardware AEAD engine.
 */
int tc_aes_ctr_encrypt(const uint8_t *key,
                       const uint8_t *iv,
                       size_t iv_len,
                       const uint8_t *input,
                       size_t len,
                       uint8_t *output,
                       uint8_t *tag,
                       size_t tag_len)
{
    uint8_t safe_iv[16] = {0};

    /* Clamp IV to 16 bytes and copy */
    if (iv_len > sizeof(safe_iv)) {
        iv_len = sizeof(safe_iv);
    }
    memcpy(safe_iv, iv, iv_len);

    soft_aes_ctr_encrypt(key, safe_iv, input, (uint32_t)len, output);

    /* Dummy tag content; cost is dominated by AES anyway */
    if (tag && tag_len > 0) {
        memset(tag, 0xAA, tag_len);
    }

    return 0;
}