#include "treecast.h"
#include <string.h>

static size_t leaf_offset(size_t num_devices) { return 1 + (num_devices - 1); }
static size_t parent(size_t idx)      { return idx / 2; }
static size_t left_child(size_t idx)  { return 2 * idx; }
static size_t right_child(size_t idx) { return 2 * idx + 1; }

int treecast_init(treecast_ctx_t *ctx, size_t num_devices)
{
    if (num_devices < 2 || num_devices > TREECAST_MAX_DEVICES) return -1;
    ctx->num_devices = num_devices;
    size_t total_nodes = 2 * num_devices;
    for (size_t i = 0; i < total_nodes; i++) ctx->nodes[i].has_secret = 0;

    for (size_t i = 0; i < num_devices; i++) {
        if (tc_ec_keypair_gen(&ctx->dev_keys[i]) != 0) return -1;
    }
    return 0;
}

int treecast_derive_all(treecast_ctx_t *ctx)
{
    size_t n = ctx->num_devices;
    size_t offset = leaf_offset(n);

    for (size_t i = 0; i < n; i++) {
        size_t idx = offset + i;
        tc_hash(ctx->dev_keys[i].pub, TREECAST_PUB_BYTES, ctx->nodes[idx].secret);
        ctx->nodes[idx].has_secret = 1;
    }

    for (size_t idx = offset - 1; idx > 0; idx--) {
        size_t l = left_child(idx);
        size_t r = right_child(idx);
        if (l >= 2 * n || r >= 2 * n) continue;
        if (!ctx->nodes[l].has_secret || !ctx->nodes[r].has_secret) continue;

        uint8_t buf[2 * TREECAST_KEY_BYTES];
        memcpy(buf, ctx->nodes[l].secret, TREECAST_KEY_BYTES);
        memcpy(buf + TREECAST_KEY_BYTES, ctx->nodes[r].secret, TREECAST_KEY_BYTES);
        tc_hash(buf, sizeof(buf), ctx->nodes[idx].secret);
        ctx->nodes[idx].has_secret = 1;
    }
    return 0;
}

int treecast_broadcast_key(treecast_ctx_t *ctx, uint8_t out[TREECAST_KEY_BYTES])
{
    if (!ctx->nodes[1].has_secret) return -1;
    memcpy(out, ctx->nodes[1].secret, TREECAST_KEY_BYTES);
    return 0;
}

static size_t lca_index(size_t a_idx, size_t b_idx)
{
    while (a_idx != b_idx) {
        if (a_idx > b_idx) a_idx = parent(a_idx);
        else b_idx = parent(b_idx);
    }
    return a_idx;
}

int treecast_multicast_key(treecast_ctx_t *ctx, size_t first_dev, size_t last_dev, uint8_t out[TREECAST_KEY_BYTES])
{
    size_t offset = leaf_offset(ctx->num_devices);
    size_t lca = lca_index(offset + first_dev, offset + last_dev);
    if (!ctx->nodes[lca].has_secret) return -1;
    memcpy(out, ctx->nodes[lca].secret, TREECAST_KEY_BYTES);
    return 0;
}

int treecast_unicast_key(treecast_ctx_t *ctx, size_t dev_i, size_t dev_j, uint8_t out[TREECAST_KEY_BYTES])
{
    return tc_ec_hdh(ctx->dev_keys[dev_i].priv, ctx->dev_keys[dev_j].pub, out);
}

/* Path Rekeying */
int treecast_rekey_path(treecast_ctx_t *ctx, size_t leaf_idx)
{
    size_t offset = leaf_offset(ctx->num_devices);
    size_t curr = offset + leaf_idx;

    uint8_t dummy_pub[64]; 
    memset(dummy_pub, 0xAB, 64); // Dummy new public key
    tc_hash(dummy_pub, 64, ctx->nodes[curr].secret);

    /* Path Update */
    curr = parent(curr);
    while (curr > 0) {
        size_t l = left_child(curr);
        size_t r = right_child(curr);
        
        uint8_t buf[64];
        memcpy(buf, ctx->nodes[l].secret, 32);
        memcpy(buf + 32, ctx->nodes[r].secret, 32);
        tc_hash(buf, 64, ctx->nodes[curr].secret);
        
        curr = parent(curr);
    }
    return 0;
}