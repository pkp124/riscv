package com.crylabeler.app.data

import android.content.Context
import android.content.Intent
import androidx.core.content.FileProvider
import com.crylabeler.core.Episode
import com.crylabeler.core.EpisodeJson
import com.crylabeler.core.ManifestCsv
import com.crylabeler.core.OutcomeLabel
import java.io.File
import java.util.zip.ZipEntry
import java.util.zip.ZipOutputStream

class EpisodeRepository(private val context: Context) {
    private val prefs = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
    private val episodesDir: File = File(context.filesDir, "episodes").also { it.mkdirs() }

    fun infantAgeWeeks(): Int? = prefs.getString(KEY_AGE_WEEKS, null)?.toIntOrNull()

    fun setInfantAgeWeeks(weeks: Int?) {
        prefs.edit().putString(KEY_AGE_WEEKS, weeks?.toString().orEmpty()).apply()
    }

    fun createWavFile(id: String): File = File(episodesDir, "$id.wav")

    fun listEpisodes(): List<Episode> {
        return episodesDir
            .listFiles { file -> file.extension == "json" }
            .orEmpty()
            .mapNotNull { file ->
                runCatching { EpisodeJson.decode(file.readText()) }.getOrNull()
            }.sortedByDescending { it.recordedAtEpochMs }
    }

    fun save(episode: Episode) {
        File(episodesDir, episode.metadataFilename()).writeText(EpisodeJson.encode(episode))
    }

    fun relabel(id: String, label: OutcomeLabel, notes: String, labeledAtEpochMs: Long): Episode? {
        val existing = listEpisodes().firstOrNull { it.id == id } ?: return null
        val updated =
            existing.copy(
                label = label,
                notes = notes,
                labeledAtEpochMs = labeledAtEpochMs,
            )
        save(updated)
        return updated
    }

    fun delete(id: String) {
        File(episodesDir, "$id.wav").delete()
        File(episodesDir, "$id.json").delete()
    }

    fun audioFile(id: String): File = File(episodesDir, "$id.wav")

    fun exportZip(): File {
        val episodes = listEpisodes()
        val zip = File(context.cacheDir, "cry-labeler-export.zip")
        ZipOutputStream(zip.outputStream().buffered()).use { zos ->
            putText(zos, "manifest.csv", ManifestCsv.write(episodes))
            putText(zos, "README.txt", EXPORT_README)
            for (episode in episodes) {
                val wav = audioFile(episode.id)
                if (wav.exists()) {
                    zos.putNextEntry(ZipEntry("audio/${episode.audioFilename()}"))
                    wav.inputStream().use { it.copyTo(zos) }
                    zos.closeEntry()
                }
                putText(zos, "meta/${episode.metadataFilename()}", EpisodeJson.encode(episode))
            }
        }
        return zip
    }

    fun shareZipIntent(zip: File): Intent {
        val uri =
            FileProvider.getUriForFile(
                context,
                "${context.packageName}.fileprovider",
                zip,
            )
        return Intent(Intent.ACTION_SEND).apply {
            type = "application/zip"
            putExtra(Intent.EXTRA_STREAM, uri)
            putExtra(Intent.EXTRA_SUBJECT, "Cry Label export")
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
        }
    }

    private fun putText(zos: ZipOutputStream, name: String, text: String) {
        zos.putNextEntry(ZipEntry(name))
        zos.write(text.toByteArray(Charsets.UTF_8))
        zos.closeEntry()
    }

    companion object {
        private const val PREFS = "cry_labeler"
        private const val KEY_AGE_WEEKS = "infant_age_weeks"
        private val EXPORT_README =
            """
            Cry Label export
            =================
            Each labelled episode is a 16 kHz mono 16-bit PCM WAV plus a JSON sidecar.

            Labels are what happened immediately after the cry (the DBL-claimed outcomes),
            not a phonetic guess:

              fed         Neh   hunger
              slept       Owh   sleepy
              discomfort  Heh   wet / cold / position
              gas         Eairh lower wind
              burped      Eh    upper wind
              other       —     does not fit
              unsure      —     unknown

            After a few hundred labelled episodes, run:

              python3 tools/analyze_separability.py /path/to/unzipped/export
            """.trimIndent()
    }
}
