package com.crylabeler.core

import java.nio.ByteBuffer
import java.nio.ByteOrder

object WavHeader {
    const val SIZE_BYTES: Int = 44

    fun pcm16Mono(dataBytes: Int, sampleRateHz: Int): ByteArray {
        require(dataBytes >= 0) { "dataBytes must be >= 0" }
        require(sampleRateHz > 0) { "sampleRateHz must be > 0" }
        val header = ByteArray(SIZE_BYTES)
        val buffer = ByteBuffer.wrap(header).order(ByteOrder.LITTLE_ENDIAN)
        val channels = 1
        val bitsPerSample = 16
        val byteRate = sampleRateHz * channels * bitsPerSample / 8
        val blockAlign = (channels * bitsPerSample / 8).toShort()

        buffer.put("RIFF".toByteArray())
        buffer.putInt(36 + dataBytes)
        buffer.put("WAVE".toByteArray())
        buffer.put("fmt ".toByteArray())
        buffer.putInt(16)
        buffer.putShort(1) // PCM
        buffer.putShort(channels.toShort())
        buffer.putInt(sampleRateHz)
        buffer.putInt(byteRate)
        buffer.putShort(blockAlign)
        buffer.putShort(bitsPerSample.toShort())
        buffer.put("data".toByteArray())
        buffer.putInt(dataBytes)
        return header
    }

    fun patchDataSize(header: ByteArray, dataBytes: Int): ByteArray {
        require(header.size == SIZE_BYTES) { "WAV header must be 44 bytes" }
        val patched = header.copyOf()
        val buffer = ByteBuffer.wrap(patched).order(ByteOrder.LITTLE_ENDIAN)
        buffer.putInt(4, 36 + dataBytes)
        buffer.putInt(40, dataBytes)
        return patched
    }
}
