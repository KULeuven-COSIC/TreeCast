#ifndef TREECAST_H
#define TREECAST_H

#include <stdint.h>
#include <stddef.h>
#include "crypto_backend.h"

#define TREECAST_MAX_DEVICES 32
#define TREECAST_MAX_NODES   (2 * TREECAST_MAX_DEVICES)

typedef struct {
    uint8_t secret[TREECAST_KEY_BYTES];
    uint8_t has_secret;
} tc_node_t;

typedef struct {
    size_t num_devices;
    tc_node_t nodes[TREECAST_MAX_NODES];
    tc_ec_keypair_t dev_keys[TREECAST_MAX_DEVICES];
} treecast_ctx_t;

/* Core protocol functions */
int treecast_init(treecast_ctx_t *ctx, size_t num_devices);
int treecast_derive_all(treecast_ctx_t *ctx);

/* Communication modes */
int treecast_unicast_key(treecast_ctx_t *ctx, size_t dev_i, size_t dev_j,
                         uint8_t out[TREECAST_KEY_BYTES]);
int treecast_multicast_key(treecast_ctx_t *ctx, size_t first_dev, size_t last_dev,
                           uint8_t out[TREECAST_KEY_BYTES]);
int treecast_broadcast_key(treecast_ctx_t *ctx,
                           uint8_t out[TREECAST_KEY_BYTES]);

/* Benchmarking helper: path rekey */
int treecast_rekey_path(treecast_ctx_t *ctx, size_t leaf_idx);

/* Symmetric crypto helper (AES-CTR wrapper) */
int tc_aes_ctr_encrypt(const uint8_t *key, const uint8_t *iv, size_t iv_len,
                       const uint8_t *input, size_t len,
                       uint8_t *output, uint8_t *tag, size_t tag_len);

#endif