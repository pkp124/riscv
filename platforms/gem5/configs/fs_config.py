#!/usr/bin/env python3
"""
gem5 Full System (FS) Configuration for RISC-V Bare-Metal
==========================================================

This script configures gem5 to run a bare-metal RISC-V ELF in Full System
mode. It models a minimal RISC-V system with:
  - Configurable CPU model (Atomic, Timing, Minor, O3)
  - Configurable number of CPUs (1-8)
  - L1 I/D caches + optional L2 cache
  - DDR memory at 0x80000000
  - Uart8250 at 0x10000000 for console output
  - CLINT at 0x02000000 for timer/IPI

Usage:
  gem5.opt fs_config.py --cmd=path/to/app.elf [options]

Options:
  --cpu-type    CPU model: AtomicSimpleCPU, TimingSimpleCPU, MinorCPU,
                DerivO3CPU (default: AtomicSimpleCPU)
  --num-cpus    Number of CPU cores (default: 1)
  --mem-size    Memory size (default: 128MB)
  --l1d-size    L1 data cache size (default: 32kB)
  --l1i-size    L1 instruction cache size (default: 32kB)
  --l2-size     L2 cache size (default: 256kB)
  --max-ticks   Maximum simulation ticks (default: 10000000000)
  --cmd         Path to the bare-metal ELF binary (required)
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
    description="gem5 RISC-V Full System bare-metal configuration"
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
    "--cmd", "--kernel", required=True, help="Path to bare-metal ELF binary"
)

args = parser.parse_args()

# Validate kernel/binary exists
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

# Memory range at standard RISC-V base address
system.mem_ranges = [AddrRange(start=0x80000000, size=args.mem_size)]

# =============================================================================
# CPU Configuration
# =============================================================================

cpu_class = getattr(m5.objects, args.cpu_type)
system.cpu = [cpu_class() for _ in range(args.num_cpus)]

# Configure each CPU
for i, cpu in enumerate(system.cpu):
    cpu.cpu_id = i
    cpu.createInterruptController()

# =============================================================================
# Memory Hierarchy
# =============================================================================

# Create memory bus
system.membus = SystemXBar()

if "Atomic" in args.cpu_type:
    # Atomic CPU: connect directly to memory bus (no caches needed)
    for cpu in system.cpu:
        cpu.icache_port = system.membus.cpu_side_ports
        cpu.dcache_port = system.membus.cpu_side_ports
else:
    # Timing/Minor/O3 CPU: add L1 caches
    # L2 bus for cache-to-memory interconnect
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

# =============================================================================
# System Port
# =============================================================================

system.system_port = system.membus.cpu_side_ports

# =============================================================================
# UART (Uart8250 at 0x10000000)
# =============================================================================

# gem5 provides Uart8250 which is NS16550A compatible
system.uart = Uart8250()
system.uart.pio_addr = 0x10000000
system.uart.pio = system.membus.mem_side_ports

# Terminal for UART output
system.terminal = Terminal()
system.uart.terminal = system.terminal

# =============================================================================
# Workload (Bare-Metal)
# =============================================================================

system.workload = RiscvBareMetal()
system.workload.bootloader = args.cmd

# =============================================================================
# Root and Instantiation
# =============================================================================

root = Root(full_system=True, system=system)
m5.instantiate()

# =============================================================================
# Simulation
# =============================================================================

print(f"[gem5] Starting FS simulation:")
print(f"[gem5]   Binary:    {args.cmd}")
print(f"[gem5]   CPU Type:  {args.cpu_type}")
print(f"[gem5]   Num CPUs:  {args.num_cpus}")
print(f"[gem5]   Mem Size:  {args.mem_size}")
print(f"[gem5]   Max Ticks: {args.max_ticks}")
if "Atomic" not in args.cpu_type:
    print(f"[gem5]   L1d Size:  {args.l1d_size}")
    print(f"[gem5]   L1i Size:  {args.l1i_size}")
    print(f"[gem5]   L2 Size:   {args.l2_size}")
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
