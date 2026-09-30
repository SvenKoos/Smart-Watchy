package com.company.http.server.server

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.*
import android.content.pm.ServiceInfo
import android.os.Binder
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat
import com.company.esp32.alerts.R
import com.company.http.server.server.controllers.alertController
import com.company.http.server.server.repositories.AlertRepository
import com.company.http.server.server.repositories.AlertRepositoryImp
import com.company.http.server.server.services.AlertService
import com.company.http.server.server.model.Alert
import io.ktor.application.*
import io.ktor.features.*
import io.ktor.gson.*
import io.ktor.routing.*
import io.ktor.server.engine.*
import io.ktor.server.netty.*
import io.netty.util.internal.logging.InternalLoggerFactory
import io.netty.util.internal.logging.JdkLoggerFactory
import org.koin.dsl.module
import org.koin.ktor.ext.Koin
import timber.log.Timber

const val PORT = 8080

class HttpService : Service() {

    companion object {
        private const val SERVICE_ID = 9002
        private const val NOTIFICATION_CHANNEL_ID = "http_service_channel"
    }

    private var isServerStarted = false

    override fun onCreate() {
        super.onCreate()
        initNotificationChannel()
    }

    override fun onDestroy() {
        Timber.w("Http Service onDestroy() called!")
        super.onDestroy()
    }

    override fun onTaskRemoved(rootIntent: Intent?) {
        Timber.w("HttpService onTaskRemoved() - App removed from Recent Apps!")
        super.onTaskRemoved(rootIntent)
    }

    private fun initNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val notificationMgr = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            val notificationChannel = NotificationChannel(
                NOTIFICATION_CHANNEL_ID,
                "HTTP Server Service",
                NotificationManager.IMPORTANCE_LOW
            )
            notificationChannel.description = "Keeps HTTP-Server active for notification requests"
            notificationMgr.createNotificationChannel(notificationChannel)
        }
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        val notification = NotificationCompat.Builder(this, NOTIFICATION_CHANNEL_ID)
            .setSmallIcon(R.drawable.ic_stat_espressif)
            .setContentTitle("ESP HTTP Server")
            .setContentText("HTTP-Server runs on Port $PORT")
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .setOngoing(true)
            .build()

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            startForeground(SERVICE_ID, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_DATA_SYNC)
        } else {
            startForeground(SERVICE_ID, notification)
        }

        if (!isServerStarted) {
            isServerStarted = true
            Thread {
                InternalLoggerFactory.setDefaultFactory(JdkLoggerFactory.INSTANCE)
                embeddedServer(Netty, PORT) {
                    install(ContentNegotiation) { gson {} }
                    handleException()
                    install(Koin) {
                        modules(
                            module {
                                single<AlertRepository> { AlertRepositoryImp() }
                                single { AlertService() }
                            }
                        )
                    }
                    install(Routing) {
                        alertController(this@HttpService)
                    }
                }.start(wait = true)
            }.start()
        }

        return START_STICKY
    }

    private val myBinder = MyLocalBinder()

    override fun onBind(intent: Intent): IBinder? {
        return myBinder
    }

    inner class MyLocalBinder : Binder() {
        fun getService(): HttpService {
            return this@HttpService
        }
    }

    class BootCompletedReceiver : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            if (intent.action == Intent.ACTION_BOOT_COMPLETED) {
                Timber.d("BootCompletedReceiver: starting service HttpService")
                val serviceIntent = Intent(context, HttpService::class.java)
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    context.startForegroundService(serviceIntent)
                } else {
                    context.startService(serviceIntent)
                }
            }
        }
    }

    fun sendAlert(
        notificationId: Int,
        notificationAppName: String,
        notificationTitle: String,
        notificationBody: String,
        notificationTimestamp: String): Int {
        if (alertService != null) {
            val alert = Alert(notificationId, notificationAppName, notificationTitle, notificationBody, notificationTimestamp)
            Timber.d("sendAlert {app=${notificationAppName},id=${notificationId},title=$notificationTitle,body=$notificationBody,posted=${notificationTimestamp}}")
            alertService?.addAlert(alert)
        }
        return 0
    }

    var alertService: AlertService? = null
}