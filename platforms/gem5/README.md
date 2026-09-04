# gem5 AMP Full-System Platform

This directory contains **gem5 Full System** configurations for RISC-V, including
asymmetric multiprocessing (AMP).

AMP is implemented **on gem5**, not SystemC. gem5 already models:

- Heterogeneous CPU clusters (different models and ISA strings per cluster)
- Multi-level caches (per-core L1I/L1D, per-cluster L2)
- A system interconnect (`SystemXBar` + `IOXBar`)
- DRAM (DDR4) plus faster scratchpad / shared SRAM
- UART, CLINT, and PLIC via the HiFive platform

A SystemC + Spike virtual platform remains documented in
[`docs/07-systemc-amp-platform.md`](../../docs/07-systemc-amp-platform.md) as a
future ISS-integration path. It is not required to run AMP simulations today.

## YAML platform description

Edit a file under `platforms/` to choose:

| Field | Controls |
|-------|----------|
| `clusters[].harts` | How many AMP harts in that cluster |
| `clusters[].isa` | `rv64gc` (scalar) or `rv64gcv` (vector) |
| `clusters[].cpu_type` | `AtomicSimpleCPU`, `TimingSimpleCPU`, `MinorCPU`, `DerivO3CPU` |
| `clusters[].l1i_size` / `l1d_size` / `l2_size` | Cache hierarchy |
| `clusters[].scratchpad` | Private fast SRAM (base, size, latency) |
| `shared_memory` | Shared SRAM mailbox region + DDR4 DRAM |
| `peripherals` | UART, CLINT, PLIC (wired through HiFive) |

Shipped examples:

- `platforms/amp_2cluster.yaml` — 2× TimingSimpleCPU (scalar) + 2× MinorCPU (RVV ISA)
- `platforms/amp_scalar_rvv.yaml` — 1 scalar hart + 1 RVV hart
- `platforms/amp_4cluster.yaml` — four mixed clusters

Validate without gem5:

```bash
python3 platforms/gem5/configs/amp_platform.py --dump \
    platforms/gem5/platforms/amp_2cluster.yaml
```

## Build and run

```bash
cmake --preset gem5-amp
cmake --build build/gem5-amp

# Requires gem5.opt (see scripts/setup-simulators.sh / the devcontainer)
gem5.opt platforms/gem5/configs/amp_config.py \
    --platform=platforms/gem5/platforms/amp_2cluster.yaml \
    --cmd=build/gem5-amp/bin/app
```

`--fast` forces `AtomicSimpleCPU` on every cluster (no caches; quicker smoke tests).

Firmware is a **single ELF**. Hart 0 is the cluster-0 leader; other harts
identify their cluster from `mhartid` ranges generated from the YAML file.

## Tests

```bash
ctest --test-dir build/gem5-amp -L phase8 --output-on-failure
```

YAML schema tests run even when gem5 is not installed. Boot / mailbox / full
AMP CTests run only when `gem5.opt` is on `PATH`.
