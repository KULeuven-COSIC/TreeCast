#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys_clock.h>
#include <math.h>

#include "treecast.h"

#define NUM_DEVICES   8
#define NUM_ITERS     100
#define MSG_SIZE      64

/* Assume nRF5340 application core at 64 MHz */
#define CPU_FREQ_HZ   64000000u

static treecast_ctx_t g_ctx;

/* Average costs of primitive operations (per *single* operation, in timer cycles) */
static uint32_t cost_kgen_cycles  = 0;
static uint32_t cost_hash_cycles  = 0;
static uint32_t cost_ecdh_cycles  = 0;
static uint32_t cost_aes_cycles   = 0;


/* Timing helpers */
#define BENCH_START() uint32_t __bench_start = k_cycle_get_32()
#define BENCH_END(acc)                                                        \
    do {                                                                      \
        uint32_t __bench_end = k_cycle_get_32();                              \
        (acc) += (uint64_t)(__bench_end - __bench_start);                     \
    } while (0)

/* Convert timer cycles (system clock) to nanoseconds */
static inline uint64_t cycles_to_ns(uint64_t cycles)
{
    return (cycles * 1000000000ull) / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
}

/* Convert timer cycles to estimated CPU cycles (64 MHz core) */
static inline uint64_t timer_to_cpu_cycles(uint32_t timer_cycles)
{
    return ((uint64_t)timer_cycles * CPU_FREQ_HZ) /
           (uint64_t)CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
}

static void print_cycles_time(const char *label,
                              uint32_t cycles_per_op,
                              uint32_t iters)
{
    if (cycles_per_op == 0) {
        printk("%s\n", label);
        printk("      - per-op:   %10u timer cycles ( < 1 tick )\n",
               cycles_per_op);
        printk("      - over %3u:          0 timer cycles\n", iters);
        return;
    }

    uint64_t total_cycles = (uint64_t)cycles_per_op * iters;
    uint64_t total_ns     = cycles_to_ns(total_cycles);

    uint32_t total_ms_int  = (uint32_t)(total_ns / 1000000ull);
    uint32_t total_ms_frac = (uint32_t)((total_ns % 1000000ull) / 1000ull);

    uint64_t per_ns     = cycles_to_ns(cycles_per_op);
    uint32_t per_ms_int  = (uint32_t)(per_ns / 1000000ull);
    uint32_t per_ms_frac = (uint32_t)((per_ns % 1000000ull) / 1000ull);

    uint64_t per_cpu_cycles   = timer_to_cpu_cycles(cycles_per_op);
    uint64_t total_cpu_cycles = timer_to_cpu_cycles((uint32_t)total_cycles);

    printk("%s\n", label);
    printk("      - per-op:   %10u timer cycles (%12llu CPU cycles) = %4u.%03u ms\n",
           cycles_per_op,
           (unsigned long long)per_cpu_cycles,
           per_ms_int, per_ms_frac);
    printk("      - over %3u: %10llu timer cycles (%12llu CPU cycles) = %4u.%03u ms\n",
           iters,
           (unsigned long long)total_cycles,
           (unsigned long long)total_cpu_cycles,
           total_ms_int, total_ms_frac);
}

static void print_primitive_cycle_table(void)
{
    printk("\n[1b] Timer vs CPU cycles (per operation, NUM_ITERS=%d)\n",
           NUM_ITERS);
    printk("   Assumed CPU clock: %u Hz\n", CPU_FREQ_HZ);
    printk("   System timer clock: %u Hz (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)\n",
           CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

    printk("   ----------------------------------------------------------\n");
    printk("   | Op       | Timer cycles |    CPU cycles (approx)       |\n");
    printk("   |----------|--------------|------------------------------|\n");

    if (cost_kgen_cycles) {
        printk("   | KGEN     | %12u | %12llu |\n",
               cost_kgen_cycles,
               (unsigned long long)timer_to_cpu_cycles(cost_kgen_cycles));
    }
    if (cost_hash_cycles) {
        printk("   | HASH     | %12u | %12llu |\n",
               cost_hash_cycles,
               (unsigned long long)timer_to_cpu_cycles(cost_hash_cycles));
    }
    if (cost_ecdh_cycles) {
        printk("   | HDH      | %12u | %12llu |\n",
               cost_ecdh_cycles,
               (unsigned long long)timer_to_cpu_cycles(cost_ecdh_cycles));
    }
    if (cost_aes_cycles) {
        printk("   | ENC(AES) | %12u | %12llu |\n",
               cost_aes_cycles,
               (unsigned long long)timer_to_cpu_cycles(cost_aes_cycles));
    }

    printk("   ----------------------------------------------------------\n");
}


/* [1] Primitive cryptographic operations */
static void benchmark_atomics(void)
{
    printk("\n[1] Primitive operations (NUM_ITERS=%d, MSG_SIZE=%d bytes)\n",
           NUM_ITERS, MSG_SIZE);

    uint8_t buf[MSG_SIZE] = {0};
    uint8_t out[32];
    uint8_t iv[12] = {0};
    uint8_t tag[16];
    int ret = 0;

    /* 1. EC key generation (P-256) */
    k_sleep(K_MSEC(100));
    {
        uint64_t total = 0;
        tc_ec_keypair_t tmp;

        for (int i = 0; i < NUM_ITERS; i++) {
            BENCH_START();
            ret = tc_ec_keypair_gen(&tmp);
            BENCH_END(total);

            if (ret != 0) {
                printk("   [KGEN] ERROR at iter %d (ret=%d)\n", i, ret);
                return;
            }
        }

        cost_kgen_cycles = (uint32_t)(total / NUM_ITERS);
        print_cycles_time("   [KGEN] EC key generation (secp256r1 sk,pk)",
                          cost_kgen_cycles, NUM_ITERS);
    }

    /* 2. Hash (SHA-256) on 64 bytes */
    k_sleep(K_MSEC(100));
    {
        uint64_t total = 0;

        for (int i = 0; i < NUM_ITERS; i++) {
            BENCH_START();
            ret = tc_hash(buf, sizeof(buf), out);
            BENCH_END(total);

            if (ret != 0) {
                printk("   [HASH] ERROR at iter %d (ret=%d)\n", i, ret);
                return;
            }
        }

        cost_hash_cycles = (uint32_t)(total / NUM_ITERS);
        print_cycles_time("   [HASH] SHA-256 over 64-byte message",
                          cost_hash_cycles, NUM_ITERS);
    }

    /* 3. HDH = ECDH + hash, using two device keys */
    k_sleep(K_MSEC(100));
    {
        uint64_t total = 0;

        for (int i = 0; i < NUM_ITERS; i++) {
            BENCH_START();
            ret = tc_ec_hdh(g_ctx.dev_keys[0].priv,
                            g_ctx.dev_keys[1].pub,
                            out);
            BENCH_END(total);

            if (ret != 0) {
                printk("   [HDH ] ERROR at iter %d (ret=%d)\n", i, ret);
                return;
            }
        }

        cost_ecdh_cycles = (uint32_t)(total / NUM_ITERS);
        print_cycles_time("   [HDH ] P-256 ECDH + SHA-256 (pairwise secret)",
                          cost_ecdh_cycles, NUM_ITERS);
    }

    /* 4. Symmetric encryption (AES-CTR, 64-byte payload) */
    k_sleep(K_MSEC(100));
    {
        uint64_t total = 0;

        for (int i = 0; i < NUM_ITERS; i++) {
            BENCH_START();
            ret = tc_aes_ctr_encrypt(out, iv, sizeof(iv),
                                     buf, MSG_SIZE,
                                     buf, tag, sizeof(tag));
            BENCH_END(total);

            if (ret != 0) {
                printk("   [ENC ] ERROR at iter %d (ret=%d)\n", i, ret);
                return;
            }
        }

        cost_aes_cycles = (uint32_t)(total / NUM_ITERS);
        print_cycles_time("   [ENC ] AES-CTR encryption of a 64-byte message",
                          cost_aes_cycles, NUM_ITERS);
    }

    /* Summary table: timer vs CPU cycles */
    print_primitive_cycle_table();
}


/* [2] Communication scenarios: unicast / multicast / broadcast */
static void benchmark_scenarios(treecast_ctx_t *ctx)
{
    printk("\n[2] Communication scenarios (NUM_DEVICES=%d, NUM_ITERS=%d)\n",
           NUM_DEVICES, NUM_ITERS);

    uint8_t key[TREECAST_KEY_BYTES];

    /* A. Unicast: pairwise between device 0 and device 1 */
    k_sleep(K_MSEC(100));
    {
        uint64_t total = 0;

        for (int i = 0; i < NUM_ITERS; i++) {
            BENCH_START();
            treecast_unicast_key(ctx, 0, 1, key);   /* HDH only */
            BENCH_END(total);
        }

        uint32_t deriv_cycles = (uint32_t)(total / NUM_ITERS);
        uint32_t total_cycles = deriv_cycles + cost_aes_cycles;

        print_cycles_time("   [UNICAST] key derivation (HDH only)",
                          deriv_cycles, NUM_ITERS);
        print_cycles_time("   [UNICAST] payload encryption (AES-CTR, 64 B)",
                          cost_aes_cycles, NUM_ITERS);
        print_cycles_time("   [UNICAST] total (HDH + AES-CTR)",
                          total_cycles, NUM_ITERS);
    }

    /* B. Multicast: subtree [0..3] */
    k_sleep(K_MSEC(100));
    {
        uint64_t total = 0;

        for (int i = 0; i < NUM_ITERS; i++) {
            BENCH_START();
            treecast_multicast_key(ctx, 0, 3, key); /* tree lookup only */
            BENCH_END(total);
        }

        uint32_t retr_cycles = (uint32_t)(total / NUM_ITERS);
        uint32_t total_cycles = retr_cycles + cost_aes_cycles;

        print_cycles_time("   [MULTICAST] key retrieval for subtree [0..3]",
                          retr_cycles, NUM_ITERS);
        print_cycles_time("   [MULTICAST] payload encryption (AES-CTR, 64 B)",
                          cost_aes_cycles, NUM_ITERS);
        print_cycles_time("   [MULTICAST] total (retrieval + AES-CTR)",
                          total_cycles, NUM_ITERS);
    }

    /* C. Broadcast: global root key */
    k_sleep(K_MSEC(100));
    {
        uint64_t total = 0;

        for (int i = 0; i < NUM_ITERS; i++) {
            BENCH_START();
            treecast_broadcast_key(ctx, key);       /* root only */
            BENCH_END(total);
        }

        uint32_t retr_cycles = (uint32_t)(total / NUM_ITERS);
        uint32_t total_cycles = retr_cycles + cost_aes_cycles;

        print_cycles_time("   [BROADCAST] root key retrieval (global)",
                          retr_cycles, NUM_ITERS);
        print_cycles_time("   [BROADCAST] payload encryption (AES-CTR, 64 B)",
                          cost_aes_cycles, NUM_ITERS);
        print_cycles_time("   [BROADCAST] total (retrieval + AES-CTR)",
                          total_cycles, NUM_ITERS);
    }
}

/* [3] Dynamic membership: path rekey cost */
static void benchmark_dynamic(treecast_ctx_t *ctx)
{
    printk("\n[3] Dynamic membership (path rekey from leaf 0 to root)\n");

    uint64_t total = 0;

    for (int i = 0; i < NUM_ITERS; i++) {
        BENCH_START();
        treecast_rekey_path(ctx, 0);   /* simulate rekey of device 0 */
        BENCH_END(total);
    }

    uint32_t avg_cycles = (uint32_t)(total / NUM_ITERS);
    print_cycles_time("   [REKEY] path rekey cost (N = 8 devices)",
                      avg_cycles, NUM_ITERS);
}

/* [4] Storage */
static void benchmark_storage(void)
{
    printk("\n[4] Storage cost (RAM)\n");
    printk("   [STRUCT] treecast_ctx_t (N=%d) : %d bytes\n",
           NUM_DEVICES, (int)sizeof(treecast_ctx_t));
    printk("   [STRUCT] tc_node_t             : %d bytes\n",
           (int)sizeof(tc_node_t));
    printk("   [STRUCT] tc_ec_keypair_t       : %d bytes\n",
           (int)sizeof(tc_ec_keypair_t));
}

/* [5] Scalability projection */
static void project_scalability(void)
{
    printk("\n[5] Scalability projection (theoretical model)\n");
    printk("   Model: Path_Cost(N) = ceil(log2 N) * Hash_Cost\n");
    printk("   Hash_Cost (from [HASH]): %u timer cycles per hash\n",
           cost_hash_cycles);

    int sizes[] = {10, 100, 1000, 10000, 100000};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    printk("   | Devices (N) | Height | Rekey (cycles) | Rekey (ms) |\n");
    printk("   |-------------|--------|----------------|------------|\n");

    for (int i = 0; i < num_sizes; i++) {
        int n = sizes[i];

        /* Height of balanced binary tree ≈ ceil(log2 N) */
        int height = 0;
        int tmp = n - 1;
        while (tmp > 0) {
            tmp >>= 1;
            height++;
        }
        if (height == 0) height = 1;

        uint32_t path_cycles = (uint32_t)(height * cost_hash_cycles);
        uint64_t ns          = cycles_to_ns(path_cycles);
        uint32_t ms_int      = (uint32_t)(ns / 1000000ull);
        uint32_t ms_frac     = (uint32_t)((ns % 1000000ull) / 1000ull);

        printk("   | %-11d | %-6d | %-14u | %4u.%03u ms |\n",
               n, height, path_cycles, ms_int, ms_frac);
    }
}



int main(void)
{
    
    k_sleep(K_MSEC(100));

    printk("\n=================================================\n");
    printk("   TreeCast benchmarking suite\n");
    printk("   NUM_DEVICES = %d, NUM_ITERS = %d, MSG_SIZE = %d bytes\n",
           NUM_DEVICES, NUM_ITERS, MSG_SIZE);
    printk("   Timer clock (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) = %u Hz\n",
           CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
    printk("   Assumed CPU core clock                               = %u Hz\n",
           CPU_FREQ_HZ);
    printk("=================================================\n");

    treecast_ctx_t *ctx = &g_ctx;

    k_sleep(K_MSEC(100));

    int ret = treecast_init(ctx, NUM_DEVICES);
    if (ret != 0) {
        printk("[ERROR] treecast_init failed: %d\n", ret);
        return 0;
    }

    k_sleep(K_MSEC(100));

    ret = treecast_derive_all(ctx);
    if (ret != 0) {
        printk("[ERROR] treecast_derive_all failed: %d\n", ret);
        return 0;
    }

    k_sleep(K_MSEC(100));
    benchmark_atomics();

    k_sleep(K_MSEC(100));
    benchmark_scenarios(ctx);

    k_sleep(K_MSEC(100));
    benchmark_dynamic(ctx);

    k_sleep(K_MSEC(100));
    benchmark_storage();

    k_sleep(K_MSEC(100));
    project_scalability();

    printk("\n=== COMPLETE ===\n");
    return 0;
}