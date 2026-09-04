#!/usr/bin/env python3
"""
gem5 Full System AMP configuration for RISC-V.

Builds a heterogeneous multi-cluster system from a YAML platform
description (see platforms/gem5/platforms/*.yaml):

  - Per-cluster CPU model and hart count (scalar vs RVV ISA string)
  - Per-core L1I/L1D and per-cluster L2
  - System interconnect (SystemXBar + IOXBar)
  - Private scratchpads, shared SRAM, DDR4 DRAM
  - HiFive UART / CLINT / PLIC

Usage:
  gem5.opt amp_config.py --platform=amp_2cluster.yaml --cmd=app.elf
"""

from __future__ import annotations

import argparse
import os
import sys

import m5
from m5.objects import *

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
if THIS_DIR not in sys.path:
    sys.path.insert(0, THIS_DIR)

from amp_platform import dump_summary, load_platform  # noqa: E402


def parse_args():
    parser = argparse.ArgumentParser(
        description="gem5 RISC-V AMP full-system configuration"
    )
    parser.add_argument(
        "--platform",
        required=True,
        help="YAML/JSON AMP platform description",
    )
    parser.add_argument(
        "--cmd",
        "--kernel",
        required=True,
        help="Path to the bare-metal ELF binary",
    )
    parser.add_argument(
        "--cpu-type",
        default=None,
        choices=["AtomicSimpleCPU", "TimingSimpleCPU", "MinorCPU", "DerivO3CPU"],
        help="Override CPU model for every cluster",
    )
    parser.add_argument(
        "--max-ticks",
        type=int,
        default=None,
        help="Override maximum simulation ticks",
    )
    parser.add_argument(
        "--fast",
        action="store_true",
        help="Force AtomicSimpleCPU on all clusters (no caches)",
    )
    return parser.parse_args()


def make_cache(size, assoc, tag_latency, data_latency, response_latency, mshrs, tgts):
    return Cache(
        size=size,
        assoc=assoc,
        tag_latency=tag_latency,
        data_latency=data_latency,
        response_latency=response_latency,
        mshrs=mshrs,
        tgts_per_mshr=tgts,
    )


def apply_isa(cpu, isa_str, vlen):
    """Best-effort per-CPU ISA configuration across gem5 versions."""
    try:
        isa_list = list(cpu.isa)
    except Exception:
        return
    if not isa_list:
        return
    isa_obj = isa_list[0]
    for attr, value in (
        ("riscv_type", "RV64" if "rv64" in isa_str else "RV32"),
        ("enable_rvv", "v" in isa_str.replace("rv64", "").replace("rv32", "")),
    ):
        if hasattr(isa_obj, attr):
            try:
                setattr(isa_obj, attr, value)
            except Exception:
                pass
    if vlen and hasattr(isa_obj, "vlen"):
        try:
            isa_obj.vlen = vlen
        except Exception:
            pass


def build_system(cfg, binary, cpu_override, fast):
    system = RiscvSystem()
    system.clk_domain = SrcClockDomain()
    system.clk_domain.clock = cfg["simulation"]["clock"]
    system.clk_domain.voltage_domain = VoltageDomain()

    clusters = cfg["clusters"]
    if fast:
        for cluster in clusters:
            cluster["cpu_type"] = "AtomicSimpleCPU"
    if cpu_override:
        for cluster in clusters:
            cluster["cpu_type"] = cpu_override

    needs_timing = any(c["cpu_type"] != "AtomicSimpleCPU" for c in clusters)
    system.mem_mode = "timing" if needs_timing else "atomic"

    mem_ranges = []
    for cluster in clusters:
        scratch = cluster["scratchpad"]
        if scratch:
            mem_ranges.append(AddrRange(start=scratch["base"], size=scratch["size"]))
    for region in cfg["shared_memory"]:
        mem_ranges.append(AddrRange(start=region["base"], size=region["size"]))
    system.mem_ranges = mem_ranges

    system.platform = HiFive()
    system.platform.terminal.outfile = "stdoutput"
    system.platform.rtc = RiscvRTC(frequency=Frequency("100MHz"))
    system.platform.clint.int_pin = system.platform.rtc.int_pin

    system.iobus = IOXBar()
    system.membus = SystemXBar()
    system.system_port = system.membus.cpu_side_ports

    system.iobus.cpu_side_ports = system.platform.pci_host.up_request_port()
    system.iobus.mem_side_ports = system.platform.pci_host.up_response_port()
    system.platform.pci_bus.cpu_side_ports = (
        system.platform.pci_host.down_request_port()
    )
    system.platform.pci_bus.default = system.platform.pci_host.down_response_port()
    system.platform.pci_bus.config_error_port = (
        system.platform.pci_host.config_error.pio
    )

    system.bridge = Bridge(delay="50ns")
    system.bridge.mem_side_port = system.iobus.cpu_side_ports
    system.bridge.cpu_side_port = system.membus.mem_side_ports
    system.bridge.ranges = system.platform._off_chip_ranges()

    system.platform.attachOnChipIO(system.membus)
    system.platform.attachOffChipIO(system.iobus)
    system.platform.attachPlic()
    system.platform.setNumCores(cfg["num_harts"])

    cpus = []
    for cluster in clusters:
        cpu_class = getattr(m5.objects, cluster["cpu_type"])
        for local in range(cluster["harts"]):
            cpu = cpu_class()
            cpu.cpu_id = cluster["hartid_base"] + local
            cpu.createInterruptController()
            cpu.createThreads()
            apply_isa(cpu, cluster["isa"], cluster["vlen"])
            cpus.append(cpu)
    system.cpu = cpus

    uncacheable = [
        *system.platform._on_chip_ranges(),
        *system.platform._off_chip_ranges(),
    ]
    for cluster in clusters:
        scratch = cluster["scratchpad"]
        if scratch:
            uncacheable.append(AddrRange(start=scratch["base"], size=scratch["size"]))
    for region in cfg["shared_memory"]:
        if region["kind"] == "sram":
            uncacheable.append(AddrRange(start=region["base"], size=region["size"]))
    for cpu in system.cpu:
        cpu.mmu.pma_checker = PMAChecker(uncacheable=uncacheable)

    if needs_timing:
        for index, cluster in enumerate(clusters):
            l2bus = L2XBar()
            setattr(system, f"l2bus_{index}", l2bus)
            start = cluster["hartid_base"]
            end = start + cluster["harts"]
            for cpu in system.cpu[start:end]:
                cpu.icache = make_cache(cluster["l1i_size"], 2, 1, 1, 1, 4, 8)
                cpu.icache.cpu_side = cpu.icache_port
                cpu.icache.mem_side = l2bus.cpu_side_ports
                cpu.dcache = make_cache(cluster["l1d_size"], 4, 2, 2, 2, 16, 8)
                cpu.dcache.cpu_side = cpu.dcache_port
                cpu.dcache.mem_side = l2bus.cpu_side_ports
            l2 = make_cache(cluster["l2_size"], 8, 10, 10, 10, 20, 12)
            l2.cpu_side = l2bus.mem_side_ports
            l2.mem_side = system.membus.cpu_side_ports
            setattr(system, f"l2cache_{index}", l2)
    else:
        for cpu in system.cpu:
            cpu.icache_port = system.membus.cpu_side_ports
            cpu.dcache_port = system.membus.cpu_side_ports

    for index, cluster in enumerate(clusters):
        scratch = cluster["scratchpad"]
        if not scratch:
            continue
        mem = SimpleMemory(
            latency=f"{scratch['latency_ns']}ns",
            range=AddrRange(start=scratch["base"], size=scratch["size"]),
        )
        mem.port = system.membus.mem_side_ports
        setattr(system, f"scratch_{index}", mem)

    for index, region in enumerate(cfg["shared_memory"]):
        addr = AddrRange(start=region["base"], size=region["size"])
        if region["kind"] in ("ddr4", "dram"):
            ctrl = MemCtrl()
            ctrl.dram = DDR4_2400_16x4()
            ctrl.dram.range = addr
            ctrl.port = system.membus.mem_side_ports
            setattr(system, f"mem_ctrl_{index}", ctrl)
        else:
            mem = SimpleMemory(
                latency=f"{region['latency_ns']}ns",
                range=addr,
            )
            mem.port = system.membus.mem_side_ports
            setattr(system, f"shared_mem_{index}", mem)

    dram_ranges = [
        AddrRange(start=r["base"], size=r["size"])
        for r in cfg["shared_memory"]
        if r["kind"] in ("ddr4", "dram")
    ]
    system.iobridge = Bridge(delay="50ns", ranges=dram_ranges or system.mem_ranges)
    system.iobridge.cpu_side_port = system.iobus.mem_side_ports
    system.iobridge.mem_side_port = system.membus.cpu_side_ports

    system.workload = RiscvBareMetal()
    system.workload.bootloader = binary
    return system


def main():
    args = parse_args()
    if not os.path.exists(args.cmd):
        print(f"Error: Binary not found: {args.cmd}", file=sys.stderr)
        sys.exit(1)
    if not os.path.exists(args.platform):
        # Allow a basename relative to platforms/gem5/platforms
        candidate = os.path.join(
            os.path.dirname(THIS_DIR), "platforms", os.path.basename(args.platform)
        )
        if os.path.exists(candidate):
            args.platform = candidate
        else:
            print(f"Error: Platform file not found: {args.platform}", file=sys.stderr)
            sys.exit(1)

    cfg = load_platform(args.platform)
    max_ticks = args.max_ticks or cfg["simulation"]["max_ticks"]

    print(dump_summary(cfg))
    print()

    system = build_system(cfg, args.cmd, args.cpu_type, args.fast)
    root = Root(full_system=True, system=system)
    m5.instantiate()

    print("[gem5] Starting AMP FS simulation:")
    print(f"[gem5]   Binary:    {args.cmd}")
    print(f"[gem5]   Platform:  {args.platform}")
    print(f"[gem5]   Harts:     {cfg['num_harts']}")
    print(f"[gem5]   Max Ticks: {max_ticks}")
    print()

    exit_event = m5.simulate(max_ticks)

    print()
    print(f"[gem5] Simulation finished at tick {m5.curTick()}")
    print(f"[gem5] Exit cause: {exit_event.getCause()}")
    print(f"[gem5] Exit code:  {exit_event.getCode()}")
    stats_file = os.path.join(m5.options.outdir, "stats.txt")
    if os.path.exists(stats_file):
        print(f"[gem5] Stats file: {stats_file}")
    sys.exit(exit_event.getCode())


if __name__ == "__main__":
    main()
