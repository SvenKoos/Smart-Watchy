package com.company.esp32.alerts.app

import android.content.Intent
import android.os.Bundle
import android.preference.PreferenceFragment
import androidx.preference.PreferenceManager // Ersetzt android.preference.PreferenceManager
import androidx.appcompat.app.AppCompatActivity // Ersetzt android.support.v7.app.AppCompatActivity
import android.widget.Toast
import com.company.esp32.alerts.R
import java.util.regex.Pattern

class SettingsActivity : AppCompatActivity() {

    companion object {
        val PREF_KEY_RUN_AS_A_SERVICE = "pref_as_bg_service"
        val PREF_KEY_REMOTE_MAC_ADDRESS = "pref_remote_mac_address"
        val PREF_KEY_START_AT_BOOT = "pref_start_at_boot"
        val PREF_KEY_FLIP_DISPLAY_VERTICALLY = "pref_flip_vertically"
        val MAC_PATTERN = Pattern.compile("^([A-F0-9]{2}[:]?){5}[A-F0-9]{2}$")
        var prefRemoteMACAddress = "00:00:00:00:00:00"

        class SettingsFragment : PreferenceFragment() {

            override fun onCreate(savedInstanceState: Bundle?) {
                super.onCreate(savedInstanceState)
                addPreferencesFromResource(R.xml.preferences)

                val sharedPref = PreferenceManager.getDefaultSharedPreferences(activity)
                setRemoteMACAddressPrefSummary(sharedPref.getString(PREF_KEY_REMOTE_MAC_ADDRESS, "00:00:00:00:00:00") ?: "00:00:00:00:00:00")

                findPreference(PREF_KEY_REMOTE_MAC_ADDRESS)?.setOnPreferenceChangeListener { preference, value ->
                    val mac = (value as String).trim()
                    if (MAC_PATTERN.matcher(mac).find()) {
                        setRemoteMACAddressPrefSummary(mac)
                        true
                    } else {
                        Toast.makeText(activity, R.string.mac_format_error, Toast.LENGTH_LONG).show()
                        false
                    }
                }
            }

            fun setRemoteMACAddressPrefSummary(summary: String) {
                val pref = findPreference(PREF_KEY_REMOTE_MAC_ADDRESS)
                pref?.summary = summary
                prefRemoteMACAddress = summary
            }

            fun getRemoteMACAddressPrefSummary(): String {
                val pref = findPreference(PREF_KEY_REMOTE_MAC_ADDRESS)
                prefRemoteMACAddress = pref?.summary.toString()
                return prefRemoteMACAddress
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        fragmentManager.beginTransaction()
            .replace(android.R.id.content, SettingsFragment())
            .commit()
    }

    override fun onStart() {
        super.onStart()
        stopService(Intent(this, ForegroundService::class.java))
    }
}