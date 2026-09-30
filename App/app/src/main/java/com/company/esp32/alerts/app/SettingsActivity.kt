package com.company.esp32.alerts.app

import android.os.Bundle
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.preference.Preference
import androidx.preference.PreferenceFragmentCompat
import androidx.preference.PreferenceManager
import com.company.esp32.alerts.R
import java.util.regex.Pattern

class SettingsActivity : AppCompatActivity() {

    companion object {
        val PREF_KEY_REMOTE_MAC_ADDRESS = "pref_remote_mac_address"
        val MAC_PATTERN: Pattern = Pattern.compile("^([A-F0-9]{2}[:]?){5}[A-F0-9]{2}$", Pattern.CASE_INSENSITIVE)
        var prefRemoteMACAddress = "00:00:00:00:00:00"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // AndroidX PreferenceFragmentCompat in die Activity einbetten
        if (savedInstanceState == null) {
            supportFragmentManager
                .beginTransaction()
                .replace(android.R.id.content, SettingsFragment())
                .commit()
        }
    }

    class SettingsFragment : PreferenceFragmentCompat() {

        override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
            setPreferencesFromResource(R.xml.preferences, rootKey)

            val context = requireContext()
            val sharedPref = PreferenceManager.getDefaultSharedPreferences(context)

            val currentMac = sharedPref.getString(PREF_KEY_REMOTE_MAC_ADDRESS, "00:00:00:00:00:00") ?: "00:00:00:00:00:00"
            setRemoteMACAddressPrefSummary(currentMac)

            findPreference<Preference>(PREF_KEY_REMOTE_MAC_ADDRESS)?.setOnPreferenceChangeListener { _, value ->
                val mac = (value as String).trim()
                if (MAC_PATTERN.matcher(mac).find()) {
                    setRemoteMACAddressPrefSummary(mac)
                    true
                } else {
                    Toast.makeText(context, R.string.mac_format_error, Toast.LENGTH_LONG).show()
                    false
                }
            }
        }

        private fun setRemoteMACAddressPrefSummary(summary: String) {
            val pref = findPreference<Preference>(PREF_KEY_REMOTE_MAC_ADDRESS)
            pref?.summary = summary
            prefRemoteMACAddress = summary
        }
    }
}