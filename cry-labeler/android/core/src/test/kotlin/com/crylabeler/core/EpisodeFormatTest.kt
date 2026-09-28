package com.crylabeler.core

import org.junit.jupiter.api.Assertions.assertArrayEquals
import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertNull
import org.junit.jupiter.api.Assertions.assertThrows
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import java.nio.ByteBuffer
import java.nio.ByteOrder

class OutcomeLabelTest {
    @Test
    fun dblOutcomesCoverTheFiveDunstanCategories() {
        val dbl = OutcomeLabel.entries.filter { it.dblSound != null }
        assertEquals(listOf("Neh", "Owh", "Heh", "Eairh", "Eh"), dbl.map { it.dblSound })
        assertEquals(
            listOf(
                OutcomeLabel.FED,
                OutcomeLabel.SLEPT,
                OutcomeLabel.DISCOMFORT,
                OutcomeLabel.GAS,
                OutcomeLabel.BURPED,
            ),
            dbl,
        )
    }

    @Test
    fun lookupIsStableForDatasetFilenames() {
        assertEquals(OutcomeLabel.FED, OutcomeLabel.fromId("fed"))
        assertEquals(OutcomeLabel.UNSURE, OutcomeLabel.fromId("unsure"))
        assertThrows(IllegalArgumentException::class.java) { OutcomeLabel.fromId("hungry") }
    }
}

class EpisodeJsonTest {
    private val sample =
        Episode(
            id = "20260819T110500Z_ab12",
            recordedAtEpochMs = 1_787_137_500_000L,
            durationMs = 4_250L,
            sampleRateHz = 16_000,
            label = OutcomeLabel.FED,
            labeledAtEpochMs = 1_787_137_508_000L,
            notes = "Took a full feed after ~3 minutes.",
            infantAgeWeeks = 8,
        )

    @Test
    fun roundTripPreservesFields() {
        val decoded = EpisodeJson.decode(EpisodeJson.encode(sample))
        assertEquals(sample, decoded)
        assertEquals("Neh", decoded.label.dblSound)
    }

    @Test
    fun notesWithQuotesAndNewlinesSurvive() {
        val messy = sample.copy(notes = "He said \"neh\"\nthen settled.")
        assertEquals(messy, EpisodeJson.decode(EpisodeJson.encode(messy)))
    }

    @Test
    fun missingOptionalFieldsDefaultSafely() {
        val json =
            """
            {
              "schema_version": 1,
              "id": "abc",
              "recorded_at_epoch_ms": 10,
              "duration_ms": 20,
              "sample_rate_hz": 16000,
              "label": "other",
              "labeled_at_epoch_ms": 30
            }
            """.trimIndent()
        val decoded = EpisodeJson.decode(json)
        assertEquals("", decoded.notes)
        assertNull(decoded.infantAgeWeeks)
        assertEquals(OutcomeLabel.OTHER, decoded.label)
    }

    @Test
    fun encodeWritesDatasetKeysInSnakeCase() {
        val json = EpisodeJson.encode(sample)
        assertTrue(json.contains("\"label\":\"fed\""))
        assertTrue(json.contains("\"dbl_sound\":\"Neh\""))
        assertTrue(json.contains("\"infant_age_weeks\":8"))
    }
}

class WavHeaderTest {
    @Test
    fun pcm16MonoHeaderMatchesCanonicalWavLayout() {
        val dataSize = 32000
        val header = WavHeader.pcm16Mono(dataBytes = dataSize, sampleRateHz = 16_000)
        assertEquals(44, header.size)

        val buffer = ByteBuffer.wrap(header).order(ByteOrder.LITTLE_ENDIAN)
        val riff = ByteArray(4)
        buffer.get(riff)
        assertArrayEquals("RIFF".toByteArray(), riff)
        assertEquals(36 + dataSize, buffer.int)
        val wave = ByteArray(4)
        buffer.get(wave)
        assertArrayEquals("WAVE".toByteArray(), wave)

        val fmt = ByteArray(4)
        buffer.get(fmt)
        assertArrayEquals("fmt ".toByteArray(), fmt)
        assertEquals(16, buffer.int)
        assertEquals(1.toShort(), buffer.short) // PCM
        assertEquals(1.toShort(), buffer.short) // mono
        assertEquals(16_000, buffer.int)
        assertEquals(32_000, buffer.int) // byte rate
        assertEquals(2.toShort(), buffer.short) // block align
        assertEquals(16.toShort(), buffer.short) // bits

        val data = ByteArray(4)
        buffer.get(data)
        assertArrayEquals("data".toByteArray(), data)
        assertEquals(dataSize, buffer.int)
    }

    @Test
    fun patchUpdatesChunkSizesWithoutRewritingAudio() {
        val header = WavHeader.pcm16Mono(dataBytes = 0, sampleRateHz = 16_000)
        val patched = WavHeader.patchDataSize(header, dataBytes = 8000)
        val buffer = ByteBuffer.wrap(patched).order(ByteOrder.LITTLE_ENDIAN)
        buffer.position(4)
        assertEquals(36 + 8000, buffer.int)
        buffer.position(40)
        assertEquals(8000, buffer.int)
    }
}

class ManifestCsvTest {
    @Test
    fun writesHeaderAndEscapesNotes() {
        val episode =
            Episode(
                id = "id1",
                recordedAtEpochMs = 1_000L,
                durationMs = 2_000L,
                sampleRateHz = 16_000,
                label = OutcomeLabel.GAS,
                labeledAtEpochMs = 1_100L,
                notes = "gas, then sleep",
                infantAgeWeeks = 6,
            )
        val csv = ManifestCsv.write(listOf(episode))
        val lines = csv.trim().split('\n')
        assertEquals(
            "id,recorded_at_epoch_ms,duration_ms,sample_rate_hz,label,dbl_sound,notes,infant_age_weeks,audio_filename",
            lines[0],
        )
        assertTrue(lines[1].contains("\"gas, then sleep\""))
        assertTrue(lines[1].contains("Eairh"))
        assertTrue(lines[1].endsWith("id1.wav"))
    }
}
