package com.company.esp32.alerts

import android.Manifest
import android.app.ActivityManager
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.PowerManager
import android.provider.Settings
import android.view.Menu
import android.view.MenuItem
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.core.app.NotificationManagerCompat
import androidx.preference.PreferenceManager
import com.google.android.material.floatingactionbutton.FloatingActionButton
import timber.log.Timber
import com.company.esp32.alerts.app.ForegroundService
import com.company.esp32.alerts.app.MainApplication
import com.company.esp32.alerts.app.SettingsActivity
import com.company.http.server.server.HttpService
import java.util.*

class MainActivity : AppCompatActivity() {

    companion object {
        private const val REQUEST_PERMISSIONS_CODE = 1001
    }

    lateinit var fab: FloatingActionButton
    private var optionsMenu: Menu? = null
    var alertDialog: AlertDialog? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        fab = findViewById(R.id.fab)

        PreferenceManager.setDefaultValues(this, R.xml.preferences, false)

        fab.setOnClickListener {
            val enabled = NotificationManagerCompat.getEnabledListenerPackages(this).contains(BuildConfig.APPLICATION_ID)
            Timber.d("Notification Listener Enabled $enabled")

            if (alertDialog == null || !(alertDialog!!.isShowing)) {
                if (enabled) {
                    val installedApps = packageManager.getInstalledApplications(PackageManager.GET_META_DATA)
                    installedApps.sortWith { a, b ->
                        val nameA = try { packageManager.getApplicationLabel(a).toString() } catch (e: Exception) { a.packageName }
                        val nameB = try { packageManager.getApplicationLabel(b).toString() } catch (e: Exception) { b.packageName }
                        nameA.compareTo(nameB)
                    }
                    val names: Array<String> = installedApps.map { applicationInfo ->
                        try {
                            packageManager.getApplicationLabel(applicationInfo).toString()
                        } catch (e: Exception) {
                            applicationInfo.packageName
                        }
                    }.toTypedArray()

                    val prefsAllowedPackages: MutableSet<String> = MainApplication.sharedPrefs.getStringSet(MainApplication.PREFS_KEY_ALLOWED_PACKAGES, mutableSetOf()) ?: mutableSetOf()
                    val checkedItems = BooleanArray(installedApps.size)
                    for (i in names.indices) {
                        checkedItems[i] = prefsAllowedPackages.contains(installedApps[i].packageName)
                    }

                    val modifiedList = ArrayList<String>()
                    modifiedList.addAll(prefsAllowedPackages)

                    val builder: AlertDialog.Builder = AlertDialog.Builder(this)
                        .setTitle(R.string.choose_app)
                        .setPositiveButton(android.R.string.ok) { _, _ ->
                            MainApplication.sharedPrefs.edit().putStringSet(MainApplication.PREFS_KEY_ALLOWED_PACKAGES, modifiedList.toSet()).commit()
                        }
                        .setNegativeButton(android.R.string.cancel, null)
                        .setMultiChoiceItems(names, checkedItems) { _, position, checked ->
                            if (checked) {
                                modifiedList.add(installedApps[position].packageName)
                            } else {
                                modifiedList.remove(installedApps[position].packageName)
                            }
                        }
                        .setOnDismissListener { alertDialog = null }
                        .setOnCancelListener { alertDialog = null }
                    alertDialog = builder.create()
                    alertDialog!!.show()
                } else {
                    val builder: AlertDialog.Builder = AlertDialog.Builder(this)
                        .setTitle(R.string.choose_app)
                        .setMessage("Looks like you must first grant this app access to notifications. Do you want to continue?")
                        .setNegativeButton(android.R.string.no, null)
                        .setPositiveButton(android.R.string.yes) { _, _ ->
                            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP_MR1) {
                                startActivity(Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS))
                            } else {
                                startActivity(Intent("android.settings.ACTION_NOTIFICATION_LISTENER_SETTINGS"))
                            }
                        }
                        .setOnDismissListener { alertDialog = null }
                        .setOnCancelListener { alertDialog = null }
                    alertDialog = builder.create()
                    alertDialog!!.show()
                }
            }
        }

        checkAndRequestBatteryOptimization()
        startAppService(Intent(this, HttpService::class.java))
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menuInflater.inflate(R.menu.main_menu, menu)
        this.optionsMenu = menu
        return true
    }

    override fun onPrepareOptionsMenu(menu: Menu): Boolean {
        val serviceRunning = isServiceRunning(ForegroundService::class.java)
        menu.findItem(R.id.menu_item_kill)?.isVisible = serviceRunning
        menu.findItem(R.id.menu_item_start)?.isVisible = !serviceRunning
        return super.onPrepareOptionsMenu(menu)
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        return when (item.itemId) {
            R.id.menu_item_prefs -> {
                showPreferences()
                true
            }
            R.id.menu_item_kill -> {
                stopService(Intent(this, ForegroundService::class.java))
                invalidateOptionsMenu() // Menü-Zustand aktualisieren
                true
            }
            R.id.menu_item_start -> {
                checkPermissionAndStartForegroundService()
                invalidateOptionsMenu() // Menü-Zustand aktualisieren
                true
            }
            else -> super.onOptionsItemSelected(item)
        }
    }

    override fun onStart() {
        super.onStart()
        checkPermissionAndStartForegroundService()
        invalidateOptionsMenu()
        Timber.w("onStart")
    }

    override fun onDestroy() {
        super.onDestroy()
        Timber.w("MainActivity onDestroy - ForegroundService stays active.")
    }

    private fun isServiceRunning(serviceClass: Class<*>): Boolean {
        val manager = getSystemService(Context.ACTIVITY_SERVICE) as ActivityManager
        @Suppress("DEPRECATION")
        for (service in manager.getRunningServices(Int.MAX_VALUE)) {
            if (serviceClass.name == service.service.className) {
                return true
            }
        }
        return false
    }

    private fun checkPermissionAndStartForegroundService() {
        val permissionsToRequest = mutableListOf<String>()

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT)
                != PackageManager.PERMISSION_GRANTED) {
                permissionsToRequest.add(Manifest.permission.BLUETOOTH_CONNECT)
            }
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS)
                != PackageManager.PERMISSION_GRANTED) {
                permissionsToRequest.add(Manifest.permission.POST_NOTIFICATIONS)
            }
        }

        if (permissionsToRequest.isNotEmpty()) {
            ActivityCompat.requestPermissions(
                this,
                permissionsToRequest.toTypedArray(),
                REQUEST_PERMISSIONS_CODE
            )
            return
        }

        startAppService(Intent(this, ForegroundService::class.java))
    }

    private fun checkAndRequestBatteryOptimization() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            val powerManager = getSystemService(POWER_SERVICE) as PowerManager
            if (!powerManager.isIgnoringBatteryOptimizations(packageName)) {
                try {
                    val intent = Intent(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS).apply {
                        data = Uri.parse("package:$packageName")
                    }
                    startActivity(intent)
                } catch (e: Exception) {
                    Timber.e(e, "Konnte Dialog zur Akku-Optimierung nicht öffnen.")
                }
            }
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_PERMISSIONS_CODE) {
            startAppService(Intent(this, ForegroundService::class.java))
        }
    }

    private fun startAppService(intent: Intent) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(intent)
        } else {
            startService(intent)
        }
    }

    fun showPreferences() {
        startActivity(Intent(this, SettingsActivity::class.java))
    }
}