package com.crylabeler.core

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import kotlin.random.Random

class EpisodeIdsTest {
    @Test
    fun usesUtcTimestampAndHexSuffix() {
        val id = EpisodeIds.create(recordedAtEpochMs = 0L, random = Random(1))
        assertTrue(id.startsWith("19700101T000000Z_"))
        assertEquals(21, id.length)
        assertTrue(id.substringAfter('_').matches(Regex("[0-9a-f]{4}")))
    }
}
