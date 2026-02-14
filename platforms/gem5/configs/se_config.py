#!/usr/bin/env python3
"""
gem5 Syscall Emulation (SE) Configuration for RISC-V
=====================================================

This script configures gem5 to run a RISC-V ELF binary in Syscall Emulation
mode. In SE mode, gem5 loads the binary and emulates Linux syscalls:
  - write() for console output
  - exit() / exit_group() for clean shutdown

SE mode is simpler and faster than FS mode, useful for quick functional
testing without modeling full hardware.

Usage:
  gem5.opt se_config.py --cmd=path/to/app.elf [options]

Options:
  --cpu-type    CPU model (default: AtomicSimpleCPU)
  --num-cpus    Number of CPU cores (default: 1)
  --mem-size    Memory size (default: 128MB)
  --l1d-size    L1 data cache size (default: 32kB)
  --l1i-size    L1 instruction cache size (default: 32kB)
  --l2-size     L2 cache size (default: 256kB)
  --max-ticks   Maximum simulation ticks (default: 10000000000)
  --cmd         Path to the binary (required)

Note:
  The binary must be compiled for gem5 SE mode (GEM5_MODE_SE defined).
  It should use semihosting (SYS_WRITE0/SYS_EXIT) for I/O.
"""

import argparse
import os
import sys

import m5
from m5.objects import *
from m5.util import addToPath

# =============================================================================
# Parse Command-Line Arguments
# =============================================================================

parser = argparse.ArgumentParser(
    description="gem5 RISC-V Syscall Emulation (SE) configuration"
)
parser.add_argument(
    "--cpu-type",
    default="AtomicSimpleCPU",
    choices=["AtomicSimpleCPU", "TimingSimpleCPU", "MinorCPU", "DerivO3CPU"],
    help="CPU model to use (default: AtomicSimpleCPU)",
)
parser.add_argument(
    "--num-cpus", type=int, default=1, help="Number of CPU cores (default: 1)"
)
parser.add_argument(
    "--mem-size", default="128MB", help="Memory size (default: 128MB)"
)
parser.add_argument(
    "--l1d-size", default="32kB", help="L1 data cache size (default: 32kB)"
)
parser.add_argument(
    "--l1i-size", default="32kB", help="L1 instruction cache size (default: 32kB)"
)
parser.add_argument(
    "--l2-size", default="256kB", help="L2 cache size (default: 256kB)"
)
parser.add_argument(
    "--max-ticks",
    type=int,
    default=10000000000,
    help="Maximum simulation ticks (default: 10B)",
)
parser.add_argument(
    "--cmd", required=True, help="Path to the binary"
)
parser.add_argument(
    "--options", default="", help="Arguments to pass to the binary"
)

args = parser.parse_args()

# Validate binary exists
if not os.path.exists(args.cmd):
    print(f"Error: Binary not found: {args.cmd}", file=sys.stderr)
    sys.exit(1)

# =============================================================================
# System Configuration
# =============================================================================

system = System()

# Clock domain
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "1GHz"
system.clk_domain.voltage_domain = VoltageDomain()

# Memory mode depends on CPU type
if "Atomic" in args.cpu_type:
    system.mem_mode = "atomic"
else:
    system.mem_mode = "timing"

# Memory range: bare-metal binary expects 0x80000000 (RISC-V standard)
system.mem_ranges = [AddrRange(start=0x80000000, size=args.mem_size)]

# =============================================================================
# CPU Configuration
# =============================================================================

cpu_class = getattr(m5.objects, args.cpu_type)
system.cpu = [cpu_class() for _ in range(args.num_cpus)]

for i, cpu in enumerate(system.cpu):
    cpu.cpu_id = i
    cpu.createInterruptController()
    cpu.createThreads()

# =============================================================================
# Memory Hierarchy
# =============================================================================

system.membus = SystemXBar()

if "Atomic" in args.cpu_type:
    # Atomic CPU: direct connection to memory bus
    for cpu in system.cpu:
        cpu.icache_port = system.membus.cpu_side_ports
        cpu.dcache_port = system.membus.cpu_side_ports
else:
    # Timing CPUs: add L1 caches + L2
    system.l2bus = L2XBar()

    for cpu in system.cpu:
        # L1 instruction cache
        cpu.icache = Cache(
            size=args.l1i_size,
            assoc=2,
            tag_latency=1,
            data_latency=1,
            response_latency=1,
            mshrs=4,
            tgts_per_mshr=8,
        )
        cpu.icache.cpu_side = cpu.icache_port
        cpu.icache.mem_side = system.l2bus.cpu_side_ports

        # L1 data cache
        cpu.dcache = Cache(
            size=args.l1d_size,
            assoc=4,
            tag_latency=2,
            data_latency=2,
            response_latency=2,
            mshrs=16,
            tgts_per_mshr=8,
        )
        cpu.dcache.cpu_side = cpu.dcache_port
        cpu.dcache.mem_side = system.l2bus.cpu_side_ports

    # L2 cache
    system.l2cache = Cache(
        size=args.l2_size,
        assoc=8,
        tag_latency=10,
        data_latency=10,
        response_latency=10,
        mshrs=20,
        tgts_per_mshr=12,
    )
    system.l2cache.cpu_side = system.l2bus.mem_side_ports
    system.l2cache.mem_side = system.membus.cpu_side_ports

# =============================================================================
# Memory Controller
# =============================================================================

system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR4_2400_16x4()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.mem_side_ports

# System port
system.system_port = system.membus.cpu_side_ports

# =============================================================================
# Workload (Bare-Metal in M-mode + Semihosting)
# =============================================================================
# RiscvBareMetal loads the binary and sets up M-mode; RiscvSemihosting handles
# the ebreak trap sequence for SYS_WRITE0/SYS_EXIT.
#
# NOTE: gem5 SE mode has a structural incompatibility: BaseCPU requires
# cpu.workload.size() == numThreads (Process objects), but Process requires
# system.workload to be SEWorkload. RiscvBareMetal is Workload, not SEWorkload.
# Using Process+SEWorkload runs in user mode (mhartid inaccessible). This config
# uses RiscvBareMetal alone; it will fail the CPU workload check. Disable the
# gem5 SE CI job until gem5 supports bare-metal SE (e.g. workload check bypass).
# =============================================================================

system.workload = RiscvBareMetal()
system.workload.bootloader = args.cmd
system.workload.semihosting = RiscvSemihosting()

# =============================================================================
# Root and Instantiation
# =============================================================================

root = Root(full_system=False, system=system)
m5.instantiate()

# =============================================================================
# Simulation
# =============================================================================

print(f"[gem5] Starting SE simulation:")
print(f"[gem5]   Binary:    {args.cmd}")
print(f"[gem5]   CPU Type:  {args.cpu_type}")
print(f"[gem5]   Num CPUs:  {args.num_cpus}")
print(f"[gem5]   Mem Size:  {args.mem_size}")
print(f"[gem5]   Max Ticks: {args.max_ticks}")
print()

exit_event = m5.simulate(args.max_ticks)

print()
print(f"[gem5] Simulation finished at tick {m5.curTick()}")
print(f"[gem5] Exit cause: {exit_event.getCause()}")
print(f"[gem5] Exit code:  {exit_event.getCode()}")

# Print basic stats summary
stats_file = os.path.join(m5.options.outdir, "stats.txt")
if os.path.exists(stats_file):
    print(f"[gem5] Stats file: {stats_file}")

sys.exit(exit_event.getCode())
