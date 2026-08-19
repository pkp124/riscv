package com.crylabeler.core

/**
 * Tiny JSON codec for the flat episode record. Kept dependency-free so the
 * dataset format can be tested on a plain JDK and parsed from Python later.
 */
object EpisodeJson {
    fun encode(episode: Episode): String {
        val fields = mutableListOf<String>()
        fields += jsonPair("schema_version", episode.schemaVersion)
        fields += jsonPair("id", episode.id)
        fields += jsonPair("recorded_at_epoch_ms", episode.recordedAtEpochMs)
        fields += jsonPair("duration_ms", episode.durationMs)
        fields += jsonPair("sample_rate_hz", episode.sampleRateHz)
        fields += jsonPair("label", episode.label.id)
        fields += jsonPair("dbl_sound", episode.label.dblSound)
        fields += jsonPair("labeled_at_epoch_ms", episode.labeledAtEpochMs)
        fields += jsonPair("notes", episode.notes)
        fields += jsonPair("infant_age_weeks", episode.infantAgeWeeks)
        return fields.joinToString(prefix = "{", postfix = "}")
    }

    fun decode(json: String): Episode {
        val obj = parseObject(json)
        return Episode(
            id = obj.requiredString("id"),
            recordedAtEpochMs = obj.requiredLong("recorded_at_epoch_ms"),
            durationMs = obj.requiredLong("duration_ms"),
            sampleRateHz = obj.requiredInt("sample_rate_hz"),
            label = OutcomeLabel.fromId(obj.requiredString("label")),
            labeledAtEpochMs = obj.requiredLong("labeled_at_epoch_ms"),
            notes = obj.optionalString("notes") ?: "",
            infantAgeWeeks = obj.optionalInt("infant_age_weeks"),
            schemaVersion = obj.optionalInt("schema_version") ?: Episode.SCHEMA_VERSION,
        )
    }

    private fun jsonPair(key: String, value: String?): String {
        val encoded = if (value == null) "null" else quote(value)
        return "\"$key\":$encoded"
    }

    private fun jsonPair(key: String, value: Int): String = "\"$key\":$value"

    private fun jsonPair(key: String, value: Long): String = "\"$key\":$value"

    private fun jsonPair(key: String, value: Int?): String {
        return if (value == null) "\"$key\":null" else "\"$key\":$value"
    }

    private fun quote(value: String): String {
        val escaped =
            value
                .replace("\\", "\\\\")
                .replace("\"", "\\\"")
                .replace("\n", "\\n")
                .replace("\r", "\\r")
                .replace("\t", "\\t")
        return "\"$escaped\""
    }

    private fun parseObject(json: String): JsonMap {
        val trimmed = json.trim()
        require(trimmed.startsWith("{") && trimmed.endsWith("}")) { "Episode JSON must be an object" }
        val body = trimmed.substring(1, trimmed.length - 1)
        val values = linkedMapOf<String, JsonValue>()
        var i = 0
        while (i < body.length) {
            i = skipWs(body, i)
            if (i >= body.length) {
                break
            }
            require(body[i] == '"') { "Expected key at index $i" }
            val keyParse = readString(body, i)
            i = skipWs(body, keyParse.end)
            require(i < body.length && body[i] == ':') { "Expected ':' after key ${keyParse.value}" }
            i = skipWs(body, i + 1)
            val valueParse = readValue(body, i)
            values[keyParse.value] = valueParse.value
            i = skipWs(body, valueParse.end)
            if (i < body.length && body[i] == ',') {
                i += 1
            }
        }
        return JsonMap(values)
    }

    private fun skipWs(text: String, start: Int): Int {
        var i = start
        while (i < text.length && text[i].isWhitespace()) {
            i += 1
        }
        return i
    }

    private fun readString(text: String, start: Int): ParseResult<String> {
        require(text[start] == '"') { "Expected string" }
        val out = StringBuilder()
        var i = start + 1
        while (i < text.length) {
            val ch = text[i]
            when {
                ch == '"' -> return ParseResult(out.toString(), i + 1)
                ch == '\\' -> {
                    require(i + 1 < text.length) { "Dangling escape" }
                    when (val escaped = text[i + 1]) {
                        '"', '\\', '/' -> out.append(escaped)
                        'n' -> out.append('\n')
                        'r' -> out.append('\r')
                        't' -> out.append('\t')
                        else -> throw IllegalArgumentException("Unsupported escape \\$escaped")
                    }
                    i += 2
                }
                else -> {
                    out.append(ch)
                    i += 1
                }
            }
        }
        throw IllegalArgumentException("Unterminated string")
    }

    private fun readValue(text: String, start: Int): ParseResult<JsonValue> {
        return when {
            text.startsWith("null", start) -> ParseResult(JsonValue.Null, start + 4)
            text[start] == '"' -> {
                val parsed = readString(text, start)
                ParseResult(JsonValue.Text(parsed.value), parsed.end)
            }
            else -> {
                var i = start
                if (i < text.length && (text[i] == '-' || text[i] == '+')) {
                    i += 1
                }
                while (i < text.length && text[i].isDigit()) {
                    i += 1
                }
                require(i > start) { "Expected value at $start" }
                ParseResult(JsonValue.Number(text.substring(start, i).toLong()), i)
            }
        }
    }

    private data class ParseResult<T>(val value: T, val end: Int)

    private sealed class JsonValue {
        data object Null : JsonValue()

        data class Text(val value: String) : JsonValue()

        data class Number(val value: Long) : JsonValue()
    }

    private class JsonMap(private val values: Map<String, JsonValue>) {
        fun requiredString(key: String): String {
            return optionalString(key) ?: throw IllegalArgumentException("Missing string $key")
        }

        fun optionalString(key: String): String? {
            return when (val value = values[key]) {
                null, JsonValue.Null -> null
                is JsonValue.Text -> value.value
                else -> throw IllegalArgumentException("$key is not a string")
            }
        }

        fun requiredLong(key: String): Long {
            return when (val value = values[key]) {
                is JsonValue.Number -> value.value
                else -> throw IllegalArgumentException("Missing number $key")
            }
        }

        fun requiredInt(key: String): Int = requiredLong(key).toInt()

        fun optionalInt(key: String): Int? {
            return when (val value = values[key]) {
                null, JsonValue.Null -> null
                is JsonValue.Number -> value.value.toInt()
                else -> throw IllegalArgumentException("$key is not a number")
            }
        }
    }
}
