package com.company.esp32.alerts.app

import android.Manifest
import android.app.*
import android.content.*
import android.content.pm.PackageManager
import android.content.pm.ServiceInfo
import android.net.Uri
import android.os.Build
import android.os.IBinder
import androidx.core.content.ContextCompat
import androidx.core.app.NotificationCompat
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import timber.log.Timber
import com.company.esp32.alerts.BuildConfig
import com.company.esp32.alerts.MainActivity
import com.company.esp32.alerts.R

class ForegroundService : Service() {

    companion object {
        val SERVICE_ID = 9001
        val NOTIFICATION_CHANNEL = BuildConfig.APPLICATION_ID
    }

    private var startId = 0
    var lastPost: String = ""

    override fun onCreate() {
        super.onCreate()
        initNotificationChannel()
        Timber.w("onCreate")

        val intentFilter = IntentFilter(NotificationListener.EXTRA_ACTION)
        LocalBroadcastManager.getInstance(this).registerReceiver(localReceiver, intentFilter)
    }

    private fun initNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val notificationMgr = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            val notificationChannel = NotificationChannel(
                NOTIFICATION_CHANNEL,
                BuildConfig.APPLICATION_ID,
                NotificationManager.IMPORTANCE_LOW
            )
            notificationChannel.description = getString(R.string.channel_desc)
            notificationMgr.createNotificationChannel(notificationChannel)
        }
    }

    override fun onDestroy() {
        Timber.w("onDestroy")
        startId = 0
        LocalBroadcastManager.getInstance(this).unregisterReceiver(localReceiver)
        super.onDestroy()
    }

    private fun notify(contentText: String): Notification {
        val flags = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        } else {
            PendingIntent.FLAG_UPDATE_CURRENT
        }

        val pendingIntent = PendingIntent.getActivity(
            this, 0, Intent(this, MainActivity::class.java), flags
        )

        return NotificationCompat.Builder(this, NOTIFICATION_CHANNEL)
            .setSmallIcon(R.drawable.ic_stat_espressif)
            .setContentTitle(getString(R.string.app_name))
            .setContentText(contentText)
            .setContentIntent(pendingIntent)
            .setSound(Uri.EMPTY)
            .setOnlyAlertOnce(true)
            .build()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        Timber.w("onStartCommand")
        if (intent == null || this.startId != 0) {
            Timber.w("onStartCommand - already running")
        } else {
            this.startId = startId
            val notification = notify("Scanning...")

            // Abfangen fehlender Berechtigungen unter Android 14+ vor startForeground()
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT)
                    != PackageManager.PERMISSION_GRANTED) {
                    Timber.e("Cannot start foreground service: BLUETOOTH_CONNECT permission missing!")
                    stopSelf()
                    return START_NOT_STICKY
                }
            }

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                startForeground(SERVICE_ID, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE)
            } else {
                startForeground(SERVICE_ID, notification)
            }
        }
        return START_STICKY
    }

    override fun onBind(intent: Intent?): IBinder? = null

    var localReceiver: BroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            if (intent != null) {
                val notificationId = intent.getIntExtra(NotificationListener.EXTRA_NOTIFICATION_ID_INT, 0)
                val notificationAppName = intent.getStringExtra(NotificationListener.EXTRA_APP_NAME) ?: ""
                val notificationTitle = intent.getStringExtra(NotificationListener.EXTRA_TITLE) ?: ""
                val notificationBody = intent.getStringExtra(NotificationListener.EXTRA_BODY) ?: ""
                val notificationTimestamp = intent.getStringExtra(NotificationListener.EXTRA_TIMESTAMP) ?: ""
                val notificationDismissed = intent.getBooleanExtra(NotificationListener.EXTRA_NOTIFICATION_DISMISSED, false)

                if (!notificationDismissed) {
                    lastPost = notificationTimestamp
                    val serviceIntent = Intent(applicationContext, com.company.http.server.server.HttpService::class.java)
                    bindService(serviceIntent, myConnection, Context.BIND_AUTO_CREATE)
                    myService?.sendAlert(notificationId, notificationAppName, notificationTitle, notificationBody, notificationTimestamp)
                }
            }
        }
    }

    var myService: com.company.http.server.server.HttpService? = null
    var isBound = false

    private val myConnection = object : ServiceConnection {
        override fun onServiceConnected(className: ComponentName, service: IBinder) {
            val binder = service as com.company.http.server.server.HttpService.MyLocalBinder
            myService = binder.getService()
            isBound = true
        }

        override fun onServiceDisconnected(name: ComponentName) {
            isBound = false
        }
    }
}