# TreeCast: Multi-Party Key Establishment Protocol for IoT Devices

> **Abstract:** Secure communication in the Internet of Things (IoT) requires lightweight protocols that scale across unicast, multicast, and broadcast settings. Existing solutions typically depend on centralized gateways, which introduce single points of failure and scalability limitations. We propose \textit{TreeCast}, a distributed group key establishment protocol that organizes devices into a binary tree of hashed Diffie–Hellman secrets, which naturally unifies unicast, multicast, and broadcast in a single scalable hierarchical structure. This design yields logarithmic rekey costs, efficient subtree derivation with efficient device addition and revocation in large and dynamic IoT environment and strong security properties including authentication, partial forward secrecy, and post-compromise recovery. We present a detailed security analysis and to demonstrate practical viability, implement TreeCast on an Arm Cortex-M33 using only software cryptography, without any hardware accelerators. Our measurements show that TreeCast performs key establishment and rekeying within realistic IoT timing and energy budgets, achieving sub-millisecond tree updates and low memory footprint. These results establish TreeCast as a lightweight, scalable, and deployable solution for secure communication in next-generation IoT networks.

This repository contains artefacts and proof-of-concept code of our paper `TreeCast`, which will appear at ICICS 2026.

The code is provided to reviewers for **full reproducibility** and **independent verification** of our implementation claims. It targets the **nRF5340 DK (Arm Cortex-M33)** running the **nRF Connect SDK (NCS)** and measures protocol costs using **pure software cryptography** (no hardware accelerators).

---

## Quick start

Install the official Nordic toolchain (the evaluation was done with these versions):

- **nRF Connect SDK v3.0.0**
- **Zephyr OS v4.0.99**

Make sure you have the target board connected to the computer, and then run.

```bash
# 1. Initialise west in a separate workspace (if you do not already use NCS)
west init -m https://github.com/nrfconnect/sdk-nrf --mr v3.0.0 ncs
cd ncs
west update

# 2. Export Zephyr environment (one shell per terminal)
west zephyr-export
source zephyr/zephyr-env.sh

# 3. After installation, ensure the following commands work in a fresh shell
west --version
cmake --version
ninja --version
arm-none-eabi-gcc --version

# 4. Download TreeCast in a sibling directory (the zip file)
cd ..
cd treecast
west build -b nrf5340dk/nrf5340/cpuapp --pristine
west flash
```

You should also have the `zephyr-env` script set up (for example, via source `~/ncs/v3.0.0/zephyr/zephyr-env.sh` or the equivalent `path` on your system). If you see any `nrfjprog` or `nrftool` errors, please ensure that the Nordic command line tools are installed and the board is listed by `nrfjprog --ids`. If you have a different Nordic board, replace the build line with your board name and version.

To view the results, open a serial terminal to view the benchmark output. You can use any terminal emulator (PuTTY, minicom) or the built-in `west` command:

```bash
# 115200 baud, 8N1 via the West command-line
west espressif monitor  # (or your preferred serial tool)

# macOS
screen /dev/tty.usbmodem* 115200

# Linux
screen /dev/ttyACM0 115200

# Windows
Use PuTTY (Serial connection type, line: COM3, speed: 115200 baud)

# Native VS Code
Use UART viewer (VCOM1)
```

## Benchmark results

Below is the exact benchmark output captured from our nRF5340 DK running this firmware. These numbers correspond to a Cortex M33 at 64 MHz using pure software cryptography (no vendor accelerators).

```text
=================================================
TreeCast benchmarking suite
NUM_DEVICES = 8, NUM_ITERS = 100, MSG_SIZE = 64 bytes
Timer clock (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) = 32768 Hz
Assumed CPU core clock                          = 64000000 Hz
=================================================

[1] Primitive operations (NUM_ITERS=100, MSG_SIZE=64 bytes)
[KGEN] EC key generation (secp256r1 sk,pk)
    - per-op:        751 timer cycles   ( 1466796 CPU cycles)  =   22.918 ms
    - over 100:    75100 timer cycles   (146679687 CPU cycles) = 2291.870 ms

[HASH] SHA-256 over 64-byte message
    - per-op:          3 timer cycles   (   5859 CPU cycles)   =    0.091 ms
    - over 100:      300 timer cycles   (  585937 CPU cycles)  =    9.155 ms

[HDH ] P-256 ECDH + SHA-256 (pairwise secret)
    - per-op:        752 timer cycles   ( 1468750 CPU cycles)  =   22.949 ms
    - over 100:    75200 timer cycles   (146875000 CPU cycles) = 2294.921 ms

[ENC ] AES-CTR encryption of a 64-byte message
    - per-op:          4 timer cycles   (   7812 CPU cycles)   =    0.122 ms
    - over 100:      400 timer cycles   (  781250 CPU cycles)  =   12.207 ms

[1b] Timer vs CPU cycles (per operation, NUM_ITERS=100)
Assumed CPU clock: 64000000 Hz
System timer clock: 32768 Hz (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)

| Op   | Timer cycles | CPU cycles (approx) |
|------|--------------|---------------------|
| KGEN |          751 |            1466796  |
| HASH |            3 |               5859  |
| HDH  |          752 |            1468750  |
| ENC  |            4 |               7812  |

[2] Communication scenarios (NUM_DEVICES=8, NUM_ITERS=100)

[UNICAST] key derivation (HDH only)
    - per-op:        752 timer cycles   ( 1468750 CPU cycles)  =   22.949 ms
    - over 100:    75200 timer cycles   (146875000 CPU cycles) = 2294.921 ms

[UNICAST] payload encryption (AES-CTR, 64 B)
    - per-op:          4 timer cycles   (   7812 CPU cycles)   =    0.122 ms
    - over 100:      400 timer cycles   (  781250 CPU cycles)  =   12.207 ms

[UNICAST] total (HDH + AES-CTR)
    - per-op:        756 timer cycles   ( 1476562 CPU cycles)  =   23.071 ms
    - over 100:    75600 timer cycles   (147656250 CPU cycles) = 2307.128 ms

[MULTICAST] key retrieval for subtree [0..3]
    - per-op:          0 timer cycles   ( < 1 tick )
    - over 100:        0 timer cycles

[MULTICAST] payload encryption (AES-CTR, 64 B)
    - per-op:          4 timer cycles   (   7812 CPU cycles)   =    0.122 ms
    - over 100:      400 timer cycles   (  781250 CPU cycles)  =   12.207 ms

[MULTICAST] total (retrieval + AES-CTR)
    - per-op:          4 timer cycles   (   7812 CPU cycles)   =    0.122 ms
    - over 100:      400 timer cycles   (  781250 CPU cycles)  =   12.207 ms

[BROADCAST] root key retrieval (global)
    - per-op:          0 timer cycles   ( < 1 tick )
    - over 100:        0 timer cycles

[BROADCAST] payload encryption (AES-CTR, 64 B)
    - per-op:          4 timer cycles   (   7812 CPU cycles)   =    0.122 ms
    - over 100:      400 timer cycles   (  781250 CPU cycles)  =   12.207 ms

[BROADCAST] total (retrieval + AES-CTR)
    - per-op:          4 timer cycles   (   7812 CPU cycles)   =    0.122 ms
    - over 100:      400 timer cycles   (  781250 CPU cycles)  =   12.207 ms

[3] Dynamic membership (path rekey from leaf 0 to root)

[REKEY] path rekey cost (N = 8 devices)
    - per-op:         12 timer cycles   (  23437 CPU cycles)   =    0.366 ms
    - over 100:     1200 timer cycles   ( 2343750 CPU cycles)  =   36.621 ms

[4] Storage cost (RAM)

[STRUCT] treecast_ctx_t (N=8) :  5188 bytes
[STRUCT] tc_node_t             :    33 bytes
[STRUCT] tc_ec_keypair_t       :    96 bytes

[5] Scalability projection (theoretical model)

Model: Path_Cost(N) = ceil(log2 N) * Hash_Cost
Hash_Cost (from [HASH]): 3 timer cycles per hash

| Devices (N) | Height | Rekey (cycles) | Rekey (ms) |
|------------|--------|----------------|-----------|
|         10 |      4 |             12 |   0.366 ms |
|        100 |      7 |             21 |   0.640 ms |
|       1000 |     10 |             30 |   0.915 ms |
|      10000 |     14 |             42 |   1.281 ms |
|     100000 |     17 |             51 |   1.556 ms |

== COMPLETE ==
```

## Directory Structure

```bash
.
├── build/                           # Build artifacts (generated)
├── CMakeLists.txt                   # CMake build configuration
├── nrf5340dk_nrf5340_cpuapp.overlay # Device tree overlay
├── prj.conf                         # Kconfig configuration (Crypto & System)
└── src/
    ├── crypto_backend.c             # Wrapper for crypto operations (ECC, AES)
    ├── crypto_backend.h             # Backend API definitions
    ├── main.c                       # Benchmarking suite entry point
    ├── soft_aes.c                   # Standalone AES implementation (CBC/CTR)
    ├── treecast.c                   # Core TreeCast protocol logic
    └── treecast.h                   # Protocol API definitions
```

## Citation

Full version of the paper can be found [here](https://cosicdatabase.esat.kuleuven.be/backend/publications/files/conferencepaper/). If you find this work useful, please consider citing the paper:

```bibtex
@inproceedings{TreeCast,
  title     = {{TreeCast: Multi-Party Key Establishment Protocol for IoT Devices}},
  author    = {Banerjee, Supriyo and Duttagupta, Sayon},
  booktitle = {Information and Communications Security (ICICS)},
  year      = {2026},
}
```

## Acknowledgments

This work was supported in part by CyberSecurity Research Flanders with reference number VR20192203, and by the European Union’s Horizon Research and Innovation program under grant agreement No.\ 101119747 (TELEMETRY).
