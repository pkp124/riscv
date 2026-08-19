package com.crylabeler.core

data class Episode(
    val id: String,
    val recordedAtEpochMs: Long,
    val durationMs: Long,
    val sampleRateHz: Int,
    val label: OutcomeLabel,
    val labeledAtEpochMs: Long,
    val notes: String = "",
    val infantAgeWeeks: Int? = null,
    val schemaVersion: Int = SCHEMA_VERSION,
) {
    fun audioFilename(): String = "$id.wav"

    fun metadataFilename(): String = "$id.json"

    companion object {
        const val SCHEMA_VERSION: Int = 1
    }
}
