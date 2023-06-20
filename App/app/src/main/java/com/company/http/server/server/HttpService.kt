package com.company.http.server.server

import android.app.Service
import android.content.*
import android.os.IBinder
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
import android.os.Binder
import timber.log.Timber

// Forward https://stackoverflow.com/a/14684485/3479489
const val PORT = 8080

class HttpService : Service() {
    override fun onCreate() {
        super.onCreate()
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

    // override fun onBind(intent: Intent): IBinder? = null
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
                context.startService(Intent(context, HttpService::class.java))
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
