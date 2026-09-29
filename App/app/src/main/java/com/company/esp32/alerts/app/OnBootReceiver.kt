package com.company.esp32.alerts.app

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.Build
import androidx.preference.PreferenceManager // Ersetzt android.preference.PreferenceManager

/**
 * Invoked after the system boots up
 */
class OnBootReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent?) {
        // Sicherheits-Check: Nur ausführen, wenn die Action wirklich BOOT_COMPLETED ist
        if (intent?.action != Intent.ACTION_BOOT_COMPLETED) {
            return
        }

        val startAtBoot = PreferenceManager.getDefaultSharedPreferences(context)
            .getBoolean(SettingsActivity.PREF_KEY_START_AT_BOOT, false)
        if (startAtBoot) {
            val serviceIntent = Intent(context, ForegroundService::class.java)
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                context.startForegroundService(serviceIntent)
            } else {
                context.startService(serviceIntent)
            }
        }
    }
}