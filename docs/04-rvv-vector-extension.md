# Design Proposal 04: RISC-V Vector Extension (RVV 1.0) Learning Plan

## 1. Overview

The RISC-V Vector Extension (RVV) is a scalable vector ISA extension that provides data-level parallelism. Unlike fixed-width SIMD (e.g., ARM NEON, x86 SSE/AVX), RVV uses a **vector-length agnostic (VLA)** programming model where the hardware vector length (VLEN) is discovered at runtime.

This document describes our plan to learn and demonstrate RVV through progressive workloads in the bare-metal application.

---

## 2. RVV Key Concepts

### 2.1 Vector Registers

| Register | Count | Width | Description |
|----------|-------|-------|-------------|
| `v0`-`v31` | 32 | VLEN bits | Vector data registers |
| `v0` | 1 | VLEN bits | Also serves as mask register (predication) |

VLEN is implementation-defined (e.g., 128, 256, 512, 1024 bits). Software must query it at runtime.

### 2.2 Key CSRs

| CSR | Name | Description |
|-----|------|-------------|
| `vlenb` | Vector byte length | VLEN/8 (read-only) |
| `vtype` | Vector type | Element width, grouping, tail/mask policy |
| `vl` | Vector length | Current number of active elements |
| `vstart` | Vector start | First element to process (usually 0) |

### 2.3 SEW and LMUL

- **SEW** (Selected Element Width): 8, 16, 32, or 64 bits per element
- **LMUL** (Length Multiplier): Groups 1, 2, 4, or 8 vector registers together
  - LMUL=1: 1 register, VLEN/SEW elements
  - LMUL=2: 2 registers, 2*VLEN/SEW elements
  - LMUL=4: 4 registers, 4*VLEN/SEW elements
  - LMUL=8: 8 registers, 8*VLEN/SEW elements
  - LMUL=1/2, 1/4, 1/8: Fractional grouping (fewer elements)

### 2.4 The vsetvli Instruction

The fundamental RVV instruction that configures vector processing:

```asm
vsetvli rd, rs1, vtypei
```

- `rs1`: Requested application vector length (AVL)
- `rd`: Actual vector length granted (VL)
- `vtypei`: Encoded vtype (SEW, LMUL, tail/mask policy)

**Example:**
```asm
# Configure for 32-bit elements, LMUL=1
# Request processing 'n' elements
vsetvli t0, a0, e32, m1, ta, ma
# t0 = actual elements that will be processed per iteration
# a0 = total elements remaining
```

### 2.5 Vector-Length Agnostic Programming

The key pattern for RVV programming:

```c
// Process n elements of a[] + b[] -> c[]
void vec_add_f32(float *a, float *b, float *c, size_t n) {
    while (n > 0) {
        size_t vl = __riscv_vsetvl_e32m1(n);  // Set VL
        vfloat32m1_t va = __riscv_vle32_v_f32m1(a, vl);  // Load a
        vfloat32m1_t vb = __riscv_vle32_v_f32m1(b, vl);  // Load b
        vfloat32m1_t vc = __riscv_vfadd_vv_f32m1(va, vb, vl);  // Add
        __riscv_vse32_v_f32m1(c, vc, vl);  // Store c
        a += vl; b += vl; c += vl; n -= vl;
    }
}
```

This code works correctly regardless of VLEN (128, 256, 512, etc.).

---

## 3. RVV Instruction Categories

### 3.1 Configuration Instructions
| Instruction | Description |
|------------|-------------|
| `vsetvli` | Set VL with immediate vtype |
| `vsetivli` | Set VL with immediate AVL and vtype |
| `vsetvl` | Set VL with register vtype |

### 3.2 Load/Store Instructions
| Instruction | Description |
|------------|-------------|
| `vle{8,16,32,64}.v` | Unit-stride load |
| `vse{8,16,32,64}.v` | Unit-stride store |
| `vlse{8,16,32,64}.v` | Strided load |
| `vsse{8,16,32,64}.v` | Strided store |
| `vluxei{8,16,32,64}.v` | Indexed (gather) load |
| `vsuxei{8,16,32,64}.v` | Indexed (scatter) store |
| `vlm.v` / `vsm.v` | Mask load/store |

### 3.3 Integer Arithmetic
| Instruction | Description |
|------------|-------------|
| `vadd.vv/vx/vi` | Vector add |
| `vsub.vv/vx` | Vector subtract |
| `vmul.vv/vx` | Vector multiply |
| `vdiv.vv/vx` | Vector divide |
| `vrem.vv/vx` | Vector remainder |
| `vand/vor/vxor` | Bitwise operations |
| `vsll/vsrl/vsra` | Shifts |
| `vmin/vmax` | Min/max |

### 3.4 Fixed-Point Arithmetic
| Instruction | Description |
|------------|-------------|
| `vsadd/vssub` | Saturating add/subtract |
| `vsmul` | Signed saturating multiply |
| `vssrl/vssra` | Scaling shift |

### 3.5 Floating-Point Arithmetic
| Instruction | Description |
|------------|-------------|
| `vfadd.vv/vf` | FP vector add |
| `vfsub.vv/vf` | FP vector subtract |
| `vfmul.vv/vf` | FP vector multiply |
| `vfdiv.vv/vf` | FP vector divide |
| `vfmacc/vfnmacc` | FP fused multiply-accumulate |
| `vfmadd/vfnmadd` | FP fused multiply-add |
| `vfsqrt.v` | FP square root |

### 3.6 Reduction Operations
| Instruction | Description |
|------------|-------------|
| `vredsum` | Sum reduction |
| `vredmax/vredmin` | Max/min reduction |
| `vfredosum` | Ordered FP sum reduction |
| `vfredusum` | Unordered FP sum reduction |

### 3.7 Mask Operations
| Instruction | Description |
|------------|-------------|
| `vmseq/vmsne/vmslt/...` | Compare and set mask |
| `vmandn/vmand/vmor/...` | Mask-mask operations |
| `vcpop.m` | Count population of mask |
| `vfirst.m` | Find first set in mask |

### 3.8 Permutation Operations
| Instruction | Description |
|------------|-------------|
| `vslideup/vslidedown` | Slide elements up/down |
| `vrgather.vv/vi` | Gather using indices |
| `vcompress.vm` | Compress using mask |
| `vmv.x.s` / `vmv.s.x` | Move scalar <-> vector |

---

## 4. Progressive RVV Workloads

We will implement RVV workloads in order of increasing complexity:

### Level 1: Basic Vector Operations (Introduction)

**4.1 Vector Addition (Integer)**
```c
// c[i] = a[i] + b[i] for i = 0..n-1
// Demonstrates: vsetvli, vle32, vadd, vse32
void rvv_vec_add_i32(int32_t *a, int32_t *b, int32_t *c, size_t n);
```

**4.2 Vector Scalar Multiply**
```c
// c[i] = a[i] * scalar for i = 0..n-1
// Demonstrates: vmul.vx (vector-scalar operation)
void rvv_vec_scale_i32(int32_t *a, int32_t scalar, int32_t *c, size_t n);
```

**4.3 Vector Memcpy**
```c
// dst[i] = src[i] for i = 0..n-1 (byte-granular)
// Demonstrates: vle8, vse8, LMUL=8 for maximum throughput
void rvv_memcpy(void *dst, const void *src, size_t n);
```

### Level 2: Floating-Point Vector Operations

**4.4 Vector Addition (FP32)**
```c
// Demonstrates: vfadd.vv, floating-point vector operations
void rvv_vec_add_f32(float *a, float *b, float *c, size_t n);
```

**4.5 Vector Dot Product**
```c
// result = sum(a[i] * b[i]) for i = 0..n-1
// Demonstrates: vfmul.vv, vfredosum (reduction)
float rvv_dot_product_f32(float *a, float *b, size_t n);
```

### Level 3: Complex Operations

**4.6 Vector Matrix Multiply**
```c
// C[m][n] = A[m][k] * B[k][n]
// Demonstrates: strided load, LMUL grouping, register tiling
void rvv_matmul_f32(float *A, float *B, float *C, int m, int n, int k);
```

**4.7 Vector Saxpy (BLAS Level 1)**
```c
// y[i] = a * x[i] + y[i]
// Demonstrates: vfmacc (fused multiply-accumulate)
void rvv_saxpy(float a, float *x, float *y, size_t n);
```

### Level 4: Advanced Patterns

**4.8 Conditional Operations with Masks**
```c
// c[i] = (a[i] > threshold) ? a[i] : 0
// Demonstrates: mask operations, predicated execution
void rvv_threshold_f32(float *a, float threshold, float *c, size_t n);
```

**4.9 Histogram / Indexed Operations**
```c
// Demonstrates: indexed (gather/scatter) load/store
void rvv_histogram(uint32_t *data, uint32_t *bins, size_t n, uint32_t num_bins);
```

**4.10 Strided Access Pattern**
```c
// Extract every Nth element (e.g., extract channel from interleaved data)
// Demonstrates: strided load (vlse32)
void rvv_deinterleave(float *interleaved, float *channel, size_t n, int stride);
```

---

## 5. Implementation Approach

### 5.1 Dual Implementation: Intrinsics + Inline Assembly

Each workload will be implemented in TWO ways:

#### A. GCC RVV Intrinsics (Portable, Recommended)

```c
#include <riscv_vector.h>

void rvv_vec_add_f32(float *a, float *b, float *c, size_t n) {
    while (n > 0) {
        size_t vl = __riscv_vsetvl_e32m1(n);
        vfloat32m1_t va = __riscv_vle32_v_f32m1(a, vl);
        vfloat32m1_t vb = __riscv_vle32_v_f32m1(b, vl);
        vfloat32m1_t vc = __riscv_vfadd_vv_f32m1(va, vb, vl);
        __riscv_vse32_v_f32m1(c, vc, vl);
        a += vl; b += vl; c += vl; n -= vl;
    }
}
```

#### B. Inline Assembly (Educational)

```c
void rvv_vec_add_f32_asm(float *a, float *b, float *c, size_t n) {
    size_t vl;
    __asm__ volatile(
        "1:\n"
        "  vsetvli %[vl], %[n], e32, m1, ta, ma\n"
        "  vle32.v v0, (%[a])\n"
        "  vle32.v v1, (%[b])\n"
        "  vfadd.vv v2, v0, v1\n"
        "  vse32.v v2, (%[c])\n"
        "  slli t0, %[vl], 2\n"    // vl * 4 bytes
        "  add %[a], %[a], t0\n"
        "  add %[b], %[b], t0\n"
        "  add %[c], %[c], t0\n"
        "  sub %[n], %[n], %[vl]\n"
        "  bnez %[n], 1b\n"
        : [vl] "=&r"(vl), [a] "+r"(a), [b] "+r"(b),
          [c] "+r"(c), [n] "+r"(n)
        :
        : "t0", "v0", "v1", "v2", "memory"
    );
}
```

### 5.2 Scalar Reference Implementation

Every RVV workload will have a scalar reference for correctness verification:

```c
void scalar_vec_add_f32(float *a, float *b, float *c, size_t n) {
    for (size_t i = 0; i < n; i++) {
        c[i] = a[i] + b[i];
    }
}
```

### 5.3 Performance Comparison Framework

```c
typedef struct {
    const char *name;
    uint64_t scalar_cycles;
    uint64_t vector_cycles;
    float speedup;
    int passed;  /* correctness check */
} rvv_benchmark_result_t;

void rvv_benchmark(const char *name,
                   void (*scalar_fn)(void),
                   void (*vector_fn)(void),
                   int (*verify_fn)(void)) {
    uint64_t start, end;

    /* Scalar */
    start = read_mcycle();
    scalar_fn();
    end = read_mcycle();
    uint64_t scalar_cycles = end - start;

    /* Vector */
    start = read_mcycle();
    vector_fn();
    end = read_mcycle();
    uint64_t vector_cycles = end - start;

    /* Verify */
    int pass = verify_fn();

    /* Report */
    platform_puts("[RVV] ");
    platform_puts(name);
    platform_puts(pass ? " ... PASS" : " ... FAIL");
    platform_puts("\n  Scalar: "); platform_put_dec(scalar_cycles);
    platform_puts(" cycles\n  Vector: "); platform_put_dec(vector_cycles);
    platform_puts(" cycles\n  Speedup: ");
    /* print speedup ratio */
}
```

---

## 6. VLEN Discovery and Adaptation

```c
/* Runtime VLEN detection */
static inline uint64_t rvv_get_vlenb(void) {
    uint64_t vlenb;
    __asm__ volatile("csrr %0, vlenb" : "=r"(vlenb));
    return vlenb;  /* VLEN in bytes */
}

static inline uint64_t rvv_get_vlen(void) {
    return rvv_get_vlenb() * 8;  /* VLEN in bits */
}

/* Check if RVV is available (via misa) */
static inline int rvv_available(void) {
    uint64_t misa;
    __asm__ volatile("csrr %0, misa" : "=r"(misa));
    return (misa & (1UL << ('V' - 'A'))) != 0;
}

void rvv_print_info(void) {
    if (!rvv_available()) {
        platform_puts("RVV: Not available\n");
        return;
    }
    platform_puts("RVV: Available\n");
    platform_puts("  VLEN = ");
    platform_put_dec(rvv_get_vlen());
    platform_puts(" bits\n");
    platform_puts("  VLENB = ");
    platform_put_dec(rvv_get_vlenb());
    platform_puts(" bytes\n");

    /* Test different SEW/LMUL combinations */
    size_t vl;

    vl = __riscv_vsetvl_e8m1(1000);
    platform_puts("  VL(e8,m1)  = "); platform_put_dec(vl); platform_puts("\n");

    vl = __riscv_vsetvl_e32m1(1000);
    platform_puts("  VL(e32,m1) = "); platform_put_dec(vl); platform_puts("\n");

    vl = __riscv_vsetvl_e32m4(1000);
    platform_puts("  VL(e32,m4) = "); platform_put_dec(vl); platform_puts("\n");

    vl = __riscv_vsetvl_e64m1(1000);
    platform_puts("  VL(e64,m1) = "); platform_put_dec(vl); platform_puts("\n");
}
```

---

## 7. LMUL Exploration

One key learning goal is understanding how LMUL affects performance:

```c
/*
 * Same vector add with different LMUL settings.
 * LMUL=1: processes VLEN/32 elements per iteration (fewer, more registers free)
 * LMUL=2: processes 2*VLEN/32 elements per iteration
 * LMUL=4: processes 4*VLEN/32 elements per iteration
 * LMUL=8: processes 8*VLEN/32 elements per iteration (more, fewer registers free)
 *
 * Trade-off: Higher LMUL = more elements per iteration but fewer register groups
 */

void rvv_vec_add_f32_m1(float *a, float *b, float *c, size_t n);  // LMUL=1
void rvv_vec_add_f32_m2(float *a, float *b, float *c, size_t n);  // LMUL=2
void rvv_vec_add_f32_m4(float *a, float *b, float *c, size_t n);  // LMUL=4
void rvv_vec_add_f32_m8(float *a, float *b, float *c, size_t n);  // LMUL=8

void rvv_lmul_comparison(void) {
    /* Run all four and compare cycles */
    /* Expect: higher LMUL = fewer loop iterations = possibly faster */
}
```

---

## 8. RVV on Different Simulators

### 8.1 QEMU

```bash
# Enable RVV with specific VLEN
qemu-system-riscv64 -machine virt \
    -cpu rv64,v=true,vlen=128 \       # VLEN=128
    -nographic -bios none -kernel app.elf

# Test with different VLENs
qemu-system-riscv64 -cpu rv64,v=true,vlen=256 ...
qemu-system-riscv64 -cpu rv64,v=true,vlen=512 ...
qemu-system-riscv64 -cpu rv64,v=true,vlen=1024 ...
```

QEMU supports VLEN from 64 to 65536 bits (powers of 2).

### 8.2 Spike

```bash
# Enable RVV
spike --isa=rv64gcv --varch=vlen:256,elen:64 app.elf

# Different VLENs
spike --isa=rv64gcv --varch=vlen:128,elen:64 app.elf
spike --isa=rv64gcv --varch=vlen:512,elen:64 app.elf
spike --isa=rv64gcv --varch=vlen:1024,elen:64 app.elf
```

### 8.3 gem5

gem5's RVV support is experimental. Status:
- Basic RVV instructions: Partially supported
- Full RVV 1.0: Community patches exist but may not be upstream
- Recommendation: Use QEMU/Spike for RVV development, gem5 for non-RVV performance analysis

---

## 9. Compiler Support

### 9.1 GCC

GCC 12+ supports RVV intrinsics via `<riscv_vector.h>`.

```bash
# Compile with RVV
riscv64-unknown-elf-gcc -march=rv64gcv -mabi=lp64d \
    -O2 -o app.o -c app.c
```

### 9.2 Clang/LLVM

Clang 16+ has good RVV intrinsic support. Generally ahead of GCC for vector auto-vectorization.

```bash
clang --target=riscv64 -march=rv64gcv -mabi=lp64d \
    -O2 -o app.o -c app.c
```

### 9.3 Auto-Vectorization

We can also explore compiler auto-vectorization:

```c
/* This loop should auto-vectorize with -O2 -march=rv64gcv */
void auto_vec_add(float *a, float *b, float *c, int n) {
    for (int i = 0; i < n; i++) {
        c[i] = a[i] + b[i];
    }
}
```

Compile with:
```bash
riscv64-unknown-elf-gcc -march=rv64gcv -O2 -ftree-vectorize \
    -fopt-info-vec-optimized  # Reports which loops were vectorized
```

---

## 10. RVV Learning Exercises (Proposed)

| # | Exercise | Concepts Learned |
|---|----------|-----------------|
| 1 | Print VLEN and VL for various SEW/LMUL | vsetvli, CSR access |
| 2 | Integer vector add (inline asm) | Basic RVV asm syntax |
| 3 | Integer vector add (intrinsics) | RVV intrinsics API |
| 4 | FP vector add with different LMUL | LMUL trade-offs |
| 5 | Dot product with reduction | vfredosum |
| 6 | Masked operations | Predication, vmseq |
| 7 | Strided load/store | vlse32, memory patterns |
| 8 | Matrix multiply | Register tiling, complex loop |
| 9 | Auto-vectorization analysis | Compiler optimization |
| 10 | Scalar vs vector benchmark | Performance analysis |

---

## 11. Open Questions

1. **Which GCC version minimum for RVV intrinsics?** GCC 12 has basic support, GCC 13+ is more complete. We should target GCC 13+.
2. **Should we include Clang/LLVM as an alternative compiler?** Clang often produces better vector code.
3. **Should RVV workloads use only intrinsics, only inline asm, or both?** Recommendation: both, with intrinsics as "production" and asm as "learning".
4. **What VLEN should we test against in CI?** Recommendation: 128 and 256 (most common real implementations).

---

*Next: See [05-build-system.md](05-build-system.md) for build system and toolchain design.*
