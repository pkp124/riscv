package com.crylabeler.app.audio

import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import com.crylabeler.core.WavHeader
import java.io.File
import java.io.RandomAccessFile

class WavRecorder(
    private val outputFile: File,
    private val sampleRateHz: Int = SAMPLE_RATE_HZ,
) {
    private var audioRecord: AudioRecord? = null
    private var writerThread: Thread? = null
    @Volatile private var running: Boolean = false
    @Volatile private var dataBytes: Int = 0
    private var startedAtMs: Long = 0L

    fun start() {
        check(audioRecord == null) { "Recorder already started" }
        val minBuffer =
            AudioRecord.getMinBufferSize(
                sampleRateHz,
                AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_16BIT,
            )
        require(minBuffer > 0) { "AudioRecord is not available on this device" }
        val bufferSize = minBuffer.coerceAtLeast(sampleRateHz * 2)

        outputFile.parentFile?.mkdirs()
        RandomAccessFile(outputFile, "rw").use { raf ->
            raf.setLength(0)
            raf.write(WavHeader.pcm16Mono(dataBytes = 0, sampleRateHz = sampleRateHz))
        }

        val recorder =
            AudioRecord(
                MediaRecorder.AudioSource.MIC,
                sampleRateHz,
                AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_16BIT,
                bufferSize,
            )
        if (recorder.state != AudioRecord.STATE_INITIALIZED) {
            recorder.release()
            throw IllegalStateException("Microphone failed to initialize")
        }

        audioRecord = recorder
        running = true
        dataBytes = 0
        startedAtMs = System.currentTimeMillis()
        recorder.startRecording()
        writerThread =
            Thread({
                writeLoop(recorder, bufferSize)
            }, "wav-recorder").also { it.start() }
    }

    fun stop(): Long {
        running = false
        writerThread?.join(2_000)
        writerThread = null
        audioRecord?.let { recorder ->
            if (recorder.recordingState == AudioRecord.RECORDSTATE_RECORDING) {
                recorder.stop()
            }
            recorder.release()
        }
        audioRecord = null
        patchHeader()
        val elapsed = (System.currentTimeMillis() - startedAtMs).coerceAtLeast(0L)
        val fromBytes = (dataBytes / 2L) * 1000L / sampleRateHz
        return if (fromBytes > 0) fromBytes else elapsed
    }

    fun cancel() {
        try {
            stop()
        } finally {
            outputFile.delete()
        }
    }

    private fun writeLoop(recorder: AudioRecord, bufferSize: Int) {
        val buffer = ByteArray(bufferSize)
        RandomAccessFile(outputFile, "rw").use { raf ->
            raf.seek(WavHeader.SIZE_BYTES.toLong())
            while (running) {
                val read = recorder.read(buffer, 0, buffer.size)
                if (read > 0) {
                    raf.write(buffer, 0, read)
                    dataBytes += read
                }
            }
        }
    }

    private fun patchHeader() {
        if (!outputFile.exists()) {
            return
        }
        RandomAccessFile(outputFile, "rw").use { raf ->
            raf.seek(0)
            raf.write(WavHeader.pcm16Mono(dataBytes = dataBytes, sampleRateHz = sampleRateHz))
        }
    }

    companion object {
        const val SAMPLE_RATE_HZ: Int = 16_000
        const val MAX_DURATION_MS: Long = 20_000
        const val MIN_DURATION_MS: Long = 400
    }
}
