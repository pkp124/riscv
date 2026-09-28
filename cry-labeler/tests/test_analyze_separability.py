#!/usr/bin/env python3
"""Synthetic-dataset checks for the first-look separability script."""

from __future__ import annotations

import json
import math
import struct
import sys
import tempfile
import unittest
import wave
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import analyze_separability as analyze  # noqa: E402


def write_sine(path: Path, freq_hz: float, seconds: float = 0.4, sample_rate: int = 16_000) -> None:
    n_samples = int(sample_rate * seconds)
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "w") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(sample_rate)
        frames = bytearray()
        for index in range(n_samples):
            value = int(12000 * math.sin(2.0 * math.pi * freq_hz * index / sample_rate))
            frames.extend(struct.pack("<h", value))
        handle.writeframes(bytes(frames))


def write_noise(path: Path, seed: int, seconds: float = 0.4, sample_rate: int = 16_000) -> None:
    n_samples = int(sample_rate * seconds)
    path.parent.mkdir(parents=True, exist_ok=True)
    state = seed
    with wave.open(str(path), "w") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(sample_rate)
        frames = bytearray()
        for _ in range(n_samples):
            state = (1103515245 * state + 12345) & 0x7FFFFFFF
            value = (state % 8000) - 4000
            frames.extend(struct.pack("<h", value))
        handle.writeframes(bytes(frames))


def write_episode(root: Path, episode_id: str, label: str, wav_builder) -> None:
    wav_path = root / "audio" / f"{episode_id}.wav"
    meta_path = root / "meta" / f"{episode_id}.json"
    wav_builder(wav_path)
    meta_path.parent.mkdir(parents=True, exist_ok=True)
    meta_path.write_text(
        json.dumps(
            {
                "schema_version": 1,
                "id": episode_id,
                "recorded_at_epoch_ms": 1,
                "duration_ms": 400,
                "sample_rate_hz": 16000,
                "label": label,
                "dbl_sound": analyze.DBL_SOUNDS[label],
                "labeled_at_epoch_ms": 2,
                "notes": "",
                "infant_age_weeks": 8,
            }
        ),
        encoding="utf-8",
    )


class AnalyzeSeparabilityTest(unittest.TestCase):
    def test_tones_of_different_pitch_separate(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for index in range(8):
                write_episode(
                    root,
                    f"fed_{index}",
                    "fed",
                    lambda path, i=index: write_sine(path, 350 + i),
                )
                write_episode(
                    root,
                    f"slept_{index}",
                    "slept",
                    lambda path, i=index: write_sine(path, 1400 + i),
                )
            rows = analyze.load_dataset(root)
            self.assertEqual(16, len(rows))
            matrix = np.asarray([[row[name] for name in analyze.FEATURE_NAMES] for row in rows], dtype=np.float64)
            scaled = analyze.zscore(matrix)
            buckets = {"fed": [], "slept": []}
            for row, vector in zip(rows, scaled):
                buckets[row["label"]].append(vector)
            grouped = {key: np.vstack(values) for key, values in buckets.items()}
            score = analyze.silhouette(grouped)
            self.assertIsNotNone(score)
            self.assertGreater(score, 0.45)
            report = analyze.format_report(rows)
            self.assertIn("fed", report)
            self.assertIn("Neh", report)

    def test_same_noise_does_not_separate(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for index in range(8):
                write_episode(
                    root,
                    f"fed_{index}",
                    "fed",
                    lambda path, i=index: write_noise(path, seed=1000 + i),
                )
                write_episode(
                    root,
                    f"slept_{index}",
                    "slept",
                    lambda path, i=index: write_noise(path, seed=2000 + i),
                )
            rows = analyze.load_dataset(root)
            matrix = np.asarray([[row[name] for name in analyze.FEATURE_NAMES] for row in rows])
            scaled = analyze.zscore(matrix)
            buckets = {"fed": [], "slept": []}
            for row, vector in zip(rows, scaled):
                buckets[row["label"]].append(vector)
            grouped = {key: np.vstack(values) for key, values in buckets.items()}
            score = analyze.silhouette(grouped)
            self.assertIsNotNone(score)
            self.assertLess(score, 0.25)

    def test_json_round_trip_keys_match_android_export(self) -> None:
        sample = {
            "schema_version": 1,
            "id": "abc",
            "recorded_at_epoch_ms": 10,
            "duration_ms": 20,
            "sample_rate_hz": 16000,
            "label": "gas",
            "dbl_sound": "Eairh",
            "labeled_at_epoch_ms": 30,
            "notes": "gas, then sleep",
            "infant_age_weeks": 6,
        }
        encoded = json.dumps(sample)
        decoded = json.loads(encoded)
        self.assertEqual("gas", decoded["label"])
        self.assertEqual("Eairh", decoded["dbl_sound"])


if __name__ == "__main__":
    unittest.main()
