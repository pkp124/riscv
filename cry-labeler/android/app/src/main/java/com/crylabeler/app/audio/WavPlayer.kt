package com.crylabeler.app.audio

import android.media.MediaPlayer
import java.io.File

class WavPlayer {
    private var player: MediaPlayer? = null

    fun play(file: File, onComplete: () -> Unit = {}) {
        stop()
        val mediaPlayer = MediaPlayer()
        player = mediaPlayer
        mediaPlayer.setDataSource(file.absolutePath)
        mediaPlayer.setOnCompletionListener {
            stop()
            onComplete()
        }
        mediaPlayer.setOnErrorListener { _, _, _ ->
            stop()
            true
        }
        mediaPlayer.prepare()
        mediaPlayer.start()
    }

    fun stop() {
        player?.setOnCompletionListener(null)
        player?.setOnErrorListener(null)
        try {
            player?.stop()
        } catch (_: IllegalStateException) {
            // Already stopped.
        }
        player?.release()
        player = null
    }

    fun isPlaying(): Boolean = player?.isPlaying == true
}
