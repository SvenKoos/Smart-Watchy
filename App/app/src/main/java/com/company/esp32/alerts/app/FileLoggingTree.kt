package com.company.esp32.alerts.app

import android.content.Context
import android.util.Log
import timber.log.Timber
import java.io.File
import java.io.FileWriter
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class FileLoggingTree(private val context: Context) : Timber.DebugTree() {
    override fun log(priority: Int, tag: String?, message: String, t: Throwable?) {
        try {
            val logFile = File(context.getExternalFilesDir(null), "app_debug.log")
            val timeStamp = SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.getDefault()).format(Date())
            val writer = FileWriter(logFile, true)
            
            writer.append("$timeStamp [$tag] $message\n")
            if (t != null) {
                writer.append(Log.getStackTraceString(t))
                writer.append("\n")
            }
            writer.flush()
            writer.close()
        } catch (e: Exception) {
            // Ignorieren, falls Dateizugriff fehlschlägt
        }
    }
}