package com.crylabeler.app

import android.content.Context
import android.content.Intent
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.crylabeler.app.audio.WavPlayer
import com.crylabeler.app.audio.WavRecorder
import com.crylabeler.app.data.EpisodeRepository
import com.crylabeler.core.Episode
import com.crylabeler.core.EpisodeIds
import com.crylabeler.core.OutcomeLabel
import java.io.File
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext

data class CryLabelerUiState(
    val screen: Screen = Screen.Home,
    val episodes: List<Episode> = emptyList(),
    val infantAgeWeeksText: String = "",
    val elapsedMs: Long = 0L,
    val notes: String = "",
    val playingId: String? = null,
    val message: String? = null,
    val error: String? = null,
)

sealed class Screen {
    data object Home : Screen()

    data object Recording : Screen()

    data class Labeling(
        val id: String,
        val wav: File,
        val recordedAtEpochMs: Long,
        val durationMs: Long,
        val relabelExisting: Boolean,
    ) : Screen()

    data object Log : Screen()

    data object Settings : Screen()
}

class CryLabelerViewModel(
    private val repository: EpisodeRepository,
    private val player: WavPlayer = WavPlayer(),
) : ViewModel() {
    private val _state = MutableStateFlow(CryLabelerUiState())
    val state: StateFlow<CryLabelerUiState> = _state

    private var recorder: WavRecorder? = null
    private var ticker: Job? = null
    private var pendingWav: File? = null

    init {
        reload()
    }

    fun dismissMessage() {
        _state.update { it.copy(message = null, error = null) }
    }

    fun goHome() {
        player.stop()
        _state.update { it.copy(screen = Screen.Home, playingId = null, notes = "") }
    }

    fun openLog() {
        player.stop()
        reload()
        _state.update { it.copy(screen = Screen.Log, playingId = null) }
    }

    fun openSettings() {
        _state.update { it.copy(screen = Screen.Settings) }
    }

    fun updateAgeWeeks(text: String) {
        _state.update { it.copy(infantAgeWeeksText = text.filter { ch -> ch.isDigit() }.take(3)) }
    }

    fun saveAgeWeeks() {
        val weeks = _state.value.infantAgeWeeksText.toIntOrNull()
        repository.setInfantAgeWeeks(weeks)
        _state.update { it.copy(message = "Saved infant age") }
    }

    fun updateNotes(text: String) {
        _state.update { it.copy(notes = text.take(240)) }
    }

    fun startRecording() {
        if (_state.value.screen is Screen.Recording) {
            return
        }
        player.stop()
        val recordedAt = System.currentTimeMillis()
        val id = EpisodeIds.create(recordedAt)
        val wav = repository.createWavFile(id)
        val wavRecorder = WavRecorder(wav)
        try {
            wavRecorder.start()
        } catch (error: Exception) {
            wav.delete()
            _state.update { it.copy(error = error.message ?: "Could not start microphone") }
            return
        }
        recorder = wavRecorder
        pendingWav = wav
        _state.update {
            it.copy(
                screen = Screen.Recording,
                elapsedMs = 0L,
                notes = "",
                error = null,
                playingId = null,
            )
        }
        ticker?.cancel()
        ticker =
            viewModelScope.launch {
                val startedAt = System.currentTimeMillis()
                while (isActive) {
                    delay(100)
                    val elapsed = System.currentTimeMillis() - startedAt
                    _state.update { ui -> ui.copy(elapsedMs = elapsed) }
                    if (elapsed >= WavRecorder.MAX_DURATION_MS) {
                        stopRecording(recordedAtEpochMs = recordedAt, id = id, wav = wav)
                        break
                    }
                }
            }
        // Stash ids on the recorder via pending file name.
        pendingMeta = PendingRecording(id, recordedAt, wav)
    }

    fun stopRecording() {
        val pending = pendingMeta ?: return
        stopRecording(pending.recordedAt, pending.id, pending.wav)
    }

    fun discardDraft() {
        ticker?.cancel()
        runCatching { recorder?.cancel() }
        recorder = null
        pendingWav?.delete()
        pendingWav = null
        pendingMeta = null
        _state.update { it.copy(screen = Screen.Home, elapsedMs = 0L, notes = "") }
    }

    fun saveLabel(label: OutcomeLabel) {
        val screen = _state.value.screen as? Screen.Labeling ?: return
        viewModelScope.launch {
            val now = System.currentTimeMillis()
            if (screen.relabelExisting) {
                withContext(Dispatchers.IO) {
                    repository.relabel(
                        id = screen.id,
                        label = label,
                        notes = _state.value.notes.trim(),
                        labeledAtEpochMs = now,
                    )
                }
            } else {
                val episode =
                    Episode(
                        id = screen.id,
                        recordedAtEpochMs = screen.recordedAtEpochMs,
                        durationMs = screen.durationMs,
                        sampleRateHz = WavRecorder.SAMPLE_RATE_HZ,
                        label = label,
                        labeledAtEpochMs = now,
                        notes = _state.value.notes.trim(),
                        infantAgeWeeks = repository.infantAgeWeeks(),
                    )
                withContext(Dispatchers.IO) { repository.save(episode) }
            }
            pendingMeta = null
            pendingWav = null
            reload()
            _state.update {
                it.copy(
                    screen = Screen.Home,
                    notes = "",
                    message = "Saved ${label.title.lowercase()}",
                )
            }
        }
    }

    fun deleteEpisode(id: String) {
        player.stop()
        repository.delete(id)
        reload()
    }

    fun relabelEpisode(episode: Episode) {
        player.stop()
        _state.update {
            it.copy(
                screen =
                    Screen.Labeling(
                        id = episode.id,
                        wav = repository.audioFile(episode.id),
                        recordedAtEpochMs = episode.recordedAtEpochMs,
                        durationMs = episode.durationMs,
                        relabelExisting = true,
                    ),
                notes = episode.notes,
                playingId = null,
            )
        }
    }

    fun togglePlayback(id: String) {
        val file = repository.audioFile(id)
        if (!file.exists()) {
            _state.update { it.copy(error = "Audio file missing") }
            return
        }
        if (_state.value.playingId == id) {
            player.stop()
            _state.update { it.copy(playingId = null) }
            return
        }
        runCatching {
            player.play(file) { _state.update { ui -> ui.copy(playingId = null) } }
            _state.update { it.copy(playingId = id) }
        }.onFailure { error ->
            _state.update { it.copy(error = error.message ?: "Playback failed", playingId = null) }
        }
    }

    fun export(): Intent? {
        return runCatching {
            if (_state.value.episodes.isEmpty()) {
                _state.update { it.copy(error = "No labelled episodes to export yet") }
                return null
            }
            val zip = runBlocking(Dispatchers.IO) { repository.exportZip() }
            _state.update { it.copy(message = "Export ready") }
            repository.shareZipIntent(zip)
        }.onFailure { error ->
            _state.update { it.copy(error = error.message ?: "Export failed") }
        }.getOrNull()
    }

    override fun onCleared() {
        ticker?.cancel()
        player.stop()
        runCatching { recorder?.cancel() }
        super.onCleared()
    }

    private fun stopRecording(recordedAtEpochMs: Long, id: String, wav: File) {
        ticker?.cancel()
        val duration =
            try {
                recorder?.stop() ?: 0L
            } catch (error: Exception) {
                _state.update { it.copy(error = error.message, screen = Screen.Home) }
                wav.delete()
                recorder = null
                pendingMeta = null
                return
            }
        recorder = null
        if (duration < WavRecorder.MIN_DURATION_MS) {
            wav.delete()
            pendingMeta = null
            pendingWav = null
            _state.update {
                it.copy(
                    screen = Screen.Home,
                    error = "That clip was too short. Try again once the cry is going.",
                )
            }
            return
        }
        _state.update {
            it.copy(
                screen =
                    Screen.Labeling(
                        id = id,
                        wav = wav,
                        recordedAtEpochMs = recordedAtEpochMs,
                        durationMs = duration,
                        relabelExisting = false,
                    ),
                elapsedMs = duration,
            )
        }
    }

    private fun reload() {
        val episodes = repository.listEpisodes()
        val age = repository.infantAgeWeeks()?.toString().orEmpty()
        _state.update { it.copy(episodes = episodes, infantAgeWeeksText = age) }
    }

    private data class PendingRecording(
        val id: String,
        val recordedAt: Long,
        val wav: File,
    )

    private var pendingMeta: PendingRecording? = null

    companion object {
        fun factory(context: Context): ViewModelProvider.Factory {
            val appContext = context.applicationContext
            return object : ViewModelProvider.Factory {
                @Suppress("UNCHECKED_CAST")
                override fun <T : ViewModel> create(modelClass: Class<T>): T {
                    return CryLabelerViewModel(EpisodeRepository(appContext)) as T
                }
            }
        }
    }
}
