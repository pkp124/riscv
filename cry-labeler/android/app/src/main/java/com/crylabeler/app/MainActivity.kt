package com.crylabeler.app

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.viewModels
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.Surface
import androidx.compose.ui.Modifier
import com.crylabeler.app.ui.CryLabelerApp
import com.crylabeler.app.ui.theme.CryLabelerTheme

class MainActivity : ComponentActivity() {
    private val viewModel: CryLabelerViewModel by viewModels {
        CryLabelerViewModel.factory(applicationContext)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            CryLabelerTheme {
                Surface(modifier = Modifier.fillMaxSize()) {
                    CryLabelerApp(viewModel = viewModel)
                }
            }
        }
    }
}
