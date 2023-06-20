package com.company.esp32.alerts.app

import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import android.support.annotation.RequiresApi
import android.support.v4.content.LocalBroadcastManager
import android.text.SpannableString
import timber.log.Timber
import java.time.*
import java.time.Instant

/**
 *
 */
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

    /**
     * Implement this method to learn about new notifications as they are posted by apps.
     *
     * @param sbn A data structure encapsulating the original [android.app.Notification]
     * object as well as its identifying information (tag and id) and source
     * (package name).
     */
    @RequiresApi(Build.VERSION_CODES.O)
    override fun onNotificationPosted(sbn: StatusBarNotification) {
        val notification = sbn.notification
        val ticker = notification?.tickerText
        val bundle: Bundle? = notification?.extras
        val titleObj = bundle?.get("android.title")
        val title: String

        when (titleObj) {
            is String -> title = titleObj
            is SpannableString -> title = titleObj.toString()
            else -> title = ""
        }

        val body: String = bundle?.getCharSequence("android.text").toString()

        val appInfo = applicationContext.packageManager.getApplicationInfo(sbn.packageName, PackageManager.GET_META_DATA)
        val appName = applicationContext.packageManager.getApplicationLabel(appInfo)
        Timber.d("onNotificationPosted {app=${appName},id=${sbn.id},title=$title,body=$body,posted=${sbn.postTime},package=${sbn.packageName}}")

        val allowedPackages: MutableSet<String> = MainApplication.sharedPrefs.getStringSet(MainApplication.PREFS_KEY_ALLOWED_PACKAGES, mutableSetOf())

        // broadcast StatusBarNotification (exclude own notifications)
        if (sbn.id != ForegroundService.SERVICE_ID
                && allowedPackages.contains(sbn.packageName) && (title.isNotEmpty())) {
            val intent = Intent(EXTRA_ACTION)
            intent.putExtra(EXTRA_NOTIFICATION_ID_INT, sbn.id)
            intent.putExtra(EXTRA_APP_NAME, appName)
            intent.putExtra(EXTRA_TITLE, title)
            intent.putExtra(EXTRA_BODY, body)
            intent.putExtra(EXTRA_NOTIFICATION_DISMISSED, false)
            val dt = Instant.ofEpochMilli(sbn.postTime)
                .atZone(ZoneId.systemDefault())
                .toLocalDateTime()
            intent.putExtra(EXTRA_TIMESTAMP, dt.toString())
            LocalBroadcastManager.getInstance(this).sendBroadcast(intent)
        }
    }

    @RequiresApi(Build.VERSION_CODES.O)
    override fun onNotificationRemoved(sbn: StatusBarNotification) {
        super.onNotificationRemoved(sbn)
        val notification = sbn.notification
        val ticker = notification?.tickerText
        val bundle: Bundle? = notification?.extras
        val titleObj = bundle?.get("android.title")
        val title: String

        when (titleObj) {
            is String -> title = titleObj
            is SpannableString -> title = titleObj.toString()
            else -> title = ""
        }

        val body: String = bundle?.getCharSequence("android.text").toString()

        val appInfo = applicationContext.packageManager.getApplicationInfo(sbn.packageName, PackageManager.GET_META_DATA)
        val appName = applicationContext.packageManager.getApplicationLabel(appInfo)
        Timber.d("onNotificationPosted {app=${appName},id=${sbn.id},ticker=$ticker,title=$title,body=$body,posted=${sbn.postTime},package=${sbn.packageName}}")

        val allowedPackages: MutableSet<String> = MainApplication.sharedPrefs.getStringSet(MainApplication.PREFS_KEY_ALLOWED_PACKAGES, mutableSetOf())

        Timber.d("onNotificationRemoved {app=${applicationContext.packageManager.getApplicationLabel(appInfo)},id=${sbn.id},title=$title,body=$body,posted=${sbn.postTime},package=${sbn.packageName}}")

        // broadcast StatusBarNotification (exclude own notifications)
        if (sbn.id != ForegroundService.SERVICE_ID
                && allowedPackages.contains(sbn.packageName) && (title.isNotEmpty())) {
            val intent = Intent(EXTRA_ACTION)
            intent.putExtra(EXTRA_NOTIFICATION_ID_INT, sbn.id)
            intent.putExtra(EXTRA_APP_NAME, appName)
            intent.putExtra(EXTRA_TITLE, title)
            intent.putExtra(EXTRA_BODY, body)
            intent.putExtra(EXTRA_NOTIFICATION_DISMISSED, true)
            val dt = Instant.ofEpochMilli(sbn.postTime)
                .atZone(ZoneId.systemDefault())
                .toLocalDateTime()
            intent.putExtra(EXTRA_TIMESTAMP, dt.toString())
            LocalBroadcastManager.getInstance(this).sendBroadcast(intent)
        }
    }


}