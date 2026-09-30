package com.company.esp32.alerts.app

import android.content.Context
import android.content.SharedPreferences
import androidx.multidex.MultiDexApplication
import timber.log.Timber
import com.company.esp32.alerts.BuildConfig

class MainApplication : MultiDexApplication() {

    companion object {
        val PREFS_KEY_ALLOWED_PACKAGES = "PREFS_KEY_ALLOWED_PACKAGES"
        lateinit var sharedPrefs: SharedPreferences
    }

    override fun onCreate() {
        super.onCreate()

        sharedPrefs = getSharedPreferences(BuildConfig.APPLICATION_ID, Context.MODE_PRIVATE)

        if (BuildConfig.DEBUG) {
            Timber.plant(Timber.DebugTree())
            Timber.plant(FileLoggingTree(this))
        }

        val defaultHandler = Thread.getDefaultUncaughtExceptionHandler()
        Thread.setDefaultUncaughtExceptionHandler { thread, throwable ->
            Timber.e(throwable, "Uncaught Exception in Thread: ${thread.name}")
            defaultHandler?.uncaughtException(thread, throwable)
        }
    }
}