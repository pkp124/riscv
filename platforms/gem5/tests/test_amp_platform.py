#!/usr/bin/env python3
"""Unit tests for AMP YAML platform loading (no gem5 required)."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

THIS_DIR = Path(__file__).resolve().parent
ROOT = THIS_DIR.parents[2]
sys.path.insert(0, str(THIS_DIR.parent / "configs"))

from amp_platform import AmpError, cmake_defines, load_platform  # noqa: E402

PLATFORMS = ROOT / "platforms" / "gem5" / "platforms"


class AmpPlatformTests(unittest.TestCase):
    def test_two_cluster_topology(self):
        cfg = load_platform(PLATFORMS / "amp_2cluster.yaml")
        self.assertEqual(cfg["name"], "amp_2cluster")
        self.assertEqual(cfg["num_clusters"], 2)
        self.assertEqual(cfg["num_harts"], 4)
        self.assertEqual(cfg["clusters"][0]["cpu_type"], "TimingSimpleCPU")
        self.assertEqual(cfg["clusters"][1]["cpu_type"], "MinorCPU")
        self.assertFalse(cfg["clusters"][0]["vector"])
        self.assertTrue(cfg["clusters"][1]["vector"])
        self.assertEqual(cfg["clusters"][1]["hartid_base"], 2)
        self.assertEqual(cfg["clusters"][1]["vlen"], 256)
        self.assertEqual(cfg["dram"]["base"], 0x80000000)

    def test_scalar_rvv_one_plus_one(self):
        cfg = load_platform(PLATFORMS / "amp_scalar_rvv.yaml")
        self.assertEqual(cfg["num_harts"], 2)
        self.assertEqual(cfg["clusters"][0]["harts"], 1)
        self.assertEqual(cfg["clusters"][1]["isa"], "rv64gcv")

    def test_four_cluster_mixed_cpus(self):
        cfg = load_platform(PLATFORMS / "amp_4cluster.yaml")
        self.assertEqual(cfg["num_clusters"], 4)
        self.assertEqual(cfg["num_harts"], 5)
        types = [c["cpu_type"] for c in cfg["clusters"]]
        self.assertEqual(
            types,
            ["TimingSimpleCPU", "TimingSimpleCPU", "MinorCPU", "AtomicSimpleCPU"],
        )

    def test_cmake_defines_match_topology(self):
        cfg = load_platform(PLATFORMS / "amp_2cluster.yaml")
        defs = cmake_defines(cfg)
        self.assertEqual(defs["NUM_HARTS"], "4")
        self.assertEqual(defs["AMP_CLUSTER0_HARTS"], "2")
        self.assertEqual(defs["AMP_CLUSTER1_VECTOR"], "1")
        self.assertEqual(defs["AMP_SHARED_SRAM_BASE"], "0x30000000")
        self.assertEqual(defs["AMP_CLUSTER0_SCRATCH_BASE"], "0x20000000")

    def test_overlap_is_rejected(self):
        text = """
platform:
  name: bad
  clusters:
    - name: c0
      isa: rv64gc
      harts: 1
      cpu_type: TimingSimpleCPU
      scratchpad:
        base: 0x20000000
        size: 0x20000
    - name: c1
      isa: rv64gc
      harts: 1
      cpu_type: TimingSimpleCPU
      scratchpad:
        base: 0x20010000
        size: 0x10000
  shared_memory:
    - name: dram
      base: 0x80000000
      size: 0x1000
      kind: ddr4
"""
        with tempfile.NamedTemporaryFile("w", suffix=".yaml", delete=False) as handle:
            handle.write(text)
            path = handle.name
        with self.assertRaises(AmpError):
            load_platform(path)

    def test_missing_dram_rejected(self):
        text = """
platform:
  name: bad
  clusters:
    - name: c0
      isa: rv64gc
      harts: 1
      cpu_type: TimingSimpleCPU
  shared_memory:
    - name: sram
      base: 0x30000000
      size: 0x1000
      kind: sram
"""
        with tempfile.NamedTemporaryFile("w", suffix=".yaml", delete=False) as handle:
            handle.write(text)
            path = handle.name
        with self.assertRaises(AmpError):
            load_platform(path)


if __name__ == "__main__":
    unittest.main()
