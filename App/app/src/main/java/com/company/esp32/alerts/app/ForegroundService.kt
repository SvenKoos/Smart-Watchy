package com.company.esp32.alerts.app

import android.annotation.TargetApi
import android.app.*
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.net.Uri
import android.os.Build
import android.os.IBinder
import android.preference.PreferenceManager
import android.support.v4.app.NotificationCompat
import android.support.v4.content.LocalBroadcastManager
import timber.log.Timber
import com.company.esp32.alerts.BuildConfig
import com.company.esp32.alerts.MainActivity
import com.company.esp32.alerts.R
import java.io.*
import java.net.*
import java.text.DateFormat
import java.text.SimpleDateFormat
import java.util.*
import android.content.ServiceConnection
import android.content.*

/**
 *
 */
class ForegroundService : Service() {

    companion object {
        val NOTIFICATION_DISPLAY_TIMEOUT = 2 * 60 * 1000 //2 minutes
        val SERVICE_ID = 9001
        val NOTIFICATION_CHANNEL = BuildConfig.APPLICATION_ID
        val VESPA_DEVICE_ADDRESS =
            "00:00:00:00:00:00"//""24:0A:C4:13:58:EA" // <--- YOUR ESP32 MAC address here
        val formatter = SimpleDateFormat.getTimeInstance(DateFormat.SHORT)
    }

    private var startId = 0

    var lastPost: String = ""

    /**
     * Called by the system when the service is first created.  Do not call this method directly.
     */
    override fun onCreate() {
        super.onCreate()
        initNotificationChannel()
        Timber.w("onCreate")
        val remoteMacAddress = PreferenceManager.getDefaultSharedPreferences(this)
            .getString(SettingsActivity.PREF_KEY_REMOTE_MAC_ADDRESS, VESPA_DEVICE_ADDRESS)

        val intentFilter = IntentFilter(NotificationListener.EXTRA_ACTION)
        LocalBroadcastManager.getInstance(this).registerReceiver(localReceiver, intentFilter)
    }

    @TargetApi(Build.VERSION_CODES.O)
    private fun initNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val notificationMgr =
                getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            val notificationChannel = NotificationChannel(
                NOTIFICATION_CHANNEL,
                BuildConfig.APPLICATION_ID,
                NotificationManager.IMPORTANCE_LOW
            )
            notificationChannel.description = getString(R.string.channel_desc)
            notificationChannel.enableLights(false)
            notificationChannel.enableVibration(false)
            notificationMgr.createNotificationChannel(notificationChannel)
        }
    }

    override fun onDestroy() {
        Timber.w("onDestroy")
        startId = 0

        LocalBroadcastManager.getInstance(this).unregisterReceiver(localReceiver)
        (getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager).cancelAll()

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val notificationMgr =
                getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            notificationMgr.deleteNotificationChannel(NOTIFICATION_CHANNEL)
        }
        super.onDestroy()
    }

    /**
     * Create/Update the notification
     */
    private fun notify(contentText: String): Notification {
        // Launch the MainActivity when user taps on the Notification
        val pendingIntent = PendingIntent.getActivity(
            this, 0, Intent(this, MainActivity::class.java), PendingIntent.FLAG_UPDATE_CURRENT
        )

        val notification = NotificationCompat.Builder(this, NOTIFICATION_CHANNEL)
            .setSmallIcon(R.drawable.ic_stat_espressif)
            .setContentTitle(getString(R.string.app_name))
            .setContentText(contentText)
            .setContentIntent(pendingIntent)
            .setSound(Uri.EMPTY)
            .setOnlyAlertOnce(true)
            .build()
        (getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager).notify(
            SERVICE_ID,
            notification
        )
        return notification
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {

        Timber.w("onStartCommand {intent=${intent != null},flags=$flags,startId=$startId}")
        if (intent == null || this.startId != 0) {
            //service restarted
            Timber.w("onStartCommand - already running")
        } else {
            //started by intent or pending intent
            this.startId = startId
            val notification = notify("Scanning...")
            startForeground(SERVICE_ID, notification)
        }
        return START_STICKY
    }

    override fun onBind(intent: Intent?): IBinder? {
        return null
    }

    var localReceiver: BroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            if (intent != null) {
                val notificationId =
                    intent.getIntExtra(NotificationListener.EXTRA_NOTIFICATION_ID_INT, 0)
                val notificationAppName = intent.getStringExtra(NotificationListener.EXTRA_APP_NAME)
                val notificationTitle = intent.getStringExtra(NotificationListener.EXTRA_TITLE)
                val notificationBody = intent.getStringExtra(NotificationListener.EXTRA_BODY)
                val notificationTimestamp =
                    intent.getStringExtra(NotificationListener.EXTRA_TIMESTAMP)
                val notificationDismissed =
                    intent.getBooleanExtra(NotificationListener.EXTRA_NOTIFICATION_DISMISSED, false)

                Timber.d("onReceive {app=${notificationAppName},id=${notificationId},title=$notificationTitle,body=$notificationBody,posted=${notificationTimestamp}}")

                if (notificationDismissed) {
                    lastPost = notificationTimestamp
                } else {
                    lastPost = notificationTimestamp

                    val intent = Intent(getApplicationContext(), com.company.http.server.server.HttpService::class.java)
                    bindService(intent, myConnection, Context.BIND_AUTO_CREATE)
                    myService?.sendAlert(notificationId, notificationAppName, notificationTitle, notificationBody, notificationTimestamp)
                    /*
                    sendPostRequest(
                        notificationId,
                        notificationAppName,
                        notificationTitle,
                        notificationBody,
                        notificationTimestamp
                    )
                     */
                }
            }
        }
    }
/*
    fun sendPostRequest(id: Int, appName: String, title: String, body: String, timestamp: String) {
        var reqParam =
            URLEncoder.encode("id", "UTF-8") + "=" + URLEncoder.encode(id.toString(), "UTF-8")
        reqParam += "&" + URLEncoder.encode("appName", "UTF-8") + "=" + URLEncoder.encode(
            appName,
            "UTF-8"
        )
        reqParam += "&" + URLEncoder.encode("title", "UTF-8") + "=" + URLEncoder.encode(
            title,
            "UTF-8"
        )
        reqParam += "&" + URLEncoder.encode("body", "UTF-8") + "=" + URLEncoder.encode(
            body,
            "UTF-8"
        )
        reqParam += "&" + URLEncoder.encode("timestamp", "UTF-8") + "=" + URLEncoder.encode(
            timestamp,
            "UTF-8"
        )

        val mURL = URL("http://127.0.0.1:8080/alert")

        val policy = StrictMode.ThreadPolicy.Builder().permitAll().build()
        StrictMode.setThreadPolicy(policy)

        val conn = mURL.openConnection() as HttpURLConnection
        conn.requestMethod = "POST"
        conn.doOutput = true
        conn.useCaches = false

        val postData: ByteArray = reqParam.toByteArray(StandardCharsets.UTF_8)

        conn.setRequestProperty("charset", "utf-8")
        conn.setRequestProperty("Content-length", postData.size.toString())
        conn.setRequestProperty("Content-Type", "application/json")

        try {
            val outputStream: DataOutputStream = DataOutputStream(conn.outputStream)
            outputStream.write(postData)
            outputStream.flush()
        } catch (exception: Exception) {
        }

        if (conn.responseCode != HttpURLConnection.HTTP_OK && conn.responseCode != HttpURLConnection.HTTP_CREATED) {
            try {
                val inputStream: DataInputStream = DataInputStream(conn.inputStream)
                val reader: BufferedReader = BufferedReader(InputStreamReader(inputStream))
                val output: String = reader.readLine()
            } catch (exception: Exception) {
            }
        }
    }
*/
    var myService: com.company.http.server.server.HttpService? = null
    var isBound = false

    private val myConnection = object : ServiceConnection {
        override fun onServiceConnected(className: ComponentName,
                                        service: IBinder) {
            val binder = service as com.company.http.server.server.HttpService.MyLocalBinder
            myService = binder.getService()
            isBound = true
        }

        override fun onServiceDisconnected(name: ComponentName) {
            isBound = false
        }
    }
}