package com.crylabeler.app.ui.theme

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color

private val RecordCoral = Color(0xFFE85D4C)
private val Night = Color(0xFF121212)
private val Raised = Color(0xFF1C1C1E)
private val Ink = Color(0xFFF2F2F7)
private val Quiet = Color(0xFF8E8E93)

private val DarkColors =
    darkColorScheme(
        primary = RecordCoral,
        onPrimary = Color.White,
        background = Night,
        onBackground = Ink,
        surface = Raised,
        onSurface = Ink,
        onSurfaceVariant = Quiet,
        secondary = Color(0xFF7AA2F7),
        error = Color(0xFFFF6B6B),
    )

@Composable
fun CryLabelerTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = DarkColors,
        content = content,
    )
}
