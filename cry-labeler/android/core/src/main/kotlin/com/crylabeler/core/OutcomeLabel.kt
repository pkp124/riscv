package com.crylabeler.core

/**
 * Parent-observed outcomes that Dunstan Baby Language (DBL) claims map onto
 * distinct cry sounds. The stored label is the outcome, not a phonetic guess:
 * that is the ground truth we need before asking whether the audio separates.
 */
enum class OutcomeLabel(
    val id: String,
    val title: String,
    val subtitle: String,
    val dblSound: String?,
) {
    FED(
        id = "fed",
        title = "Fed",
        subtitle = "Hungry — DBL Neh",
        dblSound = "Neh",
    ),
    SLEPT(
        id = "slept",
        title = "Slept / settled",
        subtitle = "Tired — DBL Owh",
        dblSound = "Owh",
    ),
    DISCOMFORT(
        id = "discomfort",
        title = "Discomfort eased",
        subtitle = "Wet, cold, position — DBL Heh",
        dblSound = "Heh",
    ),
    GAS(
        id = "gas",
        title = "Gas / tummy relief",
        subtitle = "Lower wind — DBL Eairh",
        dblSound = "Eairh",
    ),
    BURPED(
        id = "burped",
        title = "Burped",
        subtitle = "Upper wind — DBL Eh",
        dblSound = "Eh",
    ),
    OTHER(
        id = "other",
        title = "Something else",
        subtitle = "Does not fit a DBL category",
        dblSound = null,
    ),
    UNSURE(
        id = "unsure",
        title = "Not sure",
        subtitle = "Label later if you learn more",
        dblSound = null,
    ),
    ;

    companion object {
        fun fromId(id: String): OutcomeLabel {
            return entries.firstOrNull { it.id == id }
                ?: throw IllegalArgumentException("Unknown outcome label: $id")
        }
    }
}
