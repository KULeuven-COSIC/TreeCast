#ifndef CRYPTO_BACKEND_H
#define CRYPTO_BACKEND_H

#include <stdint.h>
#include <stddef.h>

#define TREECAST_KEY_BYTES 32   // 256-bit keys
#define TREECAST_PRIV_BYTES 32
#define TREECAST_PUB_BYTES  64  // uncompressed x||y

typedef struct {
    uint8_t priv[TREECAST_PRIV_BYTES];
    uint8_t pub[TREECAST_PUB_BYTES];
} tc_ec_keypair_t;

// Generate EC keypair on secp256r1
int tc_ec_keypair_gen(tc_ec_keypair_t *kp);

// Compute HDH shared secret: H(sk * pk_peer)
int tc_ec_hdh(const uint8_t priv[TREECAST_PRIV_BYTES],
              const uint8_t peer_pub[TREECAST_PUB_BYTES],
              uint8_t out[TREECAST_KEY_BYTES]);

// SHA-256 hash
int tc_hash(const uint8_t *in, size_t in_len,
            uint8_t out[32]);

#endif