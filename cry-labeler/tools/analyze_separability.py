#!/usr/bin/env python3
"""First-look check: do labelled cry outcomes occupy different acoustic regions?

This is not a production classifier. It extracts a handful of classic DSP
features from 16 kHz mono WAVs, then reports class centroids, pairwise
distances, and a simple silhouette score. After a few hundred labelled
episodes, that is enough to decide whether Dunstan-style outcome classes
are worth a heavier model.
"""

from __future__ import annotations

import argparse
import json
import sys
import wave
from collections import defaultdict
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

import numpy as np

FEATURE_NAMES = (
    "duration_s",
    "rms",
    "zcr",
    "spectral_centroid_hz",
    "peak_hz",
    "rolloff_hz",
    "flatness",
)

DBL_SOUNDS = {
    "fed": "Neh",
    "slept": "Owh",
    "discomfort": "Heh",
    "gas": "Eairh",
    "burped": "Eh",
    "other": "",
    "unsure": "",
}


def find_json_files(root: Path) -> List[Path]:
    json_files = sorted(root.rglob("*.json"))
    return [path for path in json_files if path.name != "manifest.json"]


def find_wav_for(meta_path: Path, episode_id: str) -> Optional[Path]:
    candidates = [
        meta_path.with_suffix(".wav"),
        meta_path.parent / f"{episode_id}.wav",
        meta_path.parent.parent / "audio" / f"{episode_id}.wav",
        meta_path.parent / "audio" / f"{episode_id}.wav",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return None


def load_wav_mono(path: Path) -> Tuple[np.ndarray, int]:
    with wave.open(str(path), "rb") as handle:
        channels = handle.getnchannels()
        sample_width = handle.getsampwidth()
        sample_rate = handle.getframerate()
        frames = handle.readframes(handle.getnframes())
    if sample_width != 2:
        raise ValueError(f"{path} is not 16-bit PCM")
    samples = np.frombuffer(frames, dtype="<i2").astype(np.float32)
    if channels > 1:
        samples = samples.reshape(-1, channels).mean(axis=1)
    return samples / 32768.0, sample_rate


def extract_features(samples: np.ndarray, sample_rate: int) -> Dict[str, float]:
    if samples.size == 0:
        return {name: 0.0 for name in FEATURE_NAMES}
    duration_s = samples.size / float(sample_rate)
    rms = float(np.sqrt(np.mean(np.square(samples))) + 1e-12)
    zcr = float(np.mean(np.abs(np.diff(np.signbit(samples)))))
    windowed = samples * np.hanning(samples.size)
    spectrum = np.abs(np.fft.rfft(windowed)) + 1e-12
    freqs = np.fft.rfftfreq(samples.size, d=1.0 / sample_rate)
    power = spectrum ** 2
    centroid = float(np.sum(freqs * spectrum) / np.sum(spectrum))
    peak_hz = float(freqs[int(np.argmax(spectrum))])
    cumulative = np.cumsum(power)
    rolloff_idx = int(np.searchsorted(cumulative, 0.85 * cumulative[-1]))
    rolloff_hz = float(freqs[min(rolloff_idx, freqs.size - 1)])
    log_mean = float(np.mean(np.log(spectrum)))
    mean = float(np.mean(spectrum))
    flatness = float(np.exp(log_mean) / mean)
    return {
        "duration_s": duration_s,
        "rms": rms,
        "zcr": zcr,
        "spectral_centroid_hz": centroid,
        "peak_hz": peak_hz,
        "rolloff_hz": rolloff_hz,
        "flatness": flatness,
    }


def load_dataset(root: Path) -> List[dict]:
    rows: List[dict] = []
    for meta_path in find_json_files(root):
        payload = json.loads(meta_path.read_text(encoding="utf-8"))
        episode_id = payload.get("id") or meta_path.stem
        wav_path = find_wav_for(meta_path, episode_id)
        if wav_path is None:
            continue
        samples, sample_rate = load_wav_mono(wav_path)
        features = extract_features(samples, sample_rate)
        label = payload.get("label", "unsure")
        rows.append(
            {
                "id": episode_id,
                "label": label,
                "dbl_sound": payload.get("dbl_sound") or DBL_SOUNDS.get(label, ""),
                "path": str(wav_path),
                **features,
            }
        )
    return rows


def zscore(matrix: np.ndarray) -> np.ndarray:
    mean = matrix.mean(axis=0)
    std = matrix.std(axis=0)
    std = np.where(std < 1e-9, 1.0, std)
    return (matrix - mean) / std


def class_matrices(rows: Sequence[dict]) -> Dict[str, np.ndarray]:
    grouped: Dict[str, List[List[float]]] = defaultdict(list)
    for row in rows:
        grouped[row["label"]].append([row[name] for name in FEATURE_NAMES])
    return {label: np.asarray(values, dtype=np.float64) for label, values in grouped.items()}


def pairwise_centroid_distances(standardized: Dict[str, np.ndarray]) -> Dict[Tuple[str, str], float]:
    centroids = {label: matrix.mean(axis=0) for label, matrix in standardized.items()}
    labels = sorted(centroids)
    distances = {}
    for i, left in enumerate(labels):
        for right in labels[i + 1 :]:
            distances[(left, right)] = float(np.linalg.norm(centroids[left] - centroids[right]))
    return distances


def silhouette(standardized: Dict[str, np.ndarray]) -> Optional[float]:
    usable = {label: matrix for label, matrix in standardized.items() if matrix.shape[0] >= 2}
    if len(usable) < 2:
        return None
    scores: List[float] = []
    for label, matrix in usable.items():
        others = {other: values for other, values in usable.items() if other != label}
        for index in range(matrix.shape[0]):
            point = matrix[index]
            rest = np.delete(matrix, index, axis=0)
            a = float(np.mean(np.linalg.norm(rest - point, axis=1)))
            b = min(
                float(np.mean(np.linalg.norm(values - point, axis=1)))
                for values in others.values()
            )
            denom = max(a, b, 1e-9)
            scores.append((b - a) / denom)
    return float(np.mean(scores)) if scores else None


def interpret(score: Optional[float], n_rows: int) -> str:
    if n_rows < 30:
        size_note = "Sample is still small; treat this as a sanity check, not a verdict."
    elif n_rows < 200:
        size_note = "A few dozen to a couple hundred clips: enough to see a trend, not enough to freeze a model."
    else:
        size_note = "Dataset size is in the intended first-look range."
    if score is None:
        return f"{size_note} Need at least two classes with two clips each."
    if score >= 0.45:
        verdict = "Classes occupy noticeably different regions in this feature space."
    elif score >= 0.20:
        verdict = "Some separation, but overlap is still large. Collect more before a heavy model."
    elif score >= 0.05:
        verdict = "Weak structure. DBL-style categories may not be linearly obvious in raw audio."
    else:
        verdict = "Almost no separation on these features. Do not invest in a model until labels or features change."
    return f"{verdict} {size_note}"


def format_report(rows: Sequence[dict]) -> str:
    if not rows:
        return "No labelled WAV+JSON pairs found."
    labels = sorted({row["label"] for row in rows})
    matrix = np.asarray([[row[name] for name in FEATURE_NAMES] for row in rows], dtype=np.float64)
    scaled = zscore(matrix)
    grouped_std: Dict[str, List[np.ndarray]] = defaultdict(list)
    for row, vector in zip(rows, scaled):
        grouped_std[row["label"]].append(vector)
    standardized = {label: np.vstack(values) for label, values in grouped_std.items()}
    distances = pairwise_centroid_distances(standardized)
    score = silhouette(standardized)

    lines = [
        "Cry Label — DBL outcome separability (first look)",
        f"Episodes: {len(rows)}",
        "",
        "Counts by outcome (DBL sound in parentheses):",
    ]
    counts = defaultdict(int)
    for row in rows:
        counts[row["label"]] += 1
    for label in labels:
        sound = DBL_SOUNDS.get(label, "")
        suffix = f" ({sound})" if sound else ""
        lines.append(f"  {label}{suffix}: {counts[label]}")

    lines += ["", "Per-class mean features:"]
    raw_groups = class_matrices(rows)
    for label in labels:
        means = raw_groups[label].mean(axis=0)
        pretty = ", ".join(f"{name}={value:.3f}" for name, value in zip(FEATURE_NAMES, means))
        lines.append(f"  {label}: {pretty}")

    lines += ["", "Pairwise centroid distance (z-scored features):"]
    if not distances:
        lines.append("  (need two classes)")
    else:
        for (left, right), distance in sorted(distances.items(), key=lambda item: -item[1]):
            lines.append(f"  {left} vs {right}: {distance:.3f}")

    score_text = "n/a" if score is None else f"{score:.3f}"
    lines += [
        "",
        f"Mean silhouette (higher = more separated, range -1 to 1): {score_text}",
        interpret(score, len(rows)),
        "",
        "This uses duration, energy, zero-crossing rate, and a coarse spectrum.",
        "It will not catch subtle phonetic cues; it *will* tell you if the classes",
        "are obviously different before you spend time on a neural net.",
    ]
    return "\n".join(lines)


def parse_args(argv: Optional[Sequence[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "dataset",
        nargs="?",
        help="Folder of exported episodes (WAV+JSON, or audio/ + meta/ from the zip)",
    )
    return parser.parse_args(argv)


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = parse_args(argv)
    if not args.dataset:
        print("Usage: analyze_separability.py /path/to/export", file=sys.stderr)
        return 2
    root = Path(args.dataset).expanduser().resolve()
    if not root.exists():
        print(f"Dataset path not found: {root}", file=sys.stderr)
        return 2
    report = format_report(load_dataset(root))
    print(report)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
