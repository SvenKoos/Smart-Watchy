package com.company.esp32.alerts.app

import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import androidx.annotation.RequiresApi
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import android.text.SpannableString
import timber.log.Timber
import java.time.*
import java.time.Instant

class NotificationListener : NotificationListenerService() {

    companion object {
        val EXTRA_ACTION = "ESP"
        val EXTRA_NOTIFICATION_DISMISSED = "EXTRA_NOTIFICATION_DISMISSED"
        val EXTRA_APP_NAME = "EXTRA_APP_NAME"
        val EXTRA_NOTIFICATION_ID_INT = "EXTRA_NOTIFICATION_ID_INT"
        val EXTRA_TITLE = "EXTRA_TITLE"
        val EXTRA_BODY = "EXTRA_BODY"
        val EXTRA_TIMESTAMP = "EXTRA_TIMESTAMP"
    }

    @RequiresApi(Build.VERSION_CODES.O)
    override fun onNotificationPosted(sbn: StatusBarNotification) {
        val notification = sbn.notification
        val bundle: Bundle? = notification?.extras
        val titleObj = bundle?.get("android.title")
        val title: String = when (titleObj) {
            is String -> titleObj
            is SpannableString -> titleObj.toString()
            else -> ""
        }

        val body: String = bundle?.getCharSequence("android.text")?.toString() ?: ""

        // Absicherung gegen NameNotFoundException (z.B. Work Profile Apps)
        val appName = try {
            val appInfo = packageManager.getApplicationInfo(sbn.packageName, PackageManager.GET_META_DATA)
            packageManager.getApplicationLabel(appInfo).toString()
        } catch (e: PackageManager.NameNotFoundException) {
            sbn.packageName
        }

        Timber.d("onNotificationPosted {app=${appName},id=${sbn.id},title=$title,body=$body,posted=${sbn.postTime},package=${sbn.packageName}}")

        val allowedPackages: MutableSet<String> = MainApplication.sharedPrefs.getStringSet(MainApplication.PREFS_KEY_ALLOWED_PACKAGES, mutableSetOf()) ?: mutableSetOf()

        if (sbn.id != ForegroundService.SERVICE_ID && allowedPackages.contains(sbn.packageName) && title.isNotEmpty()) {
            val intent = Intent(EXTRA_ACTION).apply {
                putExtra(EXTRA_NOTIFICATION_ID_INT, sbn.id)
                putExtra(EXTRA_APP_NAME, appName)
                putExtra(EXTRA_TITLE, title)
                putExtra(EXTRA_BODY, body)
                putExtra(EXTRA_NOTIFICATION_DISMISSED, false)
                val dt = Instant.ofEpochMilli(sbn.postTime)
                    .atZone(ZoneId.systemDefault())
                    .toLocalDateTime()
                putExtra(EXTRA_TIMESTAMP, dt.toString())
            }
            LocalBroadcastManager.getInstance(this).sendBroadcast(intent)
        }
    }

    @RequiresApi(Build.VERSION_CODES.O)
    override fun onNotificationRemoved(sbn: StatusBarNotification) {
        super.onNotificationRemoved(sbn)
        val notification = sbn.notification
        val bundle: Bundle? = notification?.extras
        val titleObj = bundle?.get("android.title")
        val title: String = when (titleObj) {
            is String -> titleObj
            is SpannableString -> titleObj.toString()
            else -> ""
        }

        val body: String = bundle?.getCharSequence("android.text")?.toString() ?: ""

        // Absicherung gegen NameNotFoundException auch beim Entfernen von Notifications
        val appName = try {
            val appInfo = packageManager.getApplicationInfo(sbn.packageName, PackageManager.GET_META_DATA)
            packageManager.getApplicationLabel(appInfo).toString()
        } catch (e: PackageManager.NameNotFoundException) {
            sbn.packageName
        }

        val allowedPackages: MutableSet<String> = MainApplication.sharedPrefs.getStringSet(MainApplication.PREFS_KEY_ALLOWED_PACKAGES, mutableSetOf()) ?: mutableSetOf()

        if (sbn.id != ForegroundService.SERVICE_ID && allowedPackages.contains(sbn.packageName) && title.isNotEmpty()) {
            val intent = Intent(EXTRA_ACTION).apply {
                putExtra(EXTRA_NOTIFICATION_ID_INT, sbn.id)
                putExtra(EXTRA_APP_NAME, appName)
                putExtra(EXTRA_TITLE, title)
                putExtra(EXTRA_BODY, body)
                putExtra(EXTRA_NOTIFICATION_DISMISSED, true)
                val dt = Instant.ofEpochMilli(sbn.postTime)
                    .atZone(ZoneId.systemDefault())
                    .toLocalDateTime()
                putExtra(EXTRA_TIMESTAMP, dt.toString())
            }
            LocalBroadcastManager.getInstance(this).sendBroadcast(intent)
        }
    }
}