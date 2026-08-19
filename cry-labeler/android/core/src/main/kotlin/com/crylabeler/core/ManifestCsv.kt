package com.crylabeler.core

object ManifestCsv {
    const val HEADER: String =
        "id,recorded_at_epoch_ms,duration_ms,sample_rate_hz,label,dbl_sound,notes,infant_age_weeks,audio_filename"

    fun write(episodes: List<Episode>): String {
        val lines = ArrayList<String>(episodes.size + 1)
        lines += HEADER
        for (episode in episodes) {
            lines +=
                listOf(
                    episode.id,
                    episode.recordedAtEpochMs.toString(),
                    episode.durationMs.toString(),
                    episode.sampleRateHz.toString(),
                    episode.label.id,
                    episode.label.dblSound ?: "",
                    csvEscape(episode.notes),
                    episode.infantAgeWeeks?.toString() ?: "",
                    episode.audioFilename(),
                ).joinToString(",")
        }
        return lines.joinToString("\n", postfix = "\n")
    }

    private fun csvEscape(value: String): String {
        val needsQuotes =
            value.contains(',') ||
                value.contains('"') ||
                value.contains('\n') ||
                value.contains('\r')
        if (!needsQuotes) {
            return value
        }
        return "\"" + value.replace("\"", "\"\"") + "\""
    }
}
