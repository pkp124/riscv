# Cry Label

A one-screen Android app for a parent standing next to a crying infant:

1. Record a short cry segment (16 kHz mono WAV, max 20 seconds).
2. Immediately label **what happened afterward**.
3. Export a zip of labelled clips once you have a few dozen to a few hundred.

The labels are the outcomes Dunstan Baby Language (DBL) claims to map onto distinct sounds. That is the scientifically useful ground truth: not “I think I heard Neh,” but “the baby then fed / slept / burped / …”. After collecting enough labelled episodes, run the first-look script to see whether those classes actually occupy different regions of a simple acoustic feature space — before investing in a model.

## DBL mapping

| Label stored | What happened afterward | DBL sound |
|---|---|---|
| `fed` | Baby fed | Neh |
| `slept` | Baby slept or settled | Owh |
| `discomfort` | Diaper, clothes, temperature, position | Heh |
| `gas` | Gas / tummy relief | Eairh |
| `burped` | Baby burped | Eh |
| `other` | Something else | — |
| `unsure` | Not sure yet | — |

Clips never leave the phone unless you share the export zip.

## Build the Android app

Open **`cry-labeler/android`** in Android Studio (do not open the RISC-V repo root). Android Studio will write `local.properties` with your SDK path.

```bash
cd cry-labeler/android
./gradlew :app:assembleDebug
```

Install `app/build/outputs/apk/debug/app-debug.apk` on a phone. Grant the microphone permission on first record.

Night-time UI is always dark. Tap **RECORD**, stop when the burst ends, then tap the outcome. Optional notes and infant age (in Settings) are copied onto each episode.

## Dataset format

Each episode is two files with the same id:

```text
20260819T110500Z_ab12.wav
20260819T110500Z_ab12.json
```

```json
{
  "schema_version": 1,
  "id": "20260819T110500Z_ab12",
  "recorded_at_epoch_ms": 1787137500000,
  "duration_ms": 4250,
  "sample_rate_hz": 16000,
  "label": "fed",
  "dbl_sound": "Neh",
  "labeled_at_epoch_ms": 1787137508000,
  "notes": "",
  "infant_age_weeks": 8
}
```

WAV is uncompressed 16-bit PCM, 16 kHz, mono — the usual starting point for cry/speech features. The export zip also contains `manifest.csv`, `audio/`, and `meta/`.

## First look at class separation

```bash
python3 -m pip install -r cry-labeler/tools/requirements.txt
python3 cry-labeler/tools/analyze_separability.py /path/to/unzipped/export
```

The script reports per-class counts, mean features, centroid distances, and a silhouette score. It is deliberately dumb (energy, zero-crossings, a coarse spectrum). If the DBL outcome classes do not separate even here *and* a spectrogram-by-eye look agrees, a neural net is unlikely to be the bottleneck — the labels or the phenomenon are.

A few hundred labelled episodes is the intended stopping point for this prototype.

## Tests

Core dataset format (JDK only, no Android SDK):

```bash
cd cry-labeler/android
./gradlew :core:test -c settings-core.gradle.kts
```

Separability script:

```bash
python3 cry-labeler/tests/test_analyze_separability.py
```

The `:app` module is included for Android Studio; it needs a local Android SDK to assemble.
