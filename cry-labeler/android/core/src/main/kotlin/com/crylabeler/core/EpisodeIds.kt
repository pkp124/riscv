package com.crylabeler.core

import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.TimeZone
import kotlin.random.Random

object EpisodeIds {
    fun create(recordedAtEpochMs: Long, random: Random = Random.Default): String {
        val utc = SimpleDateFormat("yyyyMMdd'T'HHmmss'Z'", Locale.US)
        utc.timeZone = TimeZone.getTimeZone("UTC")
        val stamp = utc.format(Date(recordedAtEpochMs))
        val suffix = random.nextInt(0x10000).toString(16).padStart(4, '0')
        return "${stamp}_$suffix"
    }
}
