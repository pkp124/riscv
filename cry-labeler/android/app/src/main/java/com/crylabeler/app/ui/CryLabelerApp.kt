package com.crylabeler.app.ui

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.view.WindowManager
import androidx.activity.compose.BackHandler
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.animation.core.RepeatMode
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.automirrored.filled.List
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.Share
import androidx.compose.material.icons.filled.Stop
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FilledTonalButton
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.scale
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalView
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.content.ContextCompat
import com.crylabeler.app.CryLabelerViewModel
import com.crylabeler.app.Screen
import com.crylabeler.app.audio.WavRecorder
import com.crylabeler.core.Episode
import com.crylabeler.core.OutcomeLabel
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

@Composable
fun CryLabelerApp(viewModel: CryLabelerViewModel) {
    val state by viewModel.state.collectAsState()
    val snackbar = remember { SnackbarHostState() }
    val context = LocalContext.current

    LaunchedEffect(state.message, state.error) {
        val text = state.error ?: state.message
        if (text != null) {
            snackbar.showSnackbar(text)
            viewModel.dismissMessage()
        }
    }

    KeepScreenOn(enabled = state.screen is Screen.Recording)

    when (val screen = state.screen) {
        Screen.Home ->
            HomeScreen(
                episodeCount = state.episodes.size,
                snackbar = snackbar,
                onRecord = { viewModel.startRecording() },
                onLog = { viewModel.openLog() },
                onSettings = { viewModel.openSettings() },
                onExport = {
                    viewModel.export()?.let { shareIntent ->
                        context.startActivity(Intent.createChooser(shareIntent, "Export labelled cries"))
                    }
                },
            )

        Screen.Recording ->
            RecordingScreen(
                elapsedMs = state.elapsedMs,
                onStop = { viewModel.stopRecording() },
                onCancel = { viewModel.discardDraft() },
            )

        is Screen.Labeling ->
            LabelScreen(
                durationMs = screen.durationMs,
                relabel = screen.relabelExisting,
                notes = state.notes,
                onNotes = viewModel::updateNotes,
                onLabel = viewModel::saveLabel,
                onDiscard = {
                    if (screen.relabelExisting) {
                        viewModel.openLog()
                    } else {
                        viewModel.discardDraft()
                    }
                },
                onPlay = { viewModel.togglePlayback(screen.id) },
                playing = state.playingId == screen.id,
            )

        Screen.Log ->
            LogScreen(
                episodes = state.episodes,
                playingId = state.playingId,
                snackbar = snackbar,
                onBack = viewModel::goHome,
                onPlay = viewModel::togglePlayback,
                onRelabel = viewModel::relabelEpisode,
                onDelete = viewModel::deleteEpisode,
            )

        Screen.Settings ->
            SettingsScreen(
                ageText = state.infantAgeWeeksText,
                snackbar = snackbar,
                onAge = viewModel::updateAgeWeeks,
                onSave = viewModel::saveAgeWeeks,
                onBack = viewModel::goHome,
            )
    }
}

@Composable
private fun KeepScreenOn(enabled: Boolean) {
    val view = LocalView.current
    DisposableEffect(enabled) {
        val window = (view.context as? Activity)?.window
        if (enabled) {
            window?.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        }
        onDispose {
            window?.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun HomeScreen(
    episodeCount: Int,
    snackbar: SnackbarHostState,
    onRecord: () -> Unit,
    onLog: () -> Unit,
    onSettings: () -> Unit,
    onExport: () -> Unit,
) {
    val context = LocalContext.current
    var showPermissionHelp by remember { mutableStateOf(false) }
    val permissionLauncher =
        rememberLauncherForActivityResult(ActivityResultContracts.RequestPermission()) { granted ->
            if (granted) {
                onRecord()
            } else {
                showPermissionHelp = true
            }
        }

    Scaffold(
        snackbarHost = { SnackbarHost(snackbar) },
        topBar = {
            TopAppBar(
                title = { Text("Cry Label") },
                actions = {
                    IconButton(onClick = onLog) {
                        Icon(Icons.AutoMirrored.Filled.List, contentDescription = "Review log")
                    }
                    IconButton(onClick = onExport) {
                        Icon(Icons.Default.Share, contentDescription = "Export")
                    }
                    IconButton(onClick = onSettings) {
                        Icon(Icons.Default.Settings, contentDescription = "Settings")
                    }
                },
                colors =
                    TopAppBarDefaults.topAppBarColors(
                        containerColor = MaterialTheme.colorScheme.background,
                    ),
            )
        },
    ) { padding ->
        Column(
            modifier =
                Modifier
                    .fillMaxSize()
                    .padding(padding)
                    .padding(horizontal = 24.dp)
                    .navigationBarsPadding(),
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            Spacer(Modifier.height(12.dp))
            Text(
                text = "Record a cry segment, then immediately tap what happened.",
                style = MaterialTheme.typography.bodyLarge,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
                textAlign = TextAlign.Center,
            )
            Spacer(Modifier.height(8.dp))
            Text(
                text = "$episodeCount labelled",
                style = MaterialTheme.typography.titleMedium,
                fontWeight = FontWeight.SemiBold,
            )
            Spacer(Modifier.weight(1f))
            RecordButton(
                onClick = {
                    val granted =
                        ContextCompat.checkSelfPermission(
                            context,
                            Manifest.permission.RECORD_AUDIO,
                        ) == PackageManager.PERMISSION_GRANTED
                    if (granted) {
                        onRecord()
                    } else {
                        permissionLauncher.launch(Manifest.permission.RECORD_AUDIO)
                    }
                },
            )
            Spacer(Modifier.height(16.dp))
            Text(
                text = "Tap to record  ·  up to 20 seconds",
                color = MaterialTheme.colorScheme.onSurfaceVariant,
            )
            Spacer(Modifier.weight(1f))
            Text(
                text = "Labels are outcomes (fed, slept, burped…), which is what DBL claims the sounds map to. A few hundred clips is enough for a first look at whether those classes separate in audio.",
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
                textAlign = TextAlign.Center,
            )
            Spacer(Modifier.height(24.dp))
        }
    }

    if (showPermissionHelp) {
        AlertDialog(
            onDismissRequest = { showPermissionHelp = false },
            confirmButton = {
                TextButton(onClick = { showPermissionHelp = false }) { Text("OK") }
            },
            title = { Text("Microphone needed") },
            text = { Text("Cry Label records on-device only. Enable the microphone permission to capture a segment.") },
        )
    }
}

@Composable
private fun RecordButton(onClick: () -> Unit) {
    Button(
        onClick = onClick,
        modifier = Modifier.size(180.dp),
        shape = CircleShape,
        colors =
            ButtonDefaults.buttonColors(
                containerColor = MaterialTheme.colorScheme.primary,
                contentColor = Color.White,
            ),
    ) {
        Text("RECORD", fontSize = 22.sp, fontWeight = FontWeight.Bold)
    }
}

@Composable
private fun RecordingScreen(
    elapsedMs: Long,
    onStop: () -> Unit,
    onCancel: () -> Unit,
) {
    BackHandler(onBack = onCancel)
    val pulse = rememberInfiniteTransition(label = "pulse")
    val scale by
        pulse.animateFloat(
            initialValue = 1f,
            targetValue = 1.08f,
            animationSpec =
                infiniteRepeatable(tween(600), RepeatMode.Reverse),
            label = "scale",
        )
    val progress = (elapsedMs.toFloat() / WavRecorder.MAX_DURATION_MS).coerceIn(0f, 1f)

    Column(
        modifier =
            Modifier
                .fillMaxSize()
                .background(MaterialTheme.colorScheme.background)
                .statusBarsPadding()
                .navigationBarsPadding()
                .padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        TextButton(onClick = onCancel, modifier = Modifier.align(Alignment.Start)) {
            Text("Cancel")
        }
        Spacer(Modifier.weight(1f))
        Box(
            modifier =
                Modifier
                    .size(28.dp)
                    .scale(scale)
                    .background(MaterialTheme.colorScheme.primary, CircleShape),
        )
        Spacer(Modifier.height(16.dp))
        Text(
            text = formatClock(elapsedMs),
            fontSize = 56.sp,
            fontWeight = FontWeight.Light,
        )
        Spacer(Modifier.height(12.dp))
        LinearProgressIndicator(
            progress = { progress },
            modifier = Modifier.fillMaxWidth(),
        )
        Spacer(Modifier.height(12.dp))
        Text(
            "Hold the phone near the baby. Stop as soon as the burst ends.",
            color = MaterialTheme.colorScheme.onSurfaceVariant,
            textAlign = TextAlign.Center,
        )
        Spacer(Modifier.weight(1f))
        Button(
            onClick = onStop,
            modifier =
                Modifier
                    .fillMaxWidth()
                    .height(64.dp),
            shape = RoundedCornerShape(18.dp),
        ) {
            Icon(Icons.Default.Stop, contentDescription = null)
            Spacer(Modifier.width(8.dp))
            Text("Stop", fontSize = 20.sp)
        }
    }
}

@Composable
private fun LabelScreen(
    durationMs: Long,
    relabel: Boolean,
    notes: String,
    onNotes: (String) -> Unit,
    onLabel: (OutcomeLabel) -> Unit,
    onDiscard: () -> Unit,
    onPlay: () -> Unit,
    playing: Boolean,
) {
    BackHandler(onBack = onDiscard)
    Column(
        modifier =
            Modifier
                .fillMaxSize()
                .background(MaterialTheme.colorScheme.background)
                .statusBarsPadding()
                .navigationBarsPadding()
                .padding(horizontal = 16.dp, vertical = 12.dp),
    ) {
        Text(
            text = if (relabel) "Update what happened" else "What happened afterward?",
            style = MaterialTheme.typography.headlineSmall,
            fontWeight = FontWeight.SemiBold,
        )
        Text(
            text = "${formatClock(durationMs)} clip  ·  tap the outcome you just observed",
            color = MaterialTheme.colorScheme.onSurfaceVariant,
        )
        Spacer(Modifier.height(8.dp))
        FilledTonalButton(onClick = onPlay) {
            Icon(if (playing) Icons.Default.Stop else Icons.Default.PlayArrow, contentDescription = null)
            Spacer(Modifier.width(6.dp))
            Text(if (playing) "Stop preview" else "Play clip")
        }
        Spacer(Modifier.height(8.dp))
        Column(
            modifier = Modifier.weight(1f).verticalScroll(rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            OutcomeLabel.entries.forEach { label ->
                Button(
                    onClick = { onLabel(label) },
                    modifier =
                        Modifier
                            .fillMaxWidth()
                            .height(64.dp),
                    shape = RoundedCornerShape(16.dp),
                    colors =
                        ButtonDefaults.buttonColors(
                            containerColor = labelColor(label),
                            contentColor = Color.White,
                        ),
                    contentPadding = PaddingValues(horizontal = 16.dp),
                ) {
                    Column(modifier = Modifier.fillMaxWidth()) {
                        Text(label.title, fontWeight = FontWeight.Bold, fontSize = 18.sp)
                        Text(label.subtitle, fontSize = 13.sp, color = Color.White.copy(alpha = 0.86f))
                    }
                }
            }
            OutlinedTextField(
                value = notes,
                onValueChange = onNotes,
                modifier = Modifier.fillMaxWidth(),
                label = { Text("Optional note") },
                placeholder = { Text("e.g. only a few swallows") },
            )
            TextButton(onClick = onDiscard, modifier = Modifier.align(Alignment.CenterHorizontally)) {
                Text(if (relabel) "Cancel" else "Discard recording")
            }
            Spacer(Modifier.height(8.dp))
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun LogScreen(
    episodes: List<Episode>,
    playingId: String?,
    snackbar: SnackbarHostState,
    onBack: () -> Unit,
    onPlay: (String) -> Unit,
    onRelabel: (Episode) -> Unit,
    onDelete: (String) -> Unit,
) {
    var pendingDelete by remember { mutableStateOf<Episode?>(null) }
    Scaffold(
        snackbarHost = { SnackbarHost(snackbar) },
        topBar = {
            TopAppBar(
                title = { Text("Labelled cries") },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = "Back")
                    }
                },
                colors =
                    TopAppBarDefaults.topAppBarColors(
                        containerColor = MaterialTheme.colorScheme.background,
                    ),
            )
        },
    ) { padding ->
        if (episodes.isEmpty()) {
            Box(
                modifier = Modifier.fillMaxSize().padding(padding),
                contentAlignment = Alignment.Center,
            ) {
                Text("Nothing labelled yet.", color = MaterialTheme.colorScheme.onSurfaceVariant)
            }
        } else {
            LazyColumn(
                modifier = Modifier.fillMaxSize().padding(padding),
                contentPadding = PaddingValues(16.dp),
                verticalArrangement = Arrangement.spacedBy(10.dp),
            ) {
                items(episodes, key = { it.id }) { episode ->
                    EpisodeRow(
                        episode = episode,
                        playing = playingId == episode.id,
                        onPlay = { onPlay(episode.id) },
                        onRelabel = { onRelabel(episode) },
                        onDelete = { pendingDelete = episode },
                    )
                }
            }
        }
    }

    pendingDelete?.let { episode ->
        AlertDialog(
            onDismissRequest = { pendingDelete = null },
            confirmButton = {
                TextButton(
                    onClick = {
                        onDelete(episode.id)
                        pendingDelete = null
                    },
                ) { Text("Delete") }
            },
            dismissButton = {
                TextButton(onClick = { pendingDelete = null }) { Text("Cancel") }
            },
            title = { Text("Delete this clip?") },
            text = { Text("The audio and label will be removed from the phone.") },
        )
    }
}

@Composable
private fun EpisodeRow(
    episode: Episode,
    playing: Boolean,
    onPlay: () -> Unit,
    onRelabel: () -> Unit,
    onDelete: () -> Unit,
) {
    Column(
        modifier =
            Modifier
                .fillMaxWidth()
                .background(MaterialTheme.colorScheme.surface, RoundedCornerShape(16.dp))
                .padding(12.dp),
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Column(modifier = Modifier.weight(1f)) {
                Text(episode.label.title, fontWeight = FontWeight.SemiBold)
                Text(
                    "${formatClock(episode.durationMs)}  ·  ${formatWhen(episode.recordedAtEpochMs)}",
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                    style = MaterialTheme.typography.bodySmall,
                )
                if (episode.label.dblSound != null) {
                    Text(
                        "DBL ${episode.label.dblSound}",
                        color = MaterialTheme.colorScheme.secondary,
                        style = MaterialTheme.typography.bodySmall,
                    )
                }
                if (episode.notes.isNotBlank()) {
                    Text(episode.notes, style = MaterialTheme.typography.bodySmall)
                }
            }
            IconButton(onClick = onPlay) {
                Icon(
                    if (playing) Icons.Default.Stop else Icons.Default.PlayArrow,
                    contentDescription = "Play",
                )
            }
            IconButton(onClick = onDelete) {
                Icon(Icons.Default.Delete, contentDescription = "Delete")
            }
        }
        TextButton(onClick = onRelabel) { Text("Relabel") }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun SettingsScreen(
    ageText: String,
    snackbar: SnackbarHostState,
    onAge: (String) -> Unit,
    onSave: () -> Unit,
    onBack: () -> Unit,
) {
    Scaffold(
        snackbarHost = { SnackbarHost(snackbar) },
        topBar = {
            TopAppBar(
                title = { Text("Settings") },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = "Back")
                    }
                },
                colors =
                    TopAppBarDefaults.topAppBarColors(
                        containerColor = MaterialTheme.colorScheme.background,
                    ),
            )
        },
    ) { padding ->
        Column(
            modifier =
                Modifier
                    .fillMaxSize()
                    .padding(padding)
                    .padding(20.dp)
                    .verticalScroll(rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(12.dp),
        ) {
            Text("Infant age is copied onto each new episode so later analysis can split by week.")
            OutlinedTextField(
                value = ageText,
                onValueChange = onAge,
                label = { Text("Age in weeks") },
                modifier = Modifier.fillMaxWidth(),
            )
            Button(onClick = onSave) { Text("Save") }
            Spacer(Modifier.height(8.dp))
            Text(
                "Clips stay on this phone as 16 kHz WAV + JSON. Export a zip when you want to check whether the DBL outcome classes occupy different regions of a simple feature space.",
                color = MaterialTheme.colorScheme.onSurfaceVariant,
            )
        }
    }
}

private fun labelColor(label: OutcomeLabel): Color {
    return when (label) {
        OutcomeLabel.FED -> Color(0xFFC75B39)
        OutcomeLabel.SLEPT -> Color(0xFF3D5A80)
        OutcomeLabel.DISCOMFORT -> Color(0xFF6A4C93)
        OutcomeLabel.GAS -> Color(0xFF2A9D8F)
        OutcomeLabel.BURPED -> Color(0xFF9C6B12)
        OutcomeLabel.OTHER -> Color(0xFF5C5C5C)
        OutcomeLabel.UNSURE -> Color(0xFF7A7A7A)
    }
}

private fun formatClock(ms: Long): String {
    val totalSeconds = (ms / 1000L).coerceAtLeast(0L)
    val minutes = totalSeconds / 60
    val seconds = totalSeconds % 60
    return "%d:%02d".format(minutes, seconds)
}

private fun formatWhen(epochMs: Long): String {
    val format = SimpleDateFormat("MMM d, h:mm a", Locale.getDefault())
    return format.format(Date(epochMs))
}

