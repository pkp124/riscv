#!/usr/bin/env python3
"""
AMP platform description loader.

Reads a YAML (or JSON) AMP topology and validates:
  - cluster count, per-cluster hart counts and ISA
  - unique, non-overlapping memory and scratchpad ranges
  - required DRAM region at 0x80000000

This module does not import gem5 (m5), so it can be unit-tested and used
from CMake at configure time.

Usage:
  python3 amp_platform.py --validate platforms/gem5/platforms/amp_2cluster.yaml
  python3 amp_platform.py --dump platforms/gem5/platforms/amp_2cluster.yaml
  python3 amp_platform.py --cmake platforms/gem5/platforms/amp_2cluster.yaml
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


CPU_TYPES = (
    "AtomicSimpleCPU",
    "TimingSimpleCPU",
    "MinorCPU",
    "DerivO3CPU",
)

DRAM_BASE = 0x80000000
MAX_CLUSTERS = 8
MAX_HARTS_PER_CLUSTER = 8
MAX_TOTAL_HARTS = 16


def parse_int(value: Any) -> int:
    """Parse int from YAML/JSON (int, decimal string, or 0x hex string)."""
    if isinstance(value, bool):
        raise ValueError(f"boolean is not an integer: {value}")
    if isinstance(value, int):
        return value
    if isinstance(value, float) and value.is_integer():
        return int(value)
    if isinstance(value, str):
        text = value.strip().replace("_", "")
        return int(text, 0)
    raise ValueError(f"cannot parse integer from {value!r}")


def parse_size_bytes(value: Any) -> int:
    """Parse a size like 128, 0x10000, 32kB, 128MB."""
    if isinstance(value, int):
        return value
    if isinstance(value, float) and value.is_integer():
        return int(value)
    if not isinstance(value, str):
        raise ValueError(f"cannot parse size from {value!r}")
    text = value.strip().replace("_", "")
    units = (
        ("kib", 1024),
        ("kb", 1024),
        ("mib", 1024 * 1024),
        ("mb", 1024 * 1024),
        ("gib", 1024 * 1024 * 1024),
        ("gb", 1024 * 1024 * 1024),
        ("b", 1),
    )
    lower = text.lower()
    for suffix, mul in units:
        if lower.endswith(suffix):
            return parse_int(text[: -len(suffix)]) * mul
    return parse_int(text)


def isa_has_vector(isa: str) -> bool:
    return "v" in isa.lower().replace("rv64", "", 1).replace("rv32", "", 1)


def _parse_simple_yaml(text: str) -> Any:
    """Minimal YAML subset parser for AMP platform files (indent maps/lists)."""
    lines: list[tuple[int, str]] = []
    for raw in text.splitlines():
        stripped = raw.split("#", 1)[0].rstrip()
        if not stripped.strip():
            continue
        indent = len(raw) - len(raw.lstrip(" "))
        lines.append((indent, stripped.strip()))

    def parse_scalar(token: str) -> Any:
        if token in ("null", "~", ""):
            return None
        if token in ("true", "True", "yes"):
            return True
        if token in ("false", "False", "no"):
            return False
        if len(token) >= 2 and token[0] == token[-1] and token[0] in "\"'":
            return token[1:-1]
        try:
            return parse_int(token)
        except ValueError:
            return token

    def parse_block(index: int, indent: int) -> tuple[Any, int]:
        if index >= len(lines):
            return None, index
        _, content = lines[index]
        if content.startswith("- "):
            items: list[Any] = []
            while index < len(lines) and lines[index][0] == indent and lines[index][1].startswith(
                "- "
            ):
                item_indent, item = lines[index]
                payload = item[2:].strip()
                index += 1
                if payload == "" or (":" in payload and not payload.startswith("http")):
                    child_indent = indent + 2
                    if payload and ":" in payload:
                        key, val = payload.split(":", 1)
                        key = key.strip()
                        val = val.strip()
                        node: dict[str, Any] = {}
                        if val:
                            node[key] = parse_scalar(val)
                        if index < len(lines) and lines[index][0] >= child_indent:
                            nested, index = parse_block(index, lines[index][0])
                            if isinstance(nested, dict):
                                node.update(nested)
                            elif key not in node:
                                node[key] = nested
                        items.append(node)
                    else:
                        nested, index = parse_block(index, child_indent)
                        items.append(nested)
                else:
                    items.append(parse_scalar(payload))
            return items, index

        mapping: dict[str, Any] = {}
        while index < len(lines) and lines[index][0] == indent:
            _, content = lines[index]
            if content.startswith("- "):
                break
            if ":" not in content:
                raise ValueError(f"invalid YAML line: {content}")
            key, val = content.split(":", 1)
            key = key.strip()
            val = val.strip()
            index += 1
            if val:
                mapping[key] = parse_scalar(val)
            else:
                if index < len(lines) and lines[index][0] > indent:
                    nested, index = parse_block(index, lines[index][0])
                    mapping[key] = nested
                else:
                    mapping[key] = None
        return mapping, index

    if not lines:
        return {}
    value, _ = parse_block(0, lines[0][0])
    return value


def load_raw(path: str | Path) -> dict[str, Any]:
    path = Path(path)
    text = path.read_text(encoding="utf-8")
    if path.suffix.lower() == ".json":
        data = json.loads(text)
    else:
        try:
            import yaml  # type: ignore

            data = yaml.safe_load(text)
        except ImportError:
            data = _parse_simple_yaml(text)
    if not isinstance(data, dict):
        raise ValueError("platform file must contain a mapping")
    if "platform" in data and isinstance(data["platform"], dict):
        return data["platform"]
    return data


class AmpError(ValueError):
    pass


def _require(mapping: dict[str, Any], key: str, ctx: str) -> Any:
    if key not in mapping or mapping[key] is None:
        raise AmpError(f"{ctx}: missing required field '{key}'")
    return mapping[key]


def normalize(raw: dict[str, Any]) -> dict[str, Any]:
    name = str(_require(raw, "name", "platform"))
    simulation = raw.get("simulation") or {}
    clusters_in = _require(raw, "clusters", "platform")
    if not isinstance(clusters_in, list) or not clusters_in:
        raise AmpError("platform.clusters must be a non-empty list")
    if len(clusters_in) > MAX_CLUSTERS:
        raise AmpError(f"at most {MAX_CLUSTERS} clusters are supported")

    clusters: list[dict[str, Any]] = []
    hart_base = 0
    ranges: list[tuple[str, int, int]] = []

    for index, cluster in enumerate(clusters_in):
        if not isinstance(cluster, dict):
            raise AmpError(f"clusters[{index}] must be a mapping")
        ctx = f"clusters[{index}]"
        n_harts = parse_int(_require(cluster, "harts", ctx))
        if n_harts < 1 or n_harts > MAX_HARTS_PER_CLUSTER:
            raise AmpError(f"{ctx}.harts must be 1..{MAX_HARTS_PER_CLUSTER}")
        cpu_type = str(cluster.get("cpu_type", "TimingSimpleCPU"))
        if cpu_type not in CPU_TYPES:
            raise AmpError(f"{ctx}.cpu_type must be one of {CPU_TYPES}")
        isa = str(cluster.get("isa", "rv64gc")).lower()
        scratch_in = cluster.get("scratchpad") or {}
        scratch = None
        if scratch_in:
            sctx = f"{ctx}.scratchpad"
            sbase = parse_int(_require(scratch_in, "base", sctx))
            ssize = parse_size_bytes(_require(scratch_in, "size", sctx))
            scratch = {
                "name": str(scratch_in.get("name", f"scratch{index}")),
                "base": sbase,
                "size": ssize,
                "latency_ns": parse_int(scratch_in.get("latency_ns", 2)),
            }
            ranges.append((f"{ctx} scratchpad", sbase, ssize))

        clusters.append(
            {
                "name": str(cluster.get("name", f"cluster{index}")),
                "role": str(cluster.get("role", "application")),
                "isa": isa,
                "vector": isa_has_vector(isa),
                "vlen": parse_int(cluster.get("vlen", 256)) if isa_has_vector(isa) else 0,
                "harts": n_harts,
                "hartid_base": hart_base,
                "cpu_type": cpu_type,
                "l1i_size": str(cluster.get("l1i_size", "32kB")),
                "l1d_size": str(cluster.get("l1d_size", "32kB")),
                "l2_size": str(cluster.get("l2_size", "256kB")),
                "scratchpad": scratch,
            }
        )
        hart_base += n_harts

    if hart_base > MAX_TOTAL_HARTS:
        raise AmpError(f"total harts {hart_base} exceeds {MAX_TOTAL_HARTS}")

    shared_in = raw.get("shared_memory") or []
    if not isinstance(shared_in, list):
        raise AmpError("shared_memory must be a list")
    shared: list[dict[str, Any]] = []
    dram = None
    for index, region in enumerate(shared_in):
        if not isinstance(region, dict):
            raise AmpError(f"shared_memory[{index}] must be a mapping")
        ctx = f"shared_memory[{index}]"
        base = parse_int(_require(region, "base", ctx))
        size = parse_size_bytes(_require(region, "size", ctx))
        kind = str(region.get("kind", "sram")).lower()
        entry = {
            "name": str(region.get("name", f"mem{index}")),
            "base": base,
            "size": size,
            "latency_ns": parse_int(region.get("latency_ns", 50 if kind == "ddr4" else 5)),
            "kind": kind,
        }
        shared.append(entry)
        ranges.append((ctx, base, size))
        if kind in ("ddr4", "dram") or base == DRAM_BASE:
            dram = entry

    if dram is None:
        raise AmpError("shared_memory must include a DRAM region at 0x80000000")

    for i, (aname, abase, asize) in enumerate(ranges):
        aend = abase + asize
        for bname, bbase, bsize in ranges[i + 1 :]:
            bend = bbase + bsize
            if abase < bend and bbase < aend:
                raise AmpError(f"address overlap: {aname} and {bname}")

    peripherals = []
    for index, peri in enumerate(raw.get("peripherals") or []):
        if not isinstance(peri, dict):
            raise AmpError(f"peripherals[{index}] must be a mapping")
        peripherals.append(
            {
                "type": str(_require(peri, "type", f"peripherals[{index}]")),
                "name": str(peri.get("name", f"peri{index}")),
                "base": parse_int(peri.get("base", 0)),
            }
        )

    needs_timing = any(c["cpu_type"] != "AtomicSimpleCPU" for c in clusters)
    mem_mode = str(simulation.get("mem_mode", "timing" if needs_timing else "atomic"))
    if needs_timing and mem_mode == "atomic":
        mem_mode = "timing"

    return {
        "name": name,
        "description": str(raw.get("description", "")),
        "simulation": {
            "clock": str(simulation.get("clock", "1GHz")),
            "mem_mode": mem_mode,
            "max_ticks": parse_int(simulation.get("max_ticks", 10000000000)),
        },
        "clusters": clusters,
        "shared_memory": shared,
        "dram": dram,
        "peripherals": peripherals,
        "num_harts": hart_base,
        "num_clusters": len(clusters),
    }


def load_platform(path: str | Path) -> dict[str, Any]:
    return normalize(load_raw(path))


def cmake_defines(cfg: dict[str, Any]) -> dict[str, str]:
    defs = {
        "NUM_HARTS": str(cfg["num_harts"]),
        "AMP_NUM_CLUSTERS": str(cfg["num_clusters"]),
        "AMP_SHARED_SRAM_BASE": "0x0",
        "AMP_SHARED_SRAM_SIZE": "0",
        "AMP_DRAM_BASE": hex(cfg["dram"]["base"]),
        "AMP_DRAM_SIZE": hex(cfg["dram"]["size"]),
    }
    for region in cfg["shared_memory"]:
        if region["kind"] == "sram":
            defs["AMP_SHARED_SRAM_BASE"] = hex(region["base"])
            defs["AMP_SHARED_SRAM_SIZE"] = hex(region["size"])
            break
    for index, cluster in enumerate(cfg["clusters"]):
        defs[f"AMP_CLUSTER{index}_HARTS"] = str(cluster["harts"])
        defs[f"AMP_CLUSTER{index}_HARTID_BASE"] = str(cluster["hartid_base"])
        defs[f"AMP_CLUSTER{index}_VECTOR"] = "1" if cluster["vector"] else "0"
        scratch = cluster["scratchpad"]
        if scratch:
            defs[f"AMP_CLUSTER{index}_SCRATCH_BASE"] = hex(scratch["base"])
            defs[f"AMP_CLUSTER{index}_SCRATCH_SIZE"] = hex(scratch["size"])
        else:
            defs[f"AMP_CLUSTER{index}_SCRATCH_BASE"] = "0x0"
            defs[f"AMP_CLUSTER{index}_SCRATCH_SIZE"] = "0"
    for index in range(cfg["num_clusters"], MAX_CLUSTERS):
        defs[f"AMP_CLUSTER{index}_HARTS"] = "0"
        defs[f"AMP_CLUSTER{index}_HARTID_BASE"] = "0"
        defs[f"AMP_CLUSTER{index}_VECTOR"] = "0"
        defs[f"AMP_CLUSTER{index}_SCRATCH_BASE"] = "0x0"
        defs[f"AMP_CLUSTER{index}_SCRATCH_SIZE"] = "0"
    return defs


def dump_summary(cfg: dict[str, Any]) -> str:
    lines = [
        f"[AMP] platform: {cfg['name']}",
        f"[AMP] clusters: {cfg['num_clusters']}",
        f"[AMP] total harts: {cfg['num_harts']}",
        f"[AMP] clock: {cfg['simulation']['clock']}",
        f"[AMP] mem_mode: {cfg['simulation']['mem_mode']}",
    ]
    for cluster in cfg["clusters"]:
        isa = cluster["isa"]
        extra = f" vlen={cluster['vlen']}" if cluster["vector"] else ""
        lines.append(
            f"[AMP] {cluster['name']}: {cluster['cpu_type']} isa={isa}{extra} "
            f"harts={cluster['harts']} hartid_base={cluster['hartid_base']} "
            f"role={cluster['role']} L1I={cluster['l1i_size']} "
            f"L1D={cluster['l1d_size']} L2={cluster['l2_size']}"
        )
        scratch = cluster["scratchpad"]
        if scratch:
            lines.append(
                f"[AMP]   scratchpad {scratch['name']}: "
                f"{hex(scratch['base'])} size={hex(scratch['size'])} "
                f"latency={scratch['latency_ns']}ns"
            )
    for region in cfg["shared_memory"]:
        lines.append(
            f"[AMP] memory {region['name']}: {hex(region['base'])} "
            f"size={hex(region['size'])} kind={region['kind']} "
            f"latency={region['latency_ns']}ns"
        )
    for peri in cfg["peripherals"]:
        lines.append(f"[AMP] peripheral {peri['name']}: {peri['type']} @{hex(peri['base'])}")
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="AMP platform YAML loader")
    parser.add_argument("path", help="YAML or JSON platform description")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--validate", action="store_true", help="validate and exit")
    group.add_argument("--dump", action="store_true", help="print topology summary")
    group.add_argument("--cmake", action="store_true", help="print KEY=VALUE for CMake")
    group.add_argument("--json", action="store_true", help="print normalized JSON")
    args = parser.parse_args(argv)

    try:
        cfg = load_platform(args.path)
    except (OSError, AmpError, ValueError, json.JSONDecodeError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    if args.cmake:
        for key, value in cmake_defines(cfg).items():
            print(f"{key}={value}")
        return 0
    if args.json:
        print(json.dumps(cfg, indent=2, sort_keys=True))
        return 0

    print(dump_summary(cfg))
    if args.validate or args.dump:
        print("[AMP] YAML validation: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
